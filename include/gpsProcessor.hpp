// ファイル概要: GPS座標から目標地点への距離・方位・カメラ照合を行うクラスを定義するヘッダー
// 作成者コンテキスト: 部活動CanSat向けのRaspberry Pi Zero 2W制御コード
// 日付: YYYY-MM-DD
#ifndef GPS_PROCESSOR_HPP
#define GPS_PROCESSOR_HPP

#include <optional>

#include "gpsTypes.hpp"

// TARGET_LATITUDE_DEGREESは、赤色コーン目標地点の緯度をプログラム内定数として保持します。
constexpr double TARGET_LATITUDE_DEGREES = 33.9609583;

// TARGET_LONGITUDE_DEGREESは、赤色コーン目標地点の経度をプログラム内定数として保持します。
constexpr double TARGET_LONGITUDE_DEGREES = 133.2910361;

// DEFAULT_DIRECTION_TOLERANCE_DEGREESは、カメラ方向とGPS方位を一致とみなす許容角度です。
constexpr double DEFAULT_DIRECTION_TOLERANCE_DEGREES = 15.0;

// DEFAULT_RELIABLE_SPEED_MPSは、GPSの進行方位を信頼する最低速度です。
constexpr double DEFAULT_RELIABLE_SPEED_MPS = 0.7;

// GpsProcessorConfigは、目標地点と照合しきい値を保持します。
// 引数: target_coordinate は赤色コーンの座標、direction_tolerance_degrees は照合許容角です。
// 戻り値: なし。
struct GpsProcessorConfig final {
  Coordinate target_coordinate;
  double direction_tolerance_degrees;
  double reliable_speed_mps;
};

// GpsProcessorは、GPS現在地から目標地点への航法計算とカメラ方向の照合を行います。
// 引数: config は目標地点としきい値です。
// 戻り値: なし。
class GpsProcessor final {
 public:
  // GPS処理クラスを初期化します。
  // 引数: config は目標地点と照合条件です。
  // 戻り値: なし。
  explicit GpsProcessor(GpsProcessorConfig config);

  // 標準のGPS処理設定を返します。
  // 引数: なし。
  // 戻り値: 目標地点定数と標準しきい値を含む設定です。
  [[nodiscard]] static GpsProcessorConfig DefaultConfig() noexcept;

  // GPSサンプルが航法計算に使える測位状態かを確認します。
  // 引数: sample は確認対象のGPSサンプルです。
  // 戻り値: fix済みならtrueです。
  [[nodiscard]] bool CheckGpsSample(const GpsSample& sample) const noexcept;

  // 2点間の球面距離を計算します。
  // 引数: from は現在地、to は目標地点です。
  // 戻り値: メートル単位の距離です。
  [[nodiscard]] double CalculateDistanceMeters(const Coordinate& from,
                                               const Coordinate& to) const
      noexcept;

  // 2点間の初期方位角を計算します。
  // 引数: from は現在地、to は目標地点です。
  // 戻り値: 北を0度とした時計回りの方位角です。
  [[nodiscard]] double CalculateBearingDegrees(const Coordinate& from,
                                               const Coordinate& to) const
      noexcept;

  // GPS相対方位とカメラ相対方向が許容範囲内かを判定します。
  // 引数: gps_relative_degrees はGPS由来の相対角、camera_direction_degrees はカメラ由来の相対角です。
  // 戻り値: 角度差が許容角以内ならtrueです。
  [[nodiscard]] bool IsCameraDirectionConsistent(
      double gps_relative_degrees, double camera_direction_degrees) const
      noexcept;

  // GPS進行方位と目標方位の角度差を計算します。
  // 引数: sample はGPSサンプル、target_bearing_degrees は目標絶対方位です。
  // 戻り値: 信頼できる進行方位がある場合だけ相対角を返します。
  [[nodiscard]] std::optional<double> CalculateHeadingError(
      const GpsSample& sample, double target_bearing_degrees) const noexcept;

  // GPSとカメラ情報を統合した航法記録を作成します。
  // 引数: sample はGPSサンプル、camera_direction_degrees は任意のカメラ相対方向です。
  // 戻り値: ログ保存用の航法記録です。
  [[nodiscard]] NavigationRecord BuildNavigationRecord(
      const GpsSample& sample,
      std::optional<double> camera_direction_degrees) const;

  // 現在のGPS処理設定を返します。
  // 引数: なし。
  // 戻り値: コンストラクタで設定された値です。
  [[nodiscard]] const GpsProcessorConfig& GetConfig() const noexcept;

 private:
  GpsProcessorConfig config_;
};

#endif  // GPS_PROCESSOR_HPP
