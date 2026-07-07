// ファイル概要: カメラ・GPS処理のログ記録とフェーズ終了時送信を実装するソース
// 作成者コンテキスト: 部活動CanSat向けのRaspberry Pi Zero 2W制御コード
// 日付: YYYY-MM-DD
#include "logSender.hpp"

#include <fstream>
#include <iomanip>
#include <optional>
#include <sstream>

#include "appException.hpp"

namespace {

// 任意値のdoubleをCSV向け文字列に変換します。
// 引数: value は任意のdouble値です。
// 戻り値: 値があれば小数6桁、なければ空文字列です。
std::string OptionalDoubleToString(const std::optional<double>& value) {
  if (!value.has_value()) {
    return "";
  }
  std::ostringstream stream;
  stream << std::fixed << std::setprecision(6) << value.value();
  return stream.str();
}

// double値をCSV向け文字列に変換します。
// 引数: value は変換対象のdouble値です。
// 戻り値: 小数6桁の文字列です。
std::string DoubleToString(const double value) {
  std::ostringstream stream;
  stream << std::fixed << std::setprecision(6) << value;
  return stream.str();
}

// 文字列をCSVの1フィールドとして安全に囲みます。
// 引数: value はCSVへ出力する文字列です。
// 戻り値: ダブルクォートで囲んだCSVフィールドです。
std::string QuoteCsv(const std::string& value) {
  std::string escaped;
  escaped.reserve(value.size() + 2U);
  escaped.push_back('"');
  for (const char character : value) {
    if (character == '"') {
      escaped.push_back('"');  // CSVではダブルクォートを2つ重ねてエスケープします。
    }
    escaped.push_back(character);
  }
  escaped.push_back('"');
  return escaped;
}

// 1行をファイルへ追記します。
// 引数: path は出力先、line は追記するCSV行です。
// 戻り値: なし。
void AppendLine(const std::filesystem::path& path, const std::string& line) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream output(path, std::ios::app);
  if (!output) {
    throw LogException("failed to open log file");
  }
  output << line << '\n';
  if (!output) {
    throw LogException("failed to write log file");
  }
}

}  // namespace

// ログ送信クラスを初期化します。
// 引数: paths はローカル保存先と送信済み保存先です。
// 戻り値: なし。
LogSender::LogSender(LogPaths paths) : paths_(std::move(paths)) {}

// カメラ検出結果をCSV行へ変換します。
// 引数: phase_name はフェーズ名、detection はカメラ検出結果です。
// 戻り値: CSV形式の1行です。
std::string LogSender::BuildCameraLogLine(
    const std::string& phase_name, const ConeDetection& detection) const {
  std::ostringstream stream;
  stream << "camera," << QuoteCsv(phase_name) << ','
         << (detection.detected ? "1" : "0") << ','
         << ToString(detection.quadrant) << ','
         << DoubleToString(detection.cone_center.x) << ','
         << DoubleToString(detection.cone_center.y) << ','
         << detection.bounding_box.min_x << ',' << detection.bounding_box.min_y
         << ',' << detection.bounding_box.max_x << ','
         << detection.bounding_box.max_y << ','
         << DoubleToString(detection.direction_degrees) << ','
         << (detection.is_best_position ? "1" : "0") << ','
         << detection.red_pixel_count;
  return stream.str();
}

// GPS航法結果をCSV行へ変換します。
// 引数: phase_name はフェーズ名、record は航法記録です。
// 戻り値: CSV形式の1行です。
std::string LogSender::BuildNavigationLogLine(
    const std::string& phase_name, const NavigationRecord& record) const {
  std::ostringstream stream;
  stream << "gps," << QuoteCsv(phase_name) << ','
         << DoubleToString(record.current_coordinate.latitude_degrees) << ','
         << DoubleToString(record.current_coordinate.longitude_degrees) << ','
         << DoubleToString(record.target_coordinate.latitude_degrees) << ','
         << DoubleToString(record.target_coordinate.longitude_degrees) << ','
         << DoubleToString(record.target_bearing_degrees) << ','
         << DoubleToString(record.target_distance_meters) << ','
         << OptionalDoubleToString(record.camera_direction_degrees) << ','
         << (record.camera_agrees_with_gps ? "1" : "0") << ','
         << DoubleToString(record.corrected_direction_degrees) << ','
         << OptionalDoubleToString(record.heading_error_degrees) << ','
         << (record.gps_heading_reliable ? "1" : "0") << ','
         << ToString(record.decision);
  return stream.str();
}

// カメラ検出結果をローカルログへ追記します。
// 引数: phase_name はフェーズ名、detection はカメラ検出結果です。
// 戻り値: なし。
void LogSender::AppendCameraRecord(const std::string& phase_name,
                                   const ConeDetection& detection) const {
  AppendLine(paths_.local_log_path, BuildCameraLogLine(phase_name, detection));
}

// GPS航法結果をローカルログへ追記します。
// 引数: phase_name はフェーズ名、record は航法記録です。
// 戻り値: なし。
void LogSender::AppendNavigationRecord(
    const std::string& phase_name, const NavigationRecord& record) const {
  AppendLine(paths_.local_log_path,
             BuildNavigationLogLine(phase_name, record));
}

// フェーズ終了時にローカルログを送信済みログへ転記します。
// 引数: phase_name は終了したフェーズ名です。
// 戻り値: なし。
void LogSender::SendPhaseLog(const std::string& phase_name) const {
  std::filesystem::create_directories(paths_.sent_log_path.parent_path());
  std::ifstream input(paths_.local_log_path);
  if (!input) {
    throw LogException("failed to open local log for sending");
  }
  std::ofstream output(paths_.sent_log_path, std::ios::app);
  if (!output) {
    throw LogException("failed to open sent log");
  }
  output << "phase_end," << QuoteCsv(phase_name) << '\n';
  std::string line;
  while (std::getline(input, line)) {
    output << line << '\n';
  }
  if (!output) {
    throw LogException("failed to send phase log");
  }
}

// ログ出力先を返します。
// 引数: なし。
// 戻り値: コンストラクタで設定されたパスです。
const LogPaths& LogSender::GetPaths() const noexcept {
  return paths_;
}
