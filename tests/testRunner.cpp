// ファイル概要: Catch2でCanSatカメラ処理・GPS処理・ログ処理の単体テストを実行するソース
// 作成者コンテキスト: 部活動CanSat向けのRaspberry Pi Zero 2W制御コード
// 日付: YYYY-MM-DD
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include "appException.hpp"
#include "cameraProcessor.hpp"
#include "fileCameraDevice.hpp"
#include "fileGpsDevice.hpp"
#include "gpsProcessor.hpp"
#include "logSender.hpp"
#include "nmeaParser.hpp"
#include "rpicamCameraDevice.hpp"

namespace {

// テスト用のYUV420フレームを作成します。
// 引数: width/height は画像サイズです。
// 戻り値: 赤色画素を含まない中立色フレームです。
YuvFrame MakeNeutralFrame(const std::size_t width, const std::size_t height) {
  return YuvFrame{width, height, std::vector<std::uint8_t>(width * height, 90U),
                  std::vector<std::uint8_t>((width / 2U) * (height / 2U), 128U),
                  std::vector<std::uint8_t>((width / 2U) * (height / 2U), 128U)};
}

// テスト用フレームに赤色矩形を描画します。
// 引数: frame は描画対象、min_x/min_y/max_x/max_y は矩形範囲です。
// 戻り値: なし。
void PaintRedBlock(YuvFrame* const frame, const std::size_t min_x,
                   const std::size_t min_y, const std::size_t max_x,
                   const std::size_t max_y) {
  for (std::size_t y = min_y; y <= max_y; ++y) {
    for (std::size_t x = min_x; x <= max_x; ++x) {
      frame->y_plane[(y * frame->width) + x] = 110U;
      const std::size_t uv_index =
          ((y / 2U) * (frame->width / 2U)) + (x / 2U);
      frame->u_plane[uv_index] = 90U;
      frame->v_plane[uv_index] = 210U;
    }
  }
}

// テスト用ファイルへバイト列を書き込みます。
// 引数: path は出力先、bytes は書き込むバイト列です。
// 戻り値: なし。
void WriteBytes(const std::filesystem::path& path,
                const std::vector<std::uint8_t>& bytes) {
  std::ofstream output(path, std::ios::binary);
  output.write(reinterpret_cast<const char*>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
}

// テスト用ファイルへ文字列を書き込みます。
// 引数: path は出力先、text は書き込む文字列です。
// 戻り値: なし。
void WriteText(const std::filesystem::path& path, const std::string& text) {
  std::ofstream output(path);
  output << text;
}

}  // namespace

// 例外クラスがメッセージを保持することを確認します。
// 引数: なし。
// 戻り値: なし。
TEST_CASE("exceptions keep message", "[exception]") {
  REQUIRE(std::string(CameraException("camera").what()) == "camera");
  REQUIRE(std::string(GpsException("gps").what()) == "gps");
  REQUIRE(std::string(LogException("log").what()) == "log");
  REQUIRE(std::string(ArgumentException("arg").what()) == "arg");
}

// 外接矩形の幅・高さ・有効性を確認します。
// 引数: なし。
// 戻り値: なし。
TEST_CASE("bounding box helpers", "[camera]") {
  const BoundingBox valid{2U, 3U, 5U, 8U};
  REQUIRE(valid.IsValid());
  REQUIRE(valid.Width() == 4U);
  REQUIRE(valid.Height() == 6U);

  const BoundingBox invalid{5U, 1U, 2U, 1U};
  REQUIRE(!invalid.IsValid());
  REQUIRE(invalid.Width() == 0U);
  REQUIRE(ToString(ConeQuadrant::kFirst) == "first");
}

// カメラ設定とフレーム形状チェックを確認します。
// 引数: なし。
// 戻り値: なし。
TEST_CASE("camera configuration and shape", "[camera]") {
  const CameraProcessor processor(CameraProcessor::DefaultConfig());
  REQUIRE(processor.GetConfig().min_red_pixels > 0U);
  REQUIRE(processor.CheckFrameShape(MakeNeutralFrame(8U, 6U)));

  YuvFrame invalid = MakeNeutralFrame(8U, 6U);
  invalid.v_plane.pop_back();
  REQUIRE(!processor.CheckFrameShape(invalid));
}

// YUV値の赤色判定を確認します。
// 引数: なし。
// 戻り値: なし。
TEST_CASE("camera red pixel check", "[camera]") {
  const CameraProcessor processor(CameraProcessor::DefaultConfig());
  REQUIRE(processor.IsRedPixel(100U, 90U, 210U));
  REQUIRE(!processor.IsRedPixel(100U, 170U, 130U));
}

// 画像座標から方向角への変換を確認します。
// 引数: なし。
// 戻り値: なし。
TEST_CASE("camera direction calculation", "[camera]") {
  const CameraProcessor processor(CameraProcessor::DefaultConfig());
  REQUIRE(processor.CalculateDirectionDegrees(160.0, 320U) ==
          Catch::Approx(0.0).margin(0.001));
  REQUIRE(processor.CalculateDirectionDegrees(240.0, 320U) > 0.0);
  REQUIRE_THROWS_AS(processor.CalculateDirectionDegrees(0.0, 0U),
                    CameraException);
}

// 画像四分割の象限判定とベストポジション判定を確認します。
// 引数: なし。
// 戻り値: なし。
TEST_CASE("camera quadrant and best position", "[camera]") {
  const CameraProcessor processor(CameraProcessor::DefaultConfig());
  REQUIRE(processor.DetermineQuadrant(Point2D{6.0, 1.0}, 10U, 10U) ==
          ConeQuadrant::kFirst);
  REQUIRE(processor.DetermineQuadrant(Point2D{1.0, 1.0}, 10U, 10U) ==
          ConeQuadrant::kSecond);
  REQUIRE(processor.DetermineQuadrant(Point2D{1.0, 8.0}, 10U, 10U) ==
          ConeQuadrant::kThird);
  REQUIRE(processor.DetermineQuadrant(Point2D{8.0, 8.0}, 10U, 10U) ==
          ConeQuadrant::kFourth);
  REQUIRE(processor.IsBestPosition(Point2D{5.0, 5.0}, 10U, 10U));
}

// 赤色コーンの検出と未検出を確認します。
// 引数: なし。
// 戻り値: なし。
TEST_CASE("camera cone detection", "[camera]") {
  CameraDetectionConfig config = CameraProcessor::DefaultConfig();
  config.min_red_pixels = 4U;
  const CameraProcessor processor(config);
  YuvFrame frame = MakeNeutralFrame(16U, 12U);
  PaintRedBlock(&frame, 10U, 2U, 13U, 5U);

  const ConeDetection detection = processor.DetectCone(frame);
  REQUIRE(detection.detected);
  REQUIRE(detection.quadrant == ConeQuadrant::kFirst);
  REQUIRE(detection.bounding_box.Width() == 4U);
  REQUIRE(detection.red_pixel_count >= 16U);

  const ConeDetection empty_detection =
      processor.DetectCone(MakeNeutralFrame(16U, 12U));
  REQUIRE(!empty_detection.detected);
}

// ファイルカメラデバイスの接続確認と読み込みを確認します。
// 引数: なし。
// 戻り値: なし。
TEST_CASE("file camera device", "[camera]") {
  const std::filesystem::path path =
      std::filesystem::temp_directory_path() / "cansatTestFrame.yuv";
  const YuvFrame frame = MakeNeutralFrame(4U, 4U);
  std::vector<std::uint8_t> bytes;
  bytes.insert(bytes.end(), frame.y_plane.begin(), frame.y_plane.end());
  bytes.insert(bytes.end(), frame.u_plane.begin(), frame.u_plane.end());
  bytes.insert(bytes.end(), frame.v_plane.begin(), frame.v_plane.end());
  WriteBytes(path, bytes);

  const FileCameraDevice device(path, 4U, 4U);
  REQUIRE(device.GetFramePath() == path);
  REQUIRE(device.IsConnected());
  const YuvFrame loaded = device.CaptureFrame();
  REQUIRE(loaded.y_plane.size() == frame.y_plane.size());
  std::filesystem::remove(path);
}

// rpicamカメラデバイスのコマンド生成と失敗経路を確認します。
// 引数: なし。
// 戻り値: なし。
TEST_CASE("rpicam camera device", "[camera]") {
  const std::filesystem::path path =
      std::filesystem::temp_directory_path() / "cansatRpicamFrame.yuv";
  const RpicamCameraDevice device(path, 320U, 240U, "/bin/false",
                                  "/bin/false");
  REQUIRE(device.GetOutputFramePath() == path);
  REQUIRE(!device.IsConnected());
  REQUIRE(device.BuildCaptureCommand().find("--codec yuv420") !=
          std::string::npos);
  REQUIRE_THROWS_AS(device.CaptureFrame(), CameraException);
}

// GPS角度正規化と文字列化を確認します。
// 引数: なし。
// 戻り値: なし。
TEST_CASE("GPS type helpers", "[gps]") {
  REQUIRE(NormalizeDegrees(-10.0) == Catch::Approx(350.0).margin(0.001));
  REQUIRE(NormalizeSignedDegrees(350.0) ==
          Catch::Approx(-10.0).margin(0.001));
  REQUIRE(ToString(DirectionDecision::kGpsCorrected) == "gps_corrected");
}

// NMEAパーサの対応文判定・チェックサム・座標変換・解析を確認します。
// 引数: なし。
// 戻り値: なし。
TEST_CASE("NMEA parser", "[gps]") {
  const NmeaParser parser;
  const std::string line =
      "$GPRMC,092751.000,A,3540.1234,N,13945.5678,E,1.5,84.4,030626,,,A";
  REQUIRE(parser.IsSupportedSentence(line));
  REQUIRE(parser.ValidateChecksum(line));
  REQUIRE(parser.ParseCoordinateDegrees("3540.1234", "N") ==
          Catch::Approx(35.668723).margin(0.000001));

  const GpsSample sample = parser.ParseLine(line);
  REQUIRE(sample.has_fix);
  REQUIRE(sample.speed_meters_per_second ==
          Catch::Approx(0.771666).margin(0.000001));
  REQUIRE(sample.course_degrees.has_value());
  REQUIRE(sample.course_degrees.value() == Catch::Approx(84.4).margin(0.001));
  REQUIRE_THROWS_AS(parser.ParseLine("$GPVTG,1,T"), GpsException);
}

// GPS距離・方位・進行方位信頼性を確認します。
// 引数: なし。
// 戻り値: なし。
TEST_CASE("GPS processor calculations", "[gps]") {
  const GpsProcessor processor(GpsProcessor::DefaultConfig());
  REQUIRE(processor.GetConfig().reliable_speed_mps > 0.0);
  const Coordinate origin{35.0, 139.0};
  const Coordinate north{35.001, 139.0};
  REQUIRE(processor.CalculateBearingDegrees(origin, north) ==
          Catch::Approx(0.0).margin(0.01));
  REQUIRE(processor.CalculateDistanceMeters(origin, north) > 100.0);

  const GpsSample sample{origin, 1.0, 10.0, true, ""};
  const std::optional<double> heading_error =
      processor.CalculateHeadingError(sample, 40.0);
  REQUIRE(heading_error.has_value());
  REQUIRE(heading_error.value() == Catch::Approx(30.0).margin(0.001));
}

// GPSとカメラ方向の照合と補正を確認します。
// 引数: なし。
// 戻り値: なし。
TEST_CASE("GPS processor navigation record", "[gps]") {
  const GpsProcessor processor(GpsProcessorConfig{Coordinate{35.001, 139.0},
                                                  10.0, 0.5});
  const GpsSample sample{Coordinate{35.0, 139.0}, 1.0, 0.0, true, ""};
  REQUIRE(processor.CheckGpsSample(sample));
  REQUIRE(processor.IsCameraDirectionConsistent(5.0, 7.0));

  const NavigationRecord confirmed =
      processor.BuildNavigationRecord(sample, 2.0);
  REQUIRE(confirmed.camera_agrees_with_gps);
  REQUIRE(confirmed.decision == DirectionDecision::kCameraConfirmed);

  const NavigationRecord corrected =
      processor.BuildNavigationRecord(sample, 60.0);
  REQUIRE(!corrected.camera_agrees_with_gps);
  REQUIRE(corrected.decision == DirectionDecision::kGpsCorrected);
  REQUIRE_THROWS_AS(
      processor.BuildNavigationRecord(
          GpsSample{Coordinate{0.0, 0.0}, 0.0, std::nullopt, false, ""}, 0.0),
      GpsException);
}

// ファイルGPSデバイスの接続確認とRMC読み取りを確認します。
// 引数: なし。
// 戻り値: なし。
TEST_CASE("file GPS device", "[gps]") {
  const std::filesystem::path path =
      std::filesystem::temp_directory_path() / "cansatTestGps.nmea";
  WriteText(path,
            "$GPVTG,1,T\n"
            "$GPRMC,092751.000,A,3540.1234,N,13945.5678,E,1.5,84.4,030626,,,A\n");

  FileGpsDevice device(path, NmeaParser{});
  REQUIRE(device.GetNmeaPath() == path);
  REQUIRE(device.IsConnected());
  const std::optional<GpsSample> sample = device.ReadSample();
  REQUIRE(sample.has_value());
  REQUIRE(sample.value().has_fix);
  std::filesystem::remove(path);
}

// ログ行生成・追記・フェーズ送信を確認します。
// 引数: なし。
// 戻り値: なし。
TEST_CASE("log sender", "[log]") {
  const std::filesystem::path log_dir =
      std::filesystem::temp_directory_path() / "cansatLogTest";
  const LogSender sender(
      LogPaths{log_dir / "local.csv", log_dir / "sent.csv"});
  REQUIRE(sender.GetPaths().local_log_path == (log_dir / "local.csv"));

  const ConeDetection detection{true,
                                ConeQuadrant::kFirst,
                                Point2D{10.0, 12.0},
                                BoundingBox{9U, 11U, 12U, 14U},
                                3.5,
                                false,
                                16U};
  const NavigationRecord record{Coordinate{35.0, 139.0},
                                Coordinate{35.001, 139.0},
                                0.0,
                                111.0,
                                2.0,
                                true,
                                2.0,
                                0.0,
                                true,
                                DirectionDecision::kCameraConfirmed};
  REQUIRE(sender.BuildCameraLogLine("phase", detection).find("camera") == 0U);
  REQUIRE(sender.BuildNavigationLogLine("phase", record).find("gps") == 0U);
  sender.AppendCameraRecord("phase", detection);
  sender.AppendNavigationRecord("phase", record);
  sender.SendPhaseLog("phase");
  REQUIRE(std::filesystem::is_regular_file(log_dir / "sent.csv"));
  std::filesystem::remove_all(log_dir);
}
