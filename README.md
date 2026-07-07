# CanSat Camera and GPS Processing

## 1. Project Overview

This project implements C++17 camera and GPS processing for a CanSat running on Raspberry Pi Zero 2W with Raspberry Pi OS Bookworm. It detects a red cone from YUV420 camera frames, records the cone quadrant and relative direction angle, parses GPS RMC data, calculates bearing and distance to a fixed target coordinate, checks camera/GPS direction consistency, and records phase logs.

Target platform: Raspberry Pi Zero 2W, Raspberry Pi OS Bookworm 64-bit or 32-bit, ARMv8-A; development and offline tests also run on macOS/Linux with a C++17 compiler.

## 2. Requirements

- Runtime: C++17 standard library or newer
- Compiler: GCC 12+ or Clang 15+
- Build tool: GNU Make 4.3+
- Test framework: Catch2 3.0+
- Camera hardware: Raspberry Pi Camera Module V2.1 connected through the CSI camera connector
- GPS hardware: UART GPS module that outputs NMEA RMC sentences at 9600 bps or higher
- OS packages on Raspberry Pi OS Bookworm:
  - `build-essential` 12+
  - `make` 4.3+
  - `cmake` 3.25+
  - `git` 2.39+
  - `rpicam-apps` 1.2+
  - `raspi-config`

## 3. Installation

Raspberry Pi OS Bookworm:

```sh
sudo apt update
sudo apt install -y build-essential make cmake git rpicam-apps
sudo raspi-config nonint do_camera 0
sudo raspi-config nonint do_serial_hw 0
sudo raspi-config nonint do_serial_cons 1
sudo reboot
```

Install Catch2 v3 on Raspberry Pi OS Bookworm:

```sh
git clone --branch v3.5.4 --depth 1 https://github.com/catchorg/Catch2.git /tmp/Catch2
cmake -S /tmp/Catch2 -B /tmp/Catch2/build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build /tmp/Catch2/build --parallel 2
sudo cmake --install /tmp/Catch2/build
sudo ldconfig
```

macOS for offline build/test:

```sh
xcode-select --install
brew install catch2
```

Build the project:

```sh
make clean
make
```

Hardware setup:

- Connect Raspberry Pi Camera V2.1 to the CSI connector and verify it with `rpicam-hello --list-cameras`.
- Connect the GPS module TX pin to Raspberry Pi GPIO15/RXD, GPS RX to GPIO14/TXD if needed, 3.3 V to 3V3, and GND to GND.
- Configure UART before reading GPS logs:

```sh
stty -F /dev/serial0 9600 raw -echo
```

## 4. Usage

Capture a YUV420 frame on Raspberry Pi, then run camera processing:

```sh
rpicam-vid --codec yuv420 --width 320 --height 240 --frames 1 --timeout 1000 --output /tmp/cone.yuv
./build/cansatController --mode camera --yuv-frame /tmp/cone.yuv --width 320 --height 240 --phase camera_phase --log-dir logs
```

Run camera processing by letting the program start `rpicam-vid`:

```sh
./build/cansatController --mode camera --camera-source rpicam --yuv-frame /tmp/cone.yuv --width 320 --height 240 --cycles 10 --interval-ms 1000 --phase camera_phase --log-dir logs
```

Run GPS processing from UART:

```sh
./build/cansatController --mode gps --nmea-file /dev/serial0 --camera-direction 0.0 --cycles 0 --interval-ms 1000 --phase gps_phase --log-dir logs
```

Run integrated processing with a pre-captured YUV frame and GPS NMEA file:

```sh
./build/cansatController --mode demo --yuv-frame /tmp/cone.yuv --nmea-file ./sample.nmea --width 320 --height 240 --phase approach --log-dir logs
```

Expected output:

```text
completed phase: approach
```

Logs are written to `logs/cansatLocal.csv` and copied to `logs/cansatSent.csv` at phase end. Update `TARGET_LATITUDE_DEGREES` and `TARGET_LONGITUDE_DEGREES` in `include/gpsProcessor.hpp` to the real cone coordinate before flight. `--cycles 0` runs continuously until the process is stopped; use a positive value for bench tests.

## 5. Running Tests

Run the unit test suite:

```sh
make test
```

Run with a coverage report on Linux when `gcov` is available:

```sh
make clean
make CXXFLAGS="-std=c++17 -Wall -Wextra -Wpedantic -O0 --coverage" test
gcov -o build src/*.cpp tests/*.cpp
```

## 6. Project Structure

```text
.
├── Makefile                         # C++17 build and test targets
├── README.md                        # Setup, usage, structure, and function list
├── include/
│   ├── appException.hpp             # Application-specific exception classes
│   ├── cameraProcessor.hpp          # YUV red cone detection API
│   ├── cameraTypes.hpp              # Camera frame and detection result types
│   ├── fileCameraDevice.hpp         # YUV420 file-backed camera input
│   ├── fileGpsDevice.hpp            # NMEA file/UART GPS input
│   ├── gpsProcessor.hpp             # GPS navigation and camera consistency API
│   ├── gpsTypes.hpp                 # GPS coordinate and navigation result types
│   ├── logSender.hpp                # CSV logging and phase-end send API
│   ├── nmeaParser.hpp               # NMEA RMC parser API
│   └── rpicamCameraDevice.hpp       # rpicam-backed camera input
├── src/
│   ├── appException.cpp             # Exception implementations
│   ├── cameraProcessor.cpp          # Red cone detection implementation
│   ├── cameraTypes.cpp              # Camera helper implementations
│   ├── fileCameraDevice.cpp         # YUV420 reader implementation
│   ├── fileGpsDevice.cpp            # NMEA input implementation
│   ├── gpsProcessor.cpp             # Distance, bearing, and consistency implementation
│   ├── gpsTypes.cpp                 # GPS helper implementations
│   ├── logSender.cpp                # Logging implementation
│   ├── main.cpp                     # CLI entry point
│   ├── nmeaParser.cpp               # NMEA parser implementation
│   └── rpicamCameraDevice.cpp       # rpicam camera implementation
└── tests/
    └── testRunner.cpp               # Catch2 unit tests
```

Function/class list:

- `AppException`, `CameraException`, `GpsException`, `LogException`, `ArgumentException`
- `BoundingBox::Width`, `BoundingBox::Height`, `BoundingBox::IsValid`, `ToString(ConeQuadrant)`
- `CameraProcessor::DefaultConfig`, `CheckFrameShape`, `IsRedPixel`, `CalculateDirectionDegrees`, `DetermineQuadrant`, `IsBestPosition`, `DetectCone`, `GetConfig`
- `FileCameraDevice::IsConnected`, `CaptureFrame`, `GetFramePath`
- `RpicamCameraDevice::IsConnected`, `CaptureFrame`, `BuildCaptureCommand`, `GetOutputFramePath`
- `NormalizeDegrees`, `NormalizeSignedDegrees`, `ToString(DirectionDecision)`
- `GpsProcessor::DefaultConfig`, `CheckGpsSample`, `CalculateDistanceMeters`, `CalculateBearingDegrees`, `IsCameraDirectionConsistent`, `CalculateHeadingError`, `BuildNavigationRecord`, `GetConfig`
- `NmeaParser::IsSupportedSentence`, `ValidateChecksum`, `ParseCoordinateDegrees`, `ParseLine`
- `FileGpsDevice::IsConnected`, `ReadSample`, `GetNmeaPath`
- `LogSender::BuildCameraLogLine`, `BuildNavigationLogLine`, `AppendCameraRecord`, `AppendNavigationRecord`, `SendPhaseLog`, `GetPaths`

## 7. License

Proprietary / Internal use only.
