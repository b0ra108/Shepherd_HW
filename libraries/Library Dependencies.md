# Library Dependencies

This project uses several external libraries to interact with hardware components and network services on the ESP32. Below is a brief explanation of each library and its role in the system.

---

## Wire.h

**Purpose:**
Provides I²C communication support.

**Role in the project:**
The MPU6050 IMU communicates with the ESP32 over the I²C bus. The `Wire` library handles low-level I²C communication between the ESP32 and the sensor.

**Used for:**

* Initializing the I²C bus
* Sending and receiving data to/from the MPU6050

Example usage:

```
Wire.begin();
```

---

## I2Cdev.h

**Purpose:**
A helper library that simplifies working with I²C devices.

**Role in the project:**
This library provides utility functions for reading and writing registers of I²C devices such as the MPU6050. It is commonly used together with the MPU6050 library.

**Used for:**

* Simplified register access
* Device communication abstraction

---

## MPU6050.h

**Purpose:**
Provides a high-level interface for interacting with the MPU6050 Inertial Measurement Unit (IMU).

**Role in the project:**
The MPU6050 sensor measures acceleration. This project uses the sensor’s FIFO buffer to collect accelerometer samples which are then stored locally.

**Used for:**

* Sensor initialization
* Configuring sampling rate and filters
* Reading accelerometer data from the FIFO buffer

Example usage:

```
mpu.initialize();
mpu.getFIFOBytes(buffer, length);
```

---

## WiFi.h

**Purpose:**
Provides WiFi connectivity for ESP32 devices.

**Role in the project:**
The ESP32 connects to a wireless network so that recorded sensor data can be transmitted to a remote server.

**Used for:**

* Connecting to a WiFi network
* Checking connection status
* Managing reconnections

Example usage:

```
WiFi.begin(ssid, password);
```

---

## HTTPClient.h

**Purpose:**
Allows the ESP32 to send HTTP requests.

**Role in the project:**
Sensor data stored locally is uploaded to a server using HTTP POST requests. The `HTTPClient` library manages request construction and transmission.

**Used for:**

* Sending binary data to the backend server
* Adding custom HTTP headers
* Handling HTTP response codes

Example usage:

```
http.POST(data, length);
```

---

## TinyGPS.h

**Purpose:**
Parses GPS data from NMEA sentences.

**Role in the project:**
The GPS module sends location data over UART. The TinyGPS library parses the raw NMEA messages and extracts usable information such as latitude, longitude, altitude, and timestamps.

**Used for:**

* Decoding GPS data streams
* Extracting geographic coordinates
* Retrieving satellite and timing information

Example usage:

```
gps.encode(serialData);
gps.f_get_position(&lat, &lon);
```

---

## SPIFFS.h

**Purpose:**
Provides access to the SPI Flash File System on the ESP32.

**Role in the project:**
Sensor data is temporarily stored on the ESP32's flash memory before being uploaded to the server.

**Used for:**

* Creating and accessing data files
* Writing accelerometer samples
* Logging GPS data

Example usage:

```
File file = SPIFFS.open("/accel.bin", FILE_APPEND);
file.write(data, length);
```

---

## Summary

These libraries enable the core functionality of the system:

* **Sensor acquisition:** MPU6050, I2Cdev, Wire
* **Position tracking:** TinyGPS
* **Local storage:** SPIFFS
* **Network communication:** WiFi, HTTPClient

Together, they allow the ESP32 to collect motion and GPS data, store it locally, and upload it to a remote server.
