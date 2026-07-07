// ファイル概要: カメラ処理とGPS処理を実行するCanSat用コマンドラインエントリポイント
// 作成者コンテキスト: 部活動CanSat向けのRaspberry Pi Zero 2W制御コード
// 日付: YYYY-MM-DD
#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <thread>

#include "appException.hpp"
#include "cameraProcessor.hpp"
#include "fileCameraDevice.hpp"
#include "fileGpsDevice.hpp"
#include "gpsProcessor.hpp"
#include "logSender.hpp"
#include "nmeaParser.hpp"
#include "rpicamCameraDevice.hpp"

namespace {

// ProgramOptionsは、コマンドライン引数から読み取った実行設定を保持します。
// 引数: 各メンバーはモード、入力パス、画像サイズ、ログ出力先です。
// 戻り値: なし。
struct ProgramOptions final {
  std::string mode;
  std::string camera_source;
  std::filesystem::path yuv_frame_path;
  std::filesystem::path nmea_path;
  std::filesystem::path log_dir;
  std::string phase_name;
  std::size_t width;
  std::size_t height;
  std::size_t cycles;
  std::size_t interval_ms;
  std::optional<double> camera_direction_degrees;
};

// 文字列をsize_tへ変換します。
// 引数: value は数値文字列、name は例外メッセージ用の項目名です。
// 戻り値: 変換後のsize_t値です。
std::size_t ParseSize(const std::string& value, const std::string& name) {
  std::size_t parsed_length = 0U;
  const unsigned long parsed = std::stoul(value, &parsed_length, 10);
  if (parsed_length != value.size()) {
    throw ArgumentException(name + " must be an unsigned integer");
  }
  return static_cast<std::size_t>(parsed);
}

// 文字列をdoubleへ変換します。
// 引数: value は数値文字列、name は例外メッセージ用の項目名です。
// 戻り値: 変換後のdouble値です。
double ParseDoubleArgument(const std::string& value, const std::string& name) {
  std::size_t parsed_length = 0U;
  const double parsed = std::stod(value, &parsed_length);
  if (parsed_length != value.size()) {
    throw ArgumentException(name + " must be a number");
  }
  return parsed;
}

// コマンドライン引数を解析します。
// 引数: argc/argv はmain関数から渡される引数です。
// 戻り値: 実行設定です。
ProgramOptions ParseArguments(const int argc, char* argv[]) {
  ProgramOptions options{"demo", "file", "", "", "logs", "phase", 320U, 240U,
                         1U,     0U,     std::nullopt};
  for (int index = 1; index < argc; ++index) {
    const std::string key(argv[index]);
    if (key == "--mode" && (index + 1) < argc) {
      options.mode = argv[++index];
    } else if (key == "--camera-source" && (index + 1) < argc) {
      options.camera_source = argv[++index];
    } else if (key == "--yuv-frame" && (index + 1) < argc) {
      options.yuv_frame_path = argv[++index];
    } else if (key == "--nmea-file" && (index + 1) < argc) {
      options.nmea_path = argv[++index];
    } else if (key == "--log-dir" && (index + 1) < argc) {
      options.log_dir = argv[++index];
    } else if (key == "--phase" && (index + 1) < argc) {
      options.phase_name = argv[++index];
    } else if (key == "--width" && (index + 1) < argc) {
      options.width = ParseSize(argv[++index], "--width");
    } else if (key == "--height" && (index + 1) < argc) {
      options.height = ParseSize(argv[++index], "--height");
    } else if (key == "--cycles" && (index + 1) < argc) {
      options.cycles = ParseSize(argv[++index], "--cycles");
    } else if (key == "--interval-ms" && (index + 1) < argc) {
      options.interval_ms = ParseSize(argv[++index], "--interval-ms");
    } else if (key == "--camera-direction" && (index + 1) < argc) {
      options.camera_direction_degrees =
          ParseDoubleArgument(argv[++index], "--camera-direction");
    } else {
      throw ArgumentException("unknown or incomplete argument: " + key);
    }
  }
  return options;
}

// ログ送信クラスを作成します。
// 引数: log_dir はログ出力ディレクトリです。
// 戻り値: ローカルログと送信済みログを扱うLogSenderです。
LogSender MakeLogSender(const std::filesystem::path& log_dir) {
  return LogSender(LogPaths{log_dir / "cansatLocal.csv",
                            log_dir / "cansatSent.csv"});
}

// カメラ処理を1回実行します。
// 引数: options は実行設定、log_sender はログ出力先です。
// 戻り値: 検出できた場合はカメラ相対方向角です。
std::optional<double> RunCameraOnce(const ProgramOptions& options,
                                    const LogSender& log_sender) {
  const CameraProcessor processor(CameraProcessor::DefaultConfig());
  ConeDetection detection{};
  if (options.camera_source == "rpicam") {
    const std::filesystem::path output_path =
        options.yuv_frame_path.empty()
            ? std::filesystem::path("/tmp/cansatCone.yuv")
            : options.yuv_frame_path;
    RpicamCameraDevice camera(output_path, options.width, options.height,
                              "rpicam-vid", "rpicam-hello");
    if (!camera.IsConnected()) {
      throw CameraException("Raspberry Pi camera is not connected");
    }
    detection = processor.DetectCone(camera.CaptureFrame());
  } else if (options.camera_source == "file") {
    FileCameraDevice camera(options.yuv_frame_path, options.width,
                            options.height);
    if (!camera.IsConnected()) {
      throw CameraException("camera frame source is not connected");
    }
    detection = processor.DetectCone(camera.CaptureFrame());
  } else {
    throw ArgumentException("--camera-source must be file or rpicam");
  }
  std::cout << "detected: " << detection.detected
            << " quadrant: " << ToString(detection.quadrant)
            << " direction: " << detection.direction_degrees << std::endl;
  log_sender.AppendCameraRecord(options.phase_name, detection);
  return detection.detected ? std::optional<double>(detection.direction_degrees)
                            : std::nullopt;
}

// GPS処理を1件実行します。
// 引数: options は実行設定、log_sender はログ出力先、gps は継続読み取り中のGPS入力、camera_direction_degrees は任意のカメラ方向角です。
// 戻り値: なし。
void RunGpsOnce(const ProgramOptions& options, const LogSender& log_sender,
                FileGpsDevice& gps,
                const std::optional<double> camera_direction_degrees) {
///////変更
  std::optional<GpsSample> sample;
  // has_fixがtrueになるまで読み続ける
  while (true) {
    sample = gps.ReadSample();
    if (!sample.has_value()) {
      throw GpsException("GPS source does not contain an RMC sentence");
    }
    if (sample->has_fix) {
      break;
    }
  }
///////
  //const std::optional<GpsSample> sample = gps.ReadSample();
  //if (!sample.has_value()) {
    //throw GpsException("GPS source does not contain an RMC sentence");
  //}
  const GpsProcessor processor(GpsProcessor::DefaultConfig());
  const NavigationRecord record =
      processor.BuildNavigationRecord(sample.value(), camera_direction_degrees);
  std::cout << "distance: " << record.target_distance_meters
            << " bearing: " << record.target_bearing_degrees
            << " consistent: " << record.camera_direction_degrees.value_or(-1.0) << std::endl;
  log_sender.AppendNavigationRecord(options.phase_name, record);
}

}  // namespace

// CanSatのカメラ・GPS処理を実行します。
// 引数: argc/argv はコマンドライン引数です。
// 戻り値: 正常終了は0、異常終了は1です。
int main(int argc, char* argv[]) {
  try {
    const ProgramOptions options = ParseArguments(argc, argv);
    const LogSender log_sender = MakeLogSender(options.log_dir);
    std::optional<double> camera_direction = options.camera_direction_degrees;
    std::unique_ptr<FileGpsDevice> gps_device;
    if (options.mode == "gps" || options.mode == "demo") {
      gps_device = std::make_unique<FileGpsDevice>(options.nmea_path,
                                                   NmeaParser{});
      if (!gps_device->IsConnected()) {
        throw GpsException("GPS source is not connected");
      }
    }

    for (std::size_t cycle = 0U;
         options.cycles == 0U || cycle < options.cycles; ++cycle) {
      if (options.mode == "camera" || options.mode == "demo") {
        camera_direction = RunCameraOnce(options, log_sender);
      }
      if (options.mode == "gps" || options.mode == "demo") {
        RunGpsOnce(options, log_sender, *gps_device, camera_direction);
      }
      if (options.interval_ms > 0U &&
          (options.cycles == 0U || (cycle + 1U) < options.cycles)) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(options.interval_ms));
      }
    }
    log_sender.SendPhaseLog(options.phase_name);
    std::cout << "completed phase: " << options.phase_name << std::endl;
    return 0;
  } catch (const AppException& exception) {
    std::cerr << "error: " << exception.what() << std::endl;
  } catch (const std::exception& exception) {
    std::cerr << "unexpected error: " << exception.what() << std::endl;
  }
  return 1;
}
