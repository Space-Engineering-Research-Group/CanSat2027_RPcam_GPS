// ファイル概要: YUV色空間で赤色コーンを検出するカメラ処理を実装するソース
// 作成者コンテキスト: 部活動CanSat向けのRaspberry Pi Zero 2W制御コード
// 日付: YYYY-MM-DD
#include "cameraProcessor.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include "appException.hpp"

namespace {

// ComponentStatsは、赤色連結成分の重心と外接矩形を計算するための一時情報です。
// 引数: count は画素数、sum_x/sum_y は重心計算用の合計値です。
// 戻り値: なし。
struct ComponentStats final {
  std::size_t count;
  double sum_x;
  double sum_y;
  BoundingBox bounding_box;
};

// 無効な外接矩形を作成します。
// 引数: なし。
// 戻り値: IsValidがfalseになる外接矩形です。
BoundingBox MakeInvalidBoundingBox() noexcept {
  return BoundingBox{1U, 1U, 0U, 0U};
}

// 連結成分探索中に外接矩形を更新します。
// 引数: box は更新対象、x/y は現在の画素座標です。
// 戻り値: なし。
void UpdateBoundingBox(BoundingBox* const box, const std::size_t x,
                       const std::size_t y) noexcept {
  box->min_x = std::min(box->min_x, x);
  box->min_y = std::min(box->min_y, y);
  box->max_x = std::max(box->max_x, x);
  box->max_y = std::max(box->max_y, y);
}

// 空の検出結果を作成します。
// 引数: なし。
// 戻り値: detected=falseの検出結果です。
ConeDetection MakeEmptyDetection() noexcept {
  return ConeDetection{false, ConeQuadrant::kNone, Point2D{0.0, 0.0},
                       MakeInvalidBoundingBox(), 0.0, false, 0U};
}

}  // namespace

// カメラ処理クラスを初期化します。
// 引数: config は赤色判定と角度計算に使う設定値です。
// 戻り値: なし。
CameraProcessor::CameraProcessor(const CameraDetectionConfig config)
    : config_(config) {}

// Raspberry Pi Camera V2.1向けの標準検出設定を返します。
// 引数: なし。
// 戻り値: 屋外での赤色コーン検出を想定した設定値です。
CameraDetectionConfig CameraProcessor::DefaultConfig() noexcept {
  return CameraDetectionConfig{25U, 245U, 109U, 157U, 42, 24U,
                               CAMERA_HORIZONTAL_FOV_DEGREES, 8.0};
}

// YUV420フレームの寸法とplaneサイズが妥当か確認します。
// 引数: frame は確認対象のYUV420フレームです。
// 戻り値: 妥当ならtrueです。
bool CameraProcessor::CheckFrameShape(const YuvFrame& frame) const noexcept {
  if (frame.width == 0U || frame.height == 0U) {
    return false;
  }
  if ((frame.width % 2U) != 0U || (frame.height % 2U) != 0U) {
    return false;  // YUV420のUV planeは2x2画素で共有するため偶数サイズに限定します。
  }
  const std::size_t y_size = frame.width * frame.height;
  const std::size_t uv_size = (frame.width / 2U) * (frame.height / 2U);
  return frame.y_plane.size() == y_size && frame.u_plane.size() == uv_size &&
         frame.v_plane.size() == uv_size;
}

// 1ピクセルが赤色コーン候補かを判定します。
// 引数: y/u/v はYUV成分です。
// 戻り値: 赤色候補ならtrueです。
bool CameraProcessor::IsRedPixel(const std::uint8_t y, const std::uint8_t u,
                                 const std::uint8_t v) const noexcept {
  const int chroma_difference = static_cast<int>(v) - static_cast<int>(u);
  return y >= config_.min_y && y <= config_.max_y && u <= config_.max_u &&
         v >= config_.min_v && chroma_difference >= config_.min_v_minus_u;
}

// 画像上のx座標を機体正面からの相対角に変換します。
// 引数: x_position は画像上の横座標、image_width は画像幅です。
// 戻り値: 右方向を正とする角度です。
double CameraProcessor::CalculateDirectionDegrees(
    const double x_position, const std::size_t image_width) const {
  if (image_width == 0U) {
    throw CameraException("image_width must be greater than zero");
  }
  const double center_x = static_cast<double>(image_width) / 2.0;
  const double normalized_offset = (x_position - center_x) / center_x;
  return normalized_offset * (config_.horizontal_fov_degrees / 2.0);
}

// 画像中心で四分割したときの象限を判定します。
// 引数: point は対象座標、width/height は画像サイズです。
// 戻り値: 第1象限から第4象限、無効時はkNoneです。
ConeQuadrant CameraProcessor::DetermineQuadrant(const Point2D& point,
                                                const std::size_t width,
                                                const std::size_t height) const
    noexcept {
  if (width == 0U || height == 0U) {
    return ConeQuadrant::kNone;
  }
  const double center_x = static_cast<double>(width) / 2.0;
  const double center_y = static_cast<double>(height) / 2.0;
  if (point.x >= center_x && point.y < center_y) {
    return ConeQuadrant::kFirst;
  }
  if (point.x < center_x && point.y < center_y) {
    return ConeQuadrant::kSecond;
  }
  if (point.x < center_x && point.y >= center_y) {
    return ConeQuadrant::kThird;
  }
  return ConeQuadrant::kFourth;
}

// コーン中心が画像中心に十分近いかを判定します。
// 引数: point はコーン重心、width/height は画像サイズです。
// 戻り値: 許容距離以内ならtrueです。
bool CameraProcessor::IsBestPosition(const Point2D& point,
                                     const std::size_t width,
                                     const std::size_t height) const noexcept {
  if (width == 0U || height == 0U) {
    return false;
  }
  const double center_x = static_cast<double>(width) / 2.0;
  const double center_y = static_cast<double>(height) / 2.0;
  const double diff_x = point.x - center_x;
  const double diff_y = point.y - center_y;
  const double distance = std::sqrt((diff_x * diff_x) + (diff_y * diff_y));
  return distance <= config_.best_position_pixel_tolerance;
}

// YUV420フレームから赤色コーンを検出します。
// 引数: frame は解析対象のYUV420フレームです。
// 戻り値: 検出有無、座標、象限、方向角を含む結果です。
ConeDetection CameraProcessor::DetectCone(const YuvFrame& frame) const {
  if (!CheckFrameShape(frame)) {
    throw CameraException("invalid YUV420 frame shape");
  }

  std::vector<std::uint8_t> mask(frame.width * frame.height, 0U);
  for (std::size_t y = 0U; y < frame.height; ++y) {
    for (std::size_t x = 0U; x < frame.width; ++x) {
      const std::size_t y_index = (y * frame.width) + x;
      const std::size_t uv_index =
          ((y / 2U) * (frame.width / 2U)) + (x / 2U);
      if (IsRedPixel(frame.y_plane[y_index], frame.u_plane[uv_index],
                     frame.v_plane[uv_index])) {
        mask[y_index] = 1U;
      }
    }
  }

  ComponentStats best_component{
      0U, 0.0, 0.0,
      BoundingBox{std::numeric_limits<std::size_t>::max(),
                  std::numeric_limits<std::size_t>::max(), 0U, 0U}};
  std::vector<std::size_t> stack;
  stack.reserve(frame.width * frame.height);

  for (std::size_t start = 0U; start < mask.size(); ++start) {
    if (mask[start] != 1U) {
      continue;
    }

    ComponentStats current{
        0U, 0.0, 0.0,
        BoundingBox{std::numeric_limits<std::size_t>::max(),
                    std::numeric_limits<std::size_t>::max(), 0U, 0U}};
    stack.clear();
    stack.push_back(start);
    mask[start] = 2U;  // 2は探索済みの赤色画素を表します。

    while (!stack.empty()) {
      const std::size_t index = stack.back();
      stack.pop_back();
      const std::size_t x = index % frame.width;
      const std::size_t y = index / frame.width;
      current.count += 1U;
      current.sum_x += static_cast<double>(x);
      current.sum_y += static_cast<double>(y);
      UpdateBoundingBox(&current.bounding_box, x, y);

      if (x > 0U) {
        const std::size_t left = index - 1U;
        if (mask[left] == 1U) {
          mask[left] = 2U;
          stack.push_back(left);
        }
      }
      if ((x + 1U) < frame.width) {
        const std::size_t right = index + 1U;
        if (mask[right] == 1U) {
          mask[right] = 2U;
          stack.push_back(right);
        }
      }
      if (y > 0U) {
        const std::size_t up = index - frame.width;
        if (mask[up] == 1U) {
          mask[up] = 2U;
          stack.push_back(up);
        }
      }
      if ((y + 1U) < frame.height) {
        const std::size_t down = index + frame.width;
        if (mask[down] == 1U) {
          mask[down] = 2U;
          stack.push_back(down);
        }
      }
    }

    if (current.count > best_component.count) {
      best_component = current;
    }
  }

  if (best_component.count < config_.min_red_pixels) {
    return MakeEmptyDetection();
  }

  const Point2D center{best_component.sum_x /
                           static_cast<double>(best_component.count),
                       best_component.sum_y /
                           static_cast<double>(best_component.count)};
  return ConeDetection{
      true,
      DetermineQuadrant(center, frame.width, frame.height),
      center,
      best_component.bounding_box,
      CalculateDirectionDegrees(center.x, frame.width),
      IsBestPosition(center, frame.width, frame.height),
      best_component.count};
}

// 現在の検出設定を返します。
// 引数: なし。
// 戻り値: コンストラクタで設定された値です。
const CameraDetectionConfig& CameraProcessor::GetConfig() const noexcept {
  return config_;
}
