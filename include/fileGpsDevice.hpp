// ファイル概要: NMEAテキストファイルやUARTデバイスをGPS入力として扱うクラスを定義するヘッダー
// 作成者コンテキスト: 部活動CanSat向けのRaspberry Pi Zero 2W制御コード
// 日付: YYYY-MM-DD
#ifndef FILE_GPS_DEVICE_HPP
#define FILE_GPS_DEVICE_HPP

#include <filesystem>
#include <fstream>
#include <optional>

#include "gpsTypes.hpp"
#include "nmeaParser.hpp"

// FileGpsDeviceは、NMEA行を出力するファイルや/dev/serial0をGPS入力として読み込みます。
// 引数: nmea_path は入力パス、parser はRMC文を解析するNmeaParserです。
// 戻り値: なし。
class FileGpsDevice final {
 public:
  // GPS入力デバイスを初期化します。
  // 引数: nmea_path はNMEA入力パス、parser はRMC解析器です。
  // 戻り値: なし。
  FileGpsDevice(std::filesystem::path nmea_path, NmeaParser parser);

  // NMEA入力パスが読み取り可能か確認します。
  // 引数: なし。
  // 戻り値: ファイルやデバイスが存在すればtrueです。
  [[nodiscard]] bool IsConnected() const;

  // 次の有効なRMC文を読み取ります。
  // 引数: なし。
  // 戻り値: 読み取れたGPSサンプル、終端ならnulloptです。
  [[nodiscard]] std::optional<GpsSample> ReadSample();

  // 入力パスを返します。
  // 引数: なし。
  // 戻り値: コンストラクタで設定されたパスです。
  [[nodiscard]] const std::filesystem::path& GetNmeaPath() const noexcept;

 private:
  std::filesystem::path nmea_path_;
  NmeaParser parser_;
  std::ifstream input_;
};

#endif  // FILE_GPS_DEVICE_HPP
