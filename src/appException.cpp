// ファイル概要: CanSat処理全体で使用する例外クラスを実装するソース
// 作成者コンテキスト: 部活動CanSat向けのRaspberry Pi Zero 2W制御コード
// 日付: YYYY-MM-DD
#include "appException.hpp"

// アプリケーション固有例外の基底クラスを初期化します。
// 引数: message は例外の詳細メッセージです。
// 戻り値: なし。
AppException::AppException(const std::string& message)
    : std::runtime_error(message) {}

// カメラ処理向け例外を初期化します。
// 引数: message は例外の詳細メッセージです。
// 戻り値: なし。
CameraException::CameraException(const std::string& message)
    : AppException(message) {}

// GPS処理向け例外を初期化します。
// 引数: message は例外の詳細メッセージです。
// 戻り値: なし。
GpsException::GpsException(const std::string& message)
    : AppException(message) {}

// ログ処理向け例外を初期化します。
// 引数: message は例外の詳細メッセージです。
// 戻り値: なし。
LogException::LogException(const std::string& message)
    : AppException(message) {}

// 引数処理向け例外を初期化します。
// 引数: message は例外の詳細メッセージです。
// 戻り値: なし。
ArgumentException::ArgumentException(const std::string& message)
    : AppException(message) {}
