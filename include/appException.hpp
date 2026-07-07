// ファイル概要: CanSat処理全体で使用する例外クラスを定義するヘッダー
// 作成者コンテキスト: 部活動CanSat向けのRaspberry Pi Zero 2W制御コード
// 日付: YYYY-MM-DD
#ifndef APP_EXCEPTION_HPP
#define APP_EXCEPTION_HPP

#include <stdexcept>
#include <string>

// AppExceptionは、アプリケーション固有の例外をまとめる基底クラスです。
// 引数: message は原因を説明する文字列です。
// 戻り値: 例外オブジェクトを生成します。
class AppException : public std::runtime_error {
 public:
  explicit AppException(const std::string& message);
};

// CameraExceptionは、カメラ接続や画像処理で発生した異常を表します。
// 引数: message は原因を説明する文字列です。
// 戻り値: 例外オブジェクトを生成します。
class CameraException final : public AppException {
 public:
  explicit CameraException(const std::string& message);
};

// GpsExceptionは、GPS接続やNMEA解析で発生した異常を表します。
// 引数: message は原因を説明する文字列です。
// 戻り値: 例外オブジェクトを生成します。
class GpsException final : public AppException {
 public:
  explicit GpsException(const std::string& message);
};

// LogExceptionは、ログ保存や送信処理で発生した異常を表します。
// 引数: message は原因を説明する文字列です。
// 戻り値: 例外オブジェクトを生成します。
class LogException final : public AppException {
 public:
  explicit LogException(const std::string& message);
};

// ArgumentExceptionは、コマンドライン引数や設定値の異常を表します。
// 引数: message は原因を説明する文字列です。
// 戻り値: 例外オブジェクトを生成します。
class ArgumentException final : public AppException {
 public:
  explicit ArgumentException(const std::string& message);
};

#endif  // APP_EXCEPTION_HPP
