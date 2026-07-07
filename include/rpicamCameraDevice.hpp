// ファイル概要: rpicam-vidを使ってRaspberry Pi CameraからYUV420フレームを取得するクラスを定義するヘッダー
// 作成者コンテキスト: 部活動CanSat向けのRaspberry Pi Zero 2W制御コード
// 日付: YYYY-MM-DD
#ifndef RPICAM_CAMERA_DEVICE_HPP
#define RPICAM_CAMERA_DEVICE_HPP

#include <filesystem>
#include <string>

#include "cameraTypes.hpp"

// RPICAM_CAPTURE_TIMEOUT_MSは、1フレーム取得時のrpicam待機時間です。
constexpr int RPICAM_CAPTURE_TIMEOUT_MS = 1000;

// RpicamCameraDeviceは、Raspberry Pi OS Bookwormのrpicamコマンドでカメラを扱います。
// 引数: output_frame_path は一時YUV保存先、width/height は画像サイズです。
// 戻り値: なし。
class RpicamCameraDevice final {
 public:
  // rpicamカメラデバイスを初期化します。
  // 引数: output_frame_path は一時YUV保存先、width/height は画像サイズ、capture_binary/list_binary は実行コマンドです。
  // 戻り値: なし。
  RpicamCameraDevice(std::filesystem::path output_frame_path,
                     std::size_t width, std::size_t height,
                     std::string capture_binary, std::string list_binary);

  // Raspberry Pi Cameraがrpicamから認識できるか確認します。
  // 引数: なし。
  // 戻り値: rpicam-hello --list-camerasが成功すればtrueです。
  [[nodiscard]] bool IsConnected() const;

  // rpicam-vidを起動してYUV420フレームを1枚取得します。
  // 引数: なし。
  // 戻り値: 取得したYUV420フレームです。
  [[nodiscard]] YuvFrame CaptureFrame() const;

  // rpicam-vidのコマンド文字列を生成します。
  // 引数: なし。
  // 戻り値: shellで実行するキャプチャコマンドです。
  [[nodiscard]] std::string BuildCaptureCommand() const;

  // 一時YUV保存先を返します。
  // 引数: なし。
  // 戻り値: コンストラクタで設定されたパスです。
  [[nodiscard]] const std::filesystem::path& GetOutputFramePath() const
      noexcept;

 private:
  std::filesystem::path output_frame_path_;
  std::size_t width_;
  std::size_t height_;
  std::string capture_binary_;
  std::string list_binary_;
};

#endif  // RPICAM_CAMERA_DEVICE_HPP
