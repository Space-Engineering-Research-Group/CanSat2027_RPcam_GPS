// ファイル概要: NMEAテキストファイルやUARTデバイスをGPS入力として扱う処理を実装するソース
// 作成者コンテキスト: 部活動CanSat向けのRaspberry Pi Zero 2W制御コード
// 日付: YYYY-MM-DD
#include "fileGpsDevice.hpp"

#include <string>

#include "appException.hpp"

// GPS入力デバイスを初期化します。
// 引数: nmea_path はNMEA入力パス、parser はRMC解析器です。
// 戻り値: なし。
FileGpsDevice::FileGpsDevice(std::filesystem::path nmea_path,
                             NmeaParser parser)
    : nmea_path_(std::move(nmea_path)), parser_(parser), input_() {}

// NMEA入力パスが読み取り可能か確認します。
// 引数: なし。
// 戻り値: ファイルやデバイスが存在すればtrueです。
bool FileGpsDevice::IsConnected() const {
  return std::filesystem::exists(nmea_path_);
}

// 次の有効なRMC文を読み取ります。
// 引数: なし。
// 戻り値: 読み取れたGPSサンプル、終端ならnulloptです。
std::optional<GpsSample> FileGpsDevice::ReadSample() {
  if (!IsConnected()) {
    throw GpsException("NMEA input path is not connected");
  }
  if (!input_.is_open()) {
    input_.open(nmea_path_);
    if (!input_) {
      throw GpsException("failed to open NMEA input path");
    }
  }

  std::string line;
  while (std::getline(input_, line)) {
    if (parser_.IsSupportedSentence(line)) {
      return parser_.ParseLine(line);
    }
  }
  return std::nullopt;
}

// 入力パスを返します。
// 引数: なし。
// 戻り値: コンストラクタで設定されたパスです。
const std::filesystem::path& FileGpsDevice::GetNmeaPath() const noexcept {
  return nmea_path_;
}
