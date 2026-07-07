// ファイル概要: GPSモジュールから出力されるNMEA RMC文を解析するクラスを定義するヘッダー
// 作成者コンテキスト: 部活動CanSat向けのRaspberry Pi Zero 2W制御コード
// 日付: YYYY-MM-DD
#ifndef NMEA_PARSER_HPP
#define NMEA_PARSER_HPP

#include <string>

#include "gpsTypes.hpp"

// KNOT_TO_METERS_PER_SECONDは、NMEAのノット速度をm/sへ変換する係数です。
constexpr double KNOT_TO_METERS_PER_SECOND = 0.514444;

// NmeaParserは、GPRMC/GNRMC文から座標・速度・進行方位を抽出します。
// 引数: なし。
// 戻り値: なし。
class NmeaParser final {
 public:
  // NMEA文が対応対象のRMC文かを確認します。
  // 引数: line はNMEAの1行文字列です。
  // 戻り値: GPRMC/GNRMC/GARMC/GLRMC/BDRMCならtrueです。
  [[nodiscard]] bool IsSupportedSentence(const std::string& line) const
      noexcept;

  // NMEA文のチェックサムを検証します。
  // 引数: line はNMEAの1行文字列です。
  // 戻り値: チェックサムなし、または正しいチェックサムならtrueです。
  [[nodiscard]] bool ValidateChecksum(const std::string& line) const noexcept;

  // NMEAのddmm.mmmm形式座標を十進度へ変換します。
  // 引数: value は座標文字列、hemisphere はN/S/E/Wです。
  // 戻り値: 十進度の座標です。
  [[nodiscard]] double ParseCoordinateDegrees(
      const std::string& value, const std::string& hemisphere) const;

  // RMC文をGPSサンプルへ変換します。
  // 引数: line はNMEA RMC文です。
  // 戻り値: 測位情報、速度、進行方位を含むGPSサンプルです。
  [[nodiscard]] GpsSample ParseLine(const std::string& line) const;
};

#endif  // NMEA_PARSER_HPP
