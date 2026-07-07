// ファイル概要: YUV420ファイルをカメラ入力として扱うデバイスクラスを定義するヘッダー
// 作成者コンテキスト: 部活動CanSat向けのRaspberry Pi Zero 2W制御コード
// 日付: YYYY-MM-DD
#ifndef FILE_CAMERA_DEVICE_HPP
#define FILE_CAMERA_DEVICE_HPP

#include <filesystem>

#include "cameraTypes.hpp"

// FileCameraDeviceは、rpicam等で保存したYUV420ファイルを読み込むカメラ入力です。
// 引数: frame_path はYUV420ファイル、width/height は画像サイズです。
// 戻り値: なし。
class FileCameraDevice final {
 public:
  // ファイルカメラデバイスを初期化します。
  // 引数: frame_path は入力ファイル、width/height はYUV420画像サイズです。
  // 戻り値: なし。
  FileCameraDevice(std::filesystem::path frame_path, std::size_t width,
                   std::size_t height);

  // 入力YUV420ファイルが読み取り可能か確認します。
  // 引数: なし。
  // 戻り値: ファイルが存在し必要サイズ以上ならtrueです。
  [[nodiscard]] bool IsConnected() const;

  // YUV420ファイルを1フレームとして読み込みます。
  // 引数: なし。
  // 戻り値: Y/U/V planeを含むフレームです。
  [[nodiscard]] YuvFrame CaptureFrame() const;

  // 入力ファイルパスを返します。
  // 引数: なし。
  // 戻り値: コンストラクタで設定されたパスです。
  [[nodiscard]] const std::filesystem::path& GetFramePath() const noexcept;

 private:
  std::filesystem::path frame_path_;
  std::size_t width_;
  std::size_t height_;
};

#endif  // FILE_CAMERA_DEVICE_HPP
