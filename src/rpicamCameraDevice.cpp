// ファイル概要: rpicam-vidを使ってRaspberry Pi CameraからYUV420フレームを取得する処理を実装するソース
// 作成者コンテキスト: 部活動CanSat向けのRaspberry Pi Zero 2W制御コード
// 日付: YYYY-MM-DD
#include "rpicamCameraDevice.hpp"

#include <cstdlib>
#include <sstream>
#include <utility>

#include "appException.hpp"
#include "fileCameraDevice.hpp"

namespace {

// shell引数として安全に扱えるようにシングルクォートで囲みます。
// 引数: value はshellへ渡す文字列です。
// 戻り値: POSIX shell向けにクォートした文字列です。
std::string ShellQuote(const std::string& value) {
  std::string quoted;
  quoted.reserve(value.size() + 2U);
  quoted.push_back('\'');
  for (const char character : value) {
    if (character == '\'') {
      quoted += "'\\''";  // POSIX shellではクォートを閉じてエスケープします。
    } else {
      quoted.push_back(character);
    }
  }
  quoted.push_back('\'');
  return quoted;
}

}  // namespace

// rpicamカメラデバイスを初期化します。
// 引数: output_frame_path は一時YUV保存先、width/height は画像サイズ、capture_binary/list_binary は実行コマンドです。
// 戻り値: なし。
RpicamCameraDevice::RpicamCameraDevice(std::filesystem::path output_frame_path,
                                       const std::size_t width,
                                       const std::size_t height,
                                       std::string capture_binary,
                                       std::string list_binary)
    : output_frame_path_(std::move(output_frame_path)),
      width_(width),
      height_(height),
      capture_binary_(std::move(capture_binary)),
      list_binary_(std::move(list_binary)) {}

// Raspberry Pi Cameraがrpicamから認識できるか確認します。
// 引数: なし。
// 戻り値: rpicam-hello --list-camerasが成功すればtrueです。
bool RpicamCameraDevice::IsConnected() const {
  const std::string command =
      ShellQuote(list_binary_) + " --list-cameras >/dev/null 2>&1";
  return std::system(command.c_str()) == 0;
}

// rpicam-vidを起動してYUV420フレームを1枚取得します。
// 引数: なし。
// 戻り値: 取得したYUV420フレームです。
YuvFrame RpicamCameraDevice::CaptureFrame() const {
  const std::filesystem::path parent_path = output_frame_path_.parent_path();
  if (!parent_path.empty()) {
    std::filesystem::create_directories(parent_path);
  }
  const int result = std::system(BuildCaptureCommand().c_str());
  if (result != 0) {
    throw CameraException("rpicam capture command failed");
  }
  return FileCameraDevice(output_frame_path_, width_, height_).CaptureFrame();
}

// rpicam-vidのコマンド文字列を生成します。
// 引数: なし。
// 戻り値: shellで実行するキャプチャコマンドです。
std::string RpicamCameraDevice::BuildCaptureCommand() const {
  std::ostringstream command;
  command << ShellQuote(capture_binary_) << " --codec yuv420 --width "
          << width_ << " --height " << height_ << " --frames 1 --timeout "
          << RPICAM_CAPTURE_TIMEOUT_MS << " --output "
          << ShellQuote(output_frame_path_.string()) << " >/dev/null 2>&1";
  return command.str();
}

// 一時YUV保存先を返します。
// 引数: なし。
// 戻り値: コンストラクタで設定されたパスです。
const std::filesystem::path& RpicamCameraDevice::GetOutputFramePath() const
    noexcept {
  return output_frame_path_;
}
