// ファイル概要: GPS処理で共有する型の補助関数を実装するソース
// 作成者コンテキスト: 部活動CanSat向けのRaspberry Pi Zero 2W制御コード
// 日付: YYYY-MM-DD
#include "gpsTypes.hpp"

#include <cmath>

// 角度を0度以上360度未満へ正規化します。
// 引数: angle_degrees は正規化対象の角度です。
// 戻り値: 0度以上360度未満の角度です。
double NormalizeDegrees(const double angle_degrees) noexcept {
  double normalized = std::fmod(angle_degrees, 360.0);
  if (normalized < 0.0) {
    normalized += 360.0;
  }
  return normalized;
}

// 角度を-180度以上180度未満へ正規化します。
// 引数: angle_degrees は正規化対象の角度です。
// 戻り値: -180度以上180度未満の角度です。
double NormalizeSignedDegrees(const double angle_degrees) noexcept {
  double normalized = NormalizeDegrees(angle_degrees);
  if (normalized >= 180.0) {
    normalized -= 360.0;
  }
  return normalized;
}

// 航法判断の列挙値をログ向け文字列へ変換します。
// 引数: decision は変換対象の判断値です。
// 戻り値: gps_absoluteなどの英数字文字列です。
std::string ToString(const DirectionDecision decision) {
  switch (decision) {
    case DirectionDecision::kGpsAbsolute:
      return "gps_absolute";
    case DirectionDecision::kGpsRelative:
      return "gps_relative";
    case DirectionDecision::kCameraConfirmed:
      return "camera_confirmed";
    case DirectionDecision::kGpsCorrected:
      return "gps_corrected";
  }
  return "gps_absolute";
}
