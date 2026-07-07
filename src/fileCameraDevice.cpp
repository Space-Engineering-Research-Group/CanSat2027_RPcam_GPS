// ファイル概要: YUV420ファイルをカメラ入力として扱うデバイス処理を実装するソース
// 作成者コンテキスト: 部活動CanSat向けのRaspberry Pi Zero 2W制御コード
// 日付: YYYY-MM-DD
#include "fileCameraDevice.hpp"

#include <fstream>
#include <iterator>

#include "appException.hpp"

namespace {

// YUV420フレームに必要な総バイト数を計算します。
// 引数: width/height は画像サイズです。
// 戻り値: Y plane + U plane + V planeの合計バイト数です。
std::size_t CalculateYuv420ByteSize(const std::size_t width,
                                    const std::size_t height) noexcept {
  return (width * height) + ((width / 2U) * (height / 2U) * 2U);
}

}  // namespace

// ファイルカメラデバイスを初期化します。
// 引数: frame_path は入力ファイル、width/height はYUV420画像サイズです。
// 戻り値: なし。
FileCameraDevice::FileCameraDevice(std::filesystem::path frame_path,
                                   const std::size_t width,
                                   const std::size_t height)
    : frame_path_(std::move(frame_path)), width_(width), height_(height) {}

// 入力YUV420ファイルが読み取り可能か確認します。
// 引数: なし。
// 戻り値: ファイルが存在し必要サイズ以上ならtrueです。
bool FileCameraDevice::IsConnected() const {
  if (!std::filesystem::is_regular_file(frame_path_)) {
    return false;
  }
  const std::uintmax_t required_size = CalculateYuv420ByteSize(width_, height_);
  return std::filesystem::file_size(frame_path_) >= required_size;
}

// YUV420ファイルを1フレームとして読み込みます。
// 引数: なし。
// 戻り値: Y/U/V planeを含むフレームです。
YuvFrame FileCameraDevice::CaptureFrame() const {
  if (!IsConnected()) {
    throw CameraException("YUV420 frame file is not readable or too small");
  }

  std::ifstream input(frame_path_, std::ios::binary);
  if (!input) {
    throw CameraException("failed to open YUV420 frame file");
  }

  const std::size_t y_size = width_ * height_;
  const std::size_t uv_size = (width_ / 2U) * (height_ / 2U);
  YuvFrame frame{width_, height_, std::vector<std::uint8_t>(y_size),
                 std::vector<std::uint8_t>(uv_size),
                 std::vector<std::uint8_t>(uv_size)};

  input.read(reinterpret_cast<char*>(frame.y_plane.data()),
             static_cast<std::streamsize>(frame.y_plane.size()));
  input.read(reinterpret_cast<char*>(frame.u_plane.data()),
             static_cast<std::streamsize>(frame.u_plane.size()));
  input.read(reinterpret_cast<char*>(frame.v_plane.data()),
             static_cast<std::streamsize>(frame.v_plane.size()));
  if (!input) {
    throw CameraException("failed to read complete YUV420 frame");
  }

  return frame;
}

// 入力ファイルパスを返します。
// 引数: なし。
// 戻り値: コンストラクタで設定されたパスです。
const std::filesystem::path& FileCameraDevice::GetFramePath() const noexcept {
  return frame_path_;
}
