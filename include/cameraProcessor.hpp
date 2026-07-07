// ファイル概要: YUV色空間で赤色コーンを検出するカメラ処理クラスを定義するヘッダー
// 作成者コンテキスト: 部活動CanSat向けのRaspberry Pi Zero 2W制御コード
// 日付: YYYY-MM-DD
#ifndef CAMERA_PROCESSOR_HPP
#define CAMERA_PROCESSOR_HPP

#include <cstddef>
#include <cstdint>

#include "cameraTypes.hpp"

// CAMERA_HORIZONTAL_FOV_DEGREESは、Raspberry Pi Camera V2.1の水平画角の代表値です。
constexpr double CAMERA_HORIZONTAL_FOV_DEGREES = 62.2;

// CameraDetectionConfigは、赤色検出と角度計算の調整値を保持します。
// 引数: しきい値は屋外芝生上の赤色コーンを想定したYUV条件です。
// 戻り値: なし。
struct CameraDetectionConfig final {
  std::uint8_t min_y;
  std::uint8_t max_y;
  std::uint8_t max_u;
  std::uint8_t min_v;
  int min_v_minus_u;
  std::size_t min_red_pixels;
  double horizontal_fov_degrees;
  double best_position_pixel_tolerance;
};

// CameraProcessorは、YUV420フレームから赤色コーンの位置と方向角を算出します。
// 引数: config は検出しきい値とカメラ画角です。
// 戻り値: なし。
class CameraProcessor final {
 public:
  // カメラ処理クラスを初期化します。
  // 引数: config は赤色判定と角度計算に使う設定値です。
  // 戻り値: なし。
  explicit CameraProcessor(CameraDetectionConfig config);

  // Raspberry Pi Camera V2.1向けの標準検出設定を返します。
  // 引数: なし。
  // 戻り値: 屋外での赤色コーン検出を想定した設定値です。
  [[nodiscard]] static CameraDetectionConfig DefaultConfig() noexcept;

  // YUV420フレームの寸法とplaneサイズが妥当か確認します。
  // 引数: frame は確認対象のYUV420フレームです。
  // 戻り値: 妥当ならtrueです。
  [[nodiscard]] bool CheckFrameShape(const YuvFrame& frame) const noexcept;

  // 1ピクセルが赤色コーン候補かを判定します。
  // 引数: y/u/v はYUV成分です。
  // 戻り値: 赤色候補ならtrueです。
  [[nodiscard]] bool IsRedPixel(std::uint8_t y, std::uint8_t u,
                                std::uint8_t v) const noexcept;

  // 画像上のx座標を機体正面からの相対角に変換します。
  // 引数: x_position は画像上の横座標、image_width は画像幅です。
  // 戻り値: 右方向を正とする角度です。
  [[nodiscard]] double CalculateDirectionDegrees(
      double x_position, std::size_t image_width) const;

  // 画像中心で四分割したときの象限を判定します。
  // 引数: point は対象座標、width/height は画像サイズです。
  // 戻り値: 第1象限から第4象限、無効時はkNoneです。
  [[nodiscard]] ConeQuadrant DetermineQuadrant(const Point2D& point,
                                               std::size_t width,
                                               std::size_t height) const
      noexcept;

  // コーン中心が画像中心に十分近いかを判定します。
  // 引数: point はコーン重心、width/height は画像サイズです。
  // 戻り値: 許容距離以内ならtrueです。
  [[nodiscard]] bool IsBestPosition(const Point2D& point, std::size_t width,
                                    std::size_t height) const noexcept;

  // YUV420フレームから赤色コーンを検出します。
  // 引数: frame は解析対象のYUV420フレームです。
  // 戻り値: 検出有無、座標、象限、方向角を含む結果です。
  [[nodiscard]] ConeDetection DetectCone(const YuvFrame& frame) const;

  // 現在の検出設定を返します。
  // 引数: なし。
  // 戻り値: コンストラクタで設定された値です。
  [[nodiscard]] const CameraDetectionConfig& GetConfig() const noexcept;

 private:
  CameraDetectionConfig config_;
};

#endif  // CAMERA_PROCESSOR_HPP
