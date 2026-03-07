# Shepherd HW
# ESP32 GPS + Accelerometer Data Logger / Uploader

A data acquisition system using an ESP32 that records accelerometer data from an MPU6050 and GPS position data, stores it locally using SPIFFS, and uploads it to a remote server.

## Features

* 10 Hz accelerometer sampling from MPU6050
* GPS position logging
* Local storage using SPIFFS
* Batch upload to server via HTTP
* Binary data logging for efficiency
* FIFO-based IMU reading

## Hardware

* ESP32
* MPU6050 IMU
* GPS module (UART)
* WiFi connection

## System Architecture

1. MPU6050 writes acceleration samples to FIFO
2. ESP32 reads FIFO and stores binary data
3. GPS module provides location and timestamp
4. Both datasets are stored in SPIFFS
5. Data is uploaded to a server periodically

## Data Storage

Two binary files are used:

accel.bin
Contains raw accelerometer samples.

gps.bin
Contains timestamped GPS samples.

### GPS sample structure

```
struct GpsSample{
  uint32_t epoch;
  int32_t lat;
  int32_t lon;
  int32_t alt;
};
```

Latitude and longitude are stored as degrees × 1e7.

Altitude is stored as meters × 100.

## WiFi Configuration

Edit the firmware file:

```
const char* ssid = "Hotspot Name";
const char* password = "Hotspot Password";
```

## Server Endpoint

The ESP uploads binary chunks to:

```
http://<SERVER_IP>:8080/api/device/data
```

Headers include:

* Content-Type
* Sample-Data-Type
* Sample-File-Size
* Sample-Offset
* Device-UID
* X-API-Key

## Upload Interval

Arbitrary upload interval:

```
30 seconds
```

Configured in the firmware.

## Building

1. Install Arduino IDE
2. Install ESP32 board support
3. Install required libraries
4. Upload the code in esp-gps-accel-logger.ino to ESP32

## Future Improvements

* FIFO overflow notification
* Optimizing memory efficiency of the code
