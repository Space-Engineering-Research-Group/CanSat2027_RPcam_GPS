// ファイル概要: GPS処理で共有する座標・サンプル・航法結果の型を定義するヘッダー
// 作成者コンテキスト: 部活動CanSat向けのRaspberry Pi Zero 2W制御コード
// 日付: YYYY-MM-DD
#ifndef GPS_TYPES_HPP
#define GPS_TYPES_HPP

#include <optional>
#include <string>

// EARTH_RADIUS_METERSは、球面近似の距離計算に使う地球半径です。
constexpr double EARTH_RADIUS_METERS = 6371008.8;

// Coordinateは、WGS84の緯度経度を度単位で保持します。
// 引数: latitude_degrees は緯度、longitude_degrees は経度です。
// 戻り値: なし。
struct Coordinate final {
  double latitude_degrees;
  double longitude_degrees;
};

// GpsSampleは、GPSから取得した1件分の測位情報を保持します。
// 引数: coordinate は現在地、speed_meters_per_second は対地速度、course_degrees は進行方位です。
// 戻り値: なし。
struct GpsSample final {
  Coordinate coordinate;
  double speed_meters_per_second;
  std::optional<double> course_degrees;
  bool has_fix;
  std::string source_sentence;
};

// DirectionDecisionは、カメラ方向とGPS方位の照合結果を表します。
// 引数: なし。
// 戻り値: なし。
enum class DirectionDecision {
  kGpsAbsolute,
  kGpsRelative,
  kCameraConfirmed,
  kGpsCorrected,
};

// NavigationRecordは、現在地から目標地点への航法計算結果を保持します。
// 引数: target_bearing_degrees はGPS由来の絶対方位、corrected_direction_degrees は採用した方向角です。
// 戻り値: なし。
struct NavigationRecord final {
  Coordinate current_coordinate;
  Coordinate target_coordinate;
  double target_bearing_degrees;
  double target_distance_meters;
  std::optional<double> camera_direction_degrees;
  bool camera_agrees_with_gps;
  double corrected_direction_degrees;
  std::optional<double> heading_error_degrees;
  bool gps_heading_reliable;
  DirectionDecision decision;
};

// 角度を0度以上360度未満へ正規化します。
// 引数: angle_degrees は正規化対象の角度です。
// 戻り値: 0度以上360度未満の角度です。
[[nodiscard]] double NormalizeDegrees(double angle_degrees) noexcept;

// 角度を-180度以上180度未満へ正規化します。
// 引数: angle_degrees は正規化対象の角度です。
// 戻り値: -180度以上180度未満の角度です。
[[nodiscard]] double NormalizeSignedDegrees(double angle_degrees) noexcept;

// 航法判断の列挙値をログ向けの文字列に変換します。
// 引数: decision は変換対象の判断値です。
// 戻り値: 英数字の判断名です。
[[nodiscard]] std::string ToString(DirectionDecision decision);

#endif  // GPS_TYPES_HPP
