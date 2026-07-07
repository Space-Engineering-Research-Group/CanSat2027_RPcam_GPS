// ファイル概要: カメラ処理で共有する型の補助関数を実装するソース
// 作成者コンテキスト: 部活動CanSat向けのRaspberry Pi Zero 2W制御コード
// 日付: YYYY-MM-DD
#include "cameraTypes.hpp"

// 外接矩形の幅を計算します。
// 引数: なし。
// 戻り値: 範囲が有効なら端点を含む幅、無効なら0です。
std::size_t BoundingBox::Width() const noexcept {
  if (!IsValid()) {
    return 0U;
  }
  return max_x - min_x + 1U;
}

// 外接矩形の高さを計算します。
// 引数: なし。
// 戻り値: 範囲が有効なら端点を含む高さ、無効なら0です。
std::size_t BoundingBox::Height() const noexcept {
  if (!IsValid()) {
    return 0U;
  }
  return max_y - min_y + 1U;
}

// 外接矩形の端点が矛盾していないか確認します。
// 引数: なし。
// 戻り値: max_x/max_yがmin_x/min_y以上ならtrueです。
bool BoundingBox::IsValid() const noexcept {
  return min_x <= max_x && min_y <= max_y;
}

// 象限の列挙値をログ向け文字列へ変換します。
// 引数: quadrant は変換対象の象限です。
// 戻り値: kNoneはnone、それ以外はfirstからfourthです。
std::string ToString(const ConeQuadrant quadrant) {
  switch (quadrant) {
    case ConeQuadrant::kFirst:
      return "first";
    case ConeQuadrant::kSecond:
      return "second";
    case ConeQuadrant::kThird:
      return "third";
    case ConeQuadrant::kFourth:
      return "fourth";
    case ConeQuadrant::kNone:
      return "none";
  }
  return "none";
}
