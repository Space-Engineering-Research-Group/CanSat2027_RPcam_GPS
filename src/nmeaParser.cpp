// ファイル概要: GPSモジュールから出力されるNMEA RMC文を解析する処理を実装するソース
// 作成者コンテキスト: 部活動CanSat向けのRaspberry Pi Zero 2W制御コード
// 日付: YYYY-MM-DD
#include "nmeaParser.hpp"
#include <cmath>

#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <vector>

#include "appException.hpp"

namespace {

// 文字列を区切り文字で分割します。
// 引数: text は分割対象、delimiter は区切り文字です。
// 戻り値: 分割後の文字列配列です。
std::vector<std::string> Split(const std::string& text, const char delimiter) {
  std::vector<std::string> values;
  std::string current;
  std::istringstream stream(text);
  while (std::getline(stream, current, delimiter)) {
    values.push_back(current);
  }
  if (!text.empty() && text.back() == delimiter) {
    values.emplace_back("");
  }
  return values;
}

// 文字列をdoubleへ変換します。
// 引数: value は変換対象、field_name は例外メッセージ用の項目名です。
// 戻り値: 変換したdouble値です。
double ParseDouble(const std::string& value, const std::string& field_name) {
  if (value.empty()) {
    throw GpsException(field_name + " is empty");
  }
  char* end_pointer = nullptr;
  const double parsed = std::strtod(value.c_str(), &end_pointer);
  if (end_pointer == value.c_str() || *end_pointer != '\0') {
    throw GpsException(field_name + " is not a valid number");
  }
  return parsed;
}

// NMEA文からチェックサム以降を除いた本文を取り出します。
// 引数: line はNMEAの1行文字列です。
// 戻り値: $を除き、*より前までの本文です。
std::string ExtractBody(const std::string& line) {
  if (line.empty() || line.front() != '$') {
    throw GpsException("NMEA sentence must start with '$'");
  }
  const std::size_t checksum_position = line.find('*');
  if (checksum_position == std::string::npos) {
    return line.substr(1U);
  }
  return line.substr(1U, checksum_position - 1U);
}

// 16進文字を整数へ変換します。
// 引数: character は0-9/A-F/a-fの1文字です。
// 戻り値: 0から15の整数、無効時は-1です。
int HexValue(const char character) noexcept {
  if (character >= '0' && character <= '9') {
    return character - '0';
  }
  if (character >= 'A' && character <= 'F') {
    return character - 'A' + 10;
  }
  if (character >= 'a' && character <= 'f') {
    return character - 'a' + 10;
  }
  return -1;
}

}  // namespace

// NMEA文が対応対象のRMC文かを確認します。
// 引数: line はNMEAの1行文字列です。
// 戻り値: GPRMC/GNRMC/GARMC/GLRMC/BDRMCならtrueです。
bool NmeaParser::IsSupportedSentence(const std::string& line) const noexcept {
  if (line.size() < 6U || line.front() != '$') {
    return false;
  }
  return line.compare(3U, 3U, "RMC") == 0;
}

// NMEA文のチェックサムを検証します。
// 引数: line はNMEAの1行文字列です。
// 戻り値: チェックサムなし、または正しいチェックサムならtrueです。
bool NmeaParser::ValidateChecksum(const std::string& line) const noexcept {
  const std::size_t checksum_position = line.find('*');
  if (checksum_position == std::string::npos) {
    return true;
  }
  if (line.empty() || line.front() != '$' ||
      (checksum_position + 2U) >= line.size()) {
    return false;
  }

  std::uint8_t checksum = 0U;
  for (std::size_t index = 1U; index < checksum_position; ++index) {
    checksum ^= static_cast<std::uint8_t>(line[index]);  // NMEAは本文のXORを使います。
  }

  const int high = HexValue(line[checksum_position + 1U]);
  const int low = HexValue(line[checksum_position + 2U]);
  if (high < 0 || low < 0) {
    return false;
  }
  const int expected = (high * 16) + low;
  return checksum == static_cast<std::uint8_t>(expected);
}

// NMEAのddmm.mmmm形式座標を十進度へ変換します。
// 引数: value は座標文字列、hemisphere はN/S/E/Wです。
// 戻り値: 十進度の座標です。
double NmeaParser::ParseCoordinateDegrees(
    const std::string& value, const std::string& hemisphere) const {
  const double raw_coordinate = ParseDouble(value, "coordinate");
  const double degrees = std::floor(raw_coordinate / 100.0);
  const double minutes = raw_coordinate - (degrees * 100.0);
  double decimal_degrees = degrees + (minutes / 60.0);
  if (hemisphere == "S" || hemisphere == "W") {
    decimal_degrees = -decimal_degrees;
  } else if (hemisphere != "N" && hemisphere != "E") {
    throw GpsException("invalid coordinate hemisphere");
  }
  return decimal_degrees;
}

// RMC文をGPSサンプルへ変換します。
// 引数: line はNMEA RMC文です。
// 戻り値: 測位情報、速度、進行方位を含むGPSサンプルです。
GpsSample NmeaParser::ParseLine(const std::string& line) const {
  if (!IsSupportedSentence(line)) {
    throw GpsException("unsupported NMEA sentence");
  }
  if (!ValidateChecksum(line)) {
    throw GpsException("invalid NMEA checksum");
  }

  const std::vector<std::string> fields = Split(ExtractBody(line), ',');
  if (fields.size() < 9U) {
    throw GpsException("RMC sentence does not contain enough fields");
  }

  const bool has_fix = fields[2U] == "A";
  if (!has_fix) {
    return GpsSample{Coordinate{0.0, 0.0}, 0.0, std::nullopt, false, line};
  }

  const double latitude =
      ParseCoordinateDegrees(fields[3U], fields[4U]);
  const double longitude =
      ParseCoordinateDegrees(fields[5U], fields[6U]);
  const double speed_mps = fields[7U].empty()
                               ? 0.0
                               : ParseDouble(fields[7U], "speed_knots") *
                                     KNOT_TO_METERS_PER_SECOND;
  std::optional<double> course_degrees = std::nullopt;
  if (!fields[8U].empty()) {
    course_degrees = NormalizeDegrees(ParseDouble(fields[8U], "course"));
  }

  return GpsSample{Coordinate{latitude, longitude}, speed_mps, course_degrees,
                   true, line};
}
