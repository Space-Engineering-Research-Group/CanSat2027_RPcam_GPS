// ファイル概要: カメラ・GPS処理のログ記録とフェーズ終了時送信を扱うクラスを定義するヘッダー
// 作成者コンテキスト: 部活動CanSat向けのRaspberry Pi Zero 2W制御コード
// 日付: YYYY-MM-DD
#ifndef LOG_SENDER_HPP
#define LOG_SENDER_HPP

#include <filesystem>
#include <string>

#include "cameraTypes.hpp"
#include "gpsTypes.hpp"

// LogPathsは、保存ログと送信済みログの出力先を保持します。
// 引数: local_log_path は機体内保存ログ、sent_log_path は送信記録ログです。
// 戻り値: なし。
struct LogPaths final {
  std::filesystem::path local_log_path;
  std::filesystem::path sent_log_path;
};

// LogSenderは、CSVログの追記とフェーズ終了時の送信記録を行います。
// 引数: paths は保存先パスです。
// 戻り値: なし。
class LogSender final {
 public:
  // ログ送信クラスを初期化します。
  // 引数: paths はローカル保存先と送信済み保存先です。
  // 戻り値: なし。
  explicit LogSender(LogPaths paths);

  // カメラ検出結果をCSV行へ変換します。
  // 引数: phase_name はフェーズ名、detection はカメラ検出結果です。
  // 戻り値: CSV形式の1行です。
  [[nodiscard]] std::string BuildCameraLogLine(
      const std::string& phase_name, const ConeDetection& detection) const;

  // GPS航法結果をCSV行へ変換します。
  // 引数: phase_name はフェーズ名、record は航法記録です。
  // 戻り値: CSV形式の1行です。
  [[nodiscard]] std::string BuildNavigationLogLine(
      const std::string& phase_name, const NavigationRecord& record) const;

  // カメラ検出結果をローカルログへ追記します。
  // 引数: phase_name はフェーズ名、detection はカメラ検出結果です。
  // 戻り値: なし。
  void AppendCameraRecord(const std::string& phase_name,
                          const ConeDetection& detection) const;

  // GPS航法結果をローカルログへ追記します。
  // 引数: phase_name はフェーズ名、record は航法記録です。
  // 戻り値: なし。
  void AppendNavigationRecord(const std::string& phase_name,
                              const NavigationRecord& record) const;

  // フェーズ終了時にローカルログを送信済みログへ転記します。
  // 引数: phase_name は終了したフェーズ名です。
  // 戻り値: なし。
  void SendPhaseLog(const std::string& phase_name) const;

  // ログ出力先を返します。
  // 引数: なし。
  // 戻り値: コンストラクタで設定されたパスです。
  [[nodiscard]] const LogPaths& GetPaths() const noexcept;

 private:
  LogPaths paths_;
};

#endif  // LOG_SENDER_HPP
