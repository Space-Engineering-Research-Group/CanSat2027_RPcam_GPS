// ファイル概要: GPS座標から目標地点への距離・方位・カメラ照合を行う処理を実装するソース
// 作成者コンテキスト: 部活動CanSat向けのRaspberry Pi Zero 2W制御コード
// 日付: YYYY-MM-DD
#include "gpsProcessor.hpp"

#include <cmath>
#include <cstdlib>

#include "appException.hpp"

namespace {

// 度をラジアンへ変換します。
// 引数: degrees は度単位の角度です。
// 戻り値: ラジアン単位の角度です。
double ToRadians(const double degrees) noexcept {
  return degrees * 3.14159265358979323846 / 180.0;
}

// ラジアンを度へ変換します。
// 引数: radians はラジアン単位の角度です。
// 戻り値: 度単位の角度です。
double ToDegrees(const double radians) noexcept {
  return radians * 180.0 / 3.14159265358979323846;
}

}  // namespace

// GPS処理クラスを初期化します。
// 引数: config は目標地点と照合条件です。
// 戻り値: なし。
GpsProcessor::GpsProcessor(const GpsProcessorConfig config) : config_(config) {}

// 標準のGPS処理設定を返します。
// 引数: なし。
// 戻り値: 目標地点定数と標準しきい値を含む設定です。
GpsProcessorConfig GpsProcessor::DefaultConfig() noexcept {
  return GpsProcessorConfig{
      Coordinate{TARGET_LATITUDE_DEGREES, TARGET_LONGITUDE_DEGREES},
      DEFAULT_DIRECTION_TOLERANCE_DEGREES, DEFAULT_RELIABLE_SPEED_MPS};
}

// GPSサンプルが航法計算に使える測位状態かを確認します。
// 引数: sample は確認対象のGPSサンプルです。
// 戻り値: fix済みならtrueです。
bool GpsProcessor::CheckGpsSample(const GpsSample& sample) const noexcept {
  return sample.has_fix;
}

// 2点間の球面距離を計算します。
// 引数: from は現在地、to は目標地点です。
// 戻り値: メートル単位の距離です。
double GpsProcessor::CalculateDistanceMeters(const Coordinate& from,
                                             const Coordinate& to) const
    noexcept {
  const double from_lat = ToRadians(from.latitude_degrees);
  const double to_lat = ToRadians(to.latitude_degrees);
  const double delta_lat = ToRadians(to.latitude_degrees - from.latitude_degrees);
  const double delta_lon =
      ToRadians(to.longitude_degrees - from.longitude_degrees);
  const double sin_lat = std::sin(delta_lat / 2.0);
  const double sin_lon = std::sin(delta_lon / 2.0);
  const double haversine =
      (sin_lat * sin_lat) +
      (std::cos(from_lat) * std::cos(to_lat) * sin_lon * sin_lon);
  const double central_angle =
      2.0 * std::atan2(std::sqrt(haversine), std::sqrt(1.0 - haversine));
  return EARTH_RADIUS_METERS * central_angle;
}

// 2点間の初期方位角を計算します。
// 引数: from は現在地、to は目標地点です。
// 戻り値: 北を0度とした時計回りの方位角です。
double GpsProcessor::CalculateBearingDegrees(const Coordinate& from,
                                             const Coordinate& to) const
    noexcept {
  const double from_lat = ToRadians(from.latitude_degrees);
  const double to_lat = ToRadians(to.latitude_degrees);
  const double delta_lon =
      ToRadians(to.longitude_degrees - from.longitude_degrees);
  const double x = std::sin(delta_lon) * std::cos(to_lat);
  const double y = (std::cos(from_lat) * std::sin(to_lat)) -
                   (std::sin(from_lat) * std::cos(to_lat) *
                    std::cos(delta_lon));
  return NormalizeDegrees(ToDegrees(std::atan2(x, y)));
}

// GPS相対方位とカメラ相対方向が許容範囲内かを判定します。
// 引数: gps_relative_degrees はGPS由来の相対角、camera_direction_degrees はカメラ由来の相対角です。
// 戻り値: 角度差が許容角以内ならtrueです。
bool GpsProcessor::IsCameraDirectionConsistent(
    const double gps_relative_degrees,
    const double camera_direction_degrees) const noexcept {
  const double difference =
      std::fabs(NormalizeSignedDegrees(gps_relative_degrees -
                                       camera_direction_degrees));
  return difference <= config_.direction_tolerance_degrees;
}

// GPS進行方位と目標方位の角度差を計算します。
// 引数: sample はGPSサンプル、target_bearing_degrees は目標絶対方位です。
// 戻り値: 信頼できる進行方位がある場合だけ相対角を返します。
std::optional<double> GpsProcessor::CalculateHeadingError(
    const GpsSample& sample, const double target_bearing_degrees) const
    noexcept {
  if (!sample.course_degrees.has_value() ||
      sample.speed_meters_per_second < config_.reliable_speed_mps) {
    return std::nullopt;
  }
  return NormalizeSignedDegrees(target_bearing_degrees -
                                sample.course_degrees.value());
}

// GPSとカメラ情報を統合した航法記録を作成します。
// 引数: sample はGPSサンプル、camera_direction_degrees は任意のカメラ相対方向です。
// 戻り値: ログ保存用の航法記録です。
NavigationRecord GpsProcessor::BuildNavigationRecord(
    const GpsSample& sample,
    const std::optional<double> camera_direction_degrees) const {
  if (!CheckGpsSample(sample)) {
    throw GpsException("GPS sample has no valid fix");
  }

  const double target_bearing =
      CalculateBearingDegrees(sample.coordinate, config_.target_coordinate);
  const double target_distance =
      CalculateDistanceMeters(sample.coordinate, config_.target_coordinate);
  const std::optional<double> heading_error =
      CalculateHeadingError(sample, target_bearing);
  const bool gps_heading_reliable = heading_error.has_value();

  DirectionDecision decision = DirectionDecision::kGpsAbsolute;
  bool camera_agrees = false;
  double corrected_direction = target_bearing;

  if (gps_heading_reliable) {
    corrected_direction = heading_error.value();
    decision = DirectionDecision::kGpsRelative;
    if (camera_direction_degrees.has_value()) {
      camera_agrees = IsCameraDirectionConsistent(
          heading_error.value(), camera_direction_degrees.value());
      corrected_direction =
          camera_agrees ? camera_direction_degrees.value() : heading_error.value();
      decision = camera_agrees ? DirectionDecision::kCameraConfirmed
                               : DirectionDecision::kGpsCorrected;
    }
  }

  return NavigationRecord{sample.coordinate,
                          config_.target_coordinate,
                          target_bearing,
                          target_distance,
                          camera_direction_degrees,
                          camera_agrees,
                          corrected_direction,
                          heading_error,
                          gps_heading_reliable,
                          decision};
}

// 現在のGPS処理設定を返します。
// 引数: なし。
// 戻り値: コンストラクタで設定された値です。
const GpsProcessorConfig& GpsProcessor::GetConfig() const noexcept {
  return config_;
}
