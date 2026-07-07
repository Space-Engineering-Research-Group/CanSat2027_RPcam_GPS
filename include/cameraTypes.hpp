// ファイル概要: カメラ処理で共有するYUVフレームと検出結果の型を定義するヘッダー
// 作成者コンテキスト: 部活動CanSat向けのRaspberry Pi Zero 2W制御コード
// 日付: YYYY-MM-DD
#ifndef CAMERA_TYPES_HPP
#define CAMERA_TYPES_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// ConeQuadrantは、画像中心で四分割した領域のどこにコーンがあるかを表します。
// 引数: なし。
// 戻り値: なし。
enum class ConeQuadrant {
  kNone,
  kFirst,
  kSecond,
  kThird,
  kFourth,
};

// Point2Dは、画像上の小数座標を表します。
// 引数: x は横方向座標、y は縦方向座標です。
// 戻り値: なし。
struct Point2D final {
  double x;
  double y;
};

// BoundingBoxは、検出した赤色領域の外接矩形を表します。
// 引数: min_x/min_y/max_x/max_y は画像上の端点座標です。
// 戻り値: なし。
struct BoundingBox final {
  std::size_t min_x;
  std::size_t min_y;
  std::size_t max_x;
  std::size_t max_y;

  // 外接矩形の幅を返します。
  // 引数: なし。
  // 戻り値: ピクセル単位の幅です。
  [[nodiscard]] std::size_t Width() const noexcept;

  // 外接矩形の高さを返します。
  // 引数: なし。
  // 戻り値: ピクセル単位の高さです。
  [[nodiscard]] std::size_t Height() const noexcept;

  // 外接矩形として有効な範囲かを返します。
  // 引数: なし。
  // 戻り値: maxがmin以上ならtrueです。
  [[nodiscard]] bool IsValid() const noexcept;
};

// YuvFrameは、YUV420 planar形式の画像を保持します。
// 引数: width/height は画像サイズ、各planeはY/U/V成分です。
// 戻り値: なし。
struct YuvFrame final {
  std::size_t width;
  std::size_t height;
  std::vector<std::uint8_t> y_plane;
  std::vector<std::uint8_t> u_plane;
  std::vector<std::uint8_t> v_plane;
};

// ConeDetectionは、赤色コーン検出の結果をまとめます。
// 引数: detected は検出可否、cone_center は重心、direction_degrees は機体正面からの相対角です。
// 戻り値: なし。
struct ConeDetection final {
  bool detected;
  ConeQuadrant quadrant;
  Point2D cone_center;
  BoundingBox bounding_box;
  double direction_degrees;
  bool is_best_position;
  std::size_t red_pixel_count;
};

// 象限の列挙値をログ向けの文字列に変換します。
// 引数: quadrant は変換対象の象限です。
// 戻り値: 英数字の象限名です。
[[nodiscard]] std::string ToString(ConeQuadrant quadrant);

#endif  // CAMERA_TYPES_HPP
