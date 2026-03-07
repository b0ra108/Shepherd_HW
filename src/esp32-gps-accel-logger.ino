#include "Wire.h"
#include "I2Cdev.h"
#include "MPU6050.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <TinyGPS.h>
#include <SPIFFS.h>


#define WIFI_SSID "Hotspot_SSID"
#define WIFI_PWD "Hotspot_Password"
#define SERVER_URL "http://Your_Ip_Adress/api/device/data"
#define deviceUID "forager"
#define apiKey "b307506fc1626c6c5151b012e1e767414c885cc4d39f0d0f5028d02c362e5cb1"
#define UPLOAD_INTERVAL 15000
#define GPS_LOG_INTERVAL 3000
#define ACCEL_FILE "/accel.bin"
#define GPS_FILE "/gps.bin"


MPU6050 mpu;
uint8_t fifoBuffer[6];

TinyGPS gps;
HardwareSerial GPSSerial(1);// using UART1


struct GpsSample{
  uint32_t epoch;
  int32_t lat;
  int32_t lon;
  int32_t alt;
};


void printAccelFile(const char* path) {
  File file = SPIFFS.open(path, FILE_READ);
  if (!file) {
    Serial.println("Failed to open accel file for debug");
    return;
  }

  Serial.printf("---- ACCEL FILE DUMP ----,bytes=%u\n",file.size());
  uint32_t index = 0;

  while (file.available() >= 6) {
    uint8_t buff[6];
    file.read(fifoBuffer, 6);

    int16_t ax = (fifoBuffer[0] << 8) | fifoBuffer[1];
    int16_t ay = (fifoBuffer[2] << 8) | fifoBuffer[3];
    int16_t az = (fifoBuffer[4] << 8) | fifoBuffer[5];
    Serial.printf(
      "[%lu] X: %.3f | Y: %.3f | Z: %.3f\n",
      index++, ax/16384.0, ay/16384.0, az/16384.0
    );

    // Optional: slow down output so Serial Monitor doesn’t choke
    delay(2);
  }

  Serial.println("---- END ACCELEROMETER FILE ----");
  file.close();
}

void printGpsFile(const char* path) {
  File file = SPIFFS.open(path, FILE_READ);
  if (!file) {
    Serial.println("Failed to open GPS file");
    return;
  }

  Serial.printf("---- GPS FILE DUMP ---- size=%u bytes\n", file.size());

  GpsSample s;
  uint32_t index = 0;

  while (file.available() >= sizeof(GpsSample)) {
    file.read((uint8_t*)&s, sizeof(GpsSample));

    Serial.printf(
      "[%lu] TIME: %lu | LAT: %.7f | LON: %.7f | ALT: %.2f m\n",
      index++,
      s.epoch,
      s.lat/1e7,
      s.lon/1e7,
      s.alt/100.0 
    );

    delay(2); // prevent Serial overflow
  }
}

void sendFileToServer(const char* path, const char* datatype) {
  if (WiFi.status() != WL_CONNECTED) return;

  uint16_t UPLOAD_CHUNK_SIZE = 1024;
  
  File file = SPIFFS.open(path, FILE_READ);
  if (!file) return;

  uint32_t fileSize = file.size();
  uint32_t offset = 0;

  uint8_t buffer[UPLOAD_CHUNK_SIZE];

  while (file.available()) {
    size_t bytesRead = file.read(buffer, UPLOAD_CHUNK_SIZE);
    if (bytesRead > 0) {
      sendBinaryToServer(buffer, bytesRead, datatype, fileSize, offset);
      offset += bytesRead;
      delay(10);
    }
  }

  file.close();
}

void sendBinaryToServer(
  uint8_t* data,
  size_t length,
  const char* dataType,
  uint32_t fileSize,
  uint32_t offset
) {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(SERVER_URL);

  http.addHeader("Content-Type", "application/octet-stream");
  http.addHeader("Sample-Data-Type", dataType);
  http.addHeader("Sample-File-Size", String(fileSize));
  http.addHeader("Sample-Offset", String(offset)); // servera orderlı şekilde gitmediği için bu header da lazım
  http.addHeader("Device-UID", deviceUID);
  http.addHeader("X-API-Key",apiKey);
  
  int code = http.POST(data, length);

  if (code <= 0 || code >= 400) {
    Serial.printf("Upload failed (%s) code=%d\n", dataType, code);
  }

  http.end();
}

void saveSamplesBatch(uint8_t* data, size_t length) {
  File file = SPIFFS.open(ACCEL_FILE, FILE_APPEND);
  if (!file) return;

  file.write(data, length);
  file.close();
}

void readFifo(uint16_t fifoCount){
  File file = SPIFFS.open(ACCEL_FILE, FILE_APPEND);
  while(fifoCount >= 6){
    mpu.getFIFOBytes(fifoBuffer, 6);

    int16_t ax = (fifoBuffer[0] << 8) | fifoBuffer[1];
    int16_t ay = (fifoBuffer[2] << 8) | fifoBuffer[3];
    int16_t az = (fifoBuffer[4] << 8) | fifoBuffer[5];

    /*Serial.print("A: ");
    Serial.print(ax); Serial.print(", ");
    Serial.print(ay); Serial.print(", ");
    Serial.println(az);*/

    fifoCount -= 6;
    file.write(fifoBuffer, 6);
  }
  file.close();
}

uint32_t gpsToEpoch(
  int year, byte month, byte day,
  byte hour, byte minute, byte second
) {
  tm t;
  t.tm_year = year - 1900;
  t.tm_mon  = month - 1;
  t.tm_mday = day;
  t.tm_hour = hour;
  t.tm_min  = minute;
  t.tm_sec  = second;
  t.tm_isdst = 0;

  return (uint32_t)mktime(&t);
}

void logGps() {
  float lat, lon;
  unsigned long age;

  // Get position
  gps.f_get_position(&lat, &lon, &age);
  if (age == TinyGPS::GPS_INVALID_AGE) return;

  // Get altitude
  float alt = gps.f_altitude();
  if (alt == TinyGPS::GPS_INVALID_F_ALTITUDE) return;

  // Require a valid fix
  if (gps.satellites() < 4) return;

  // Get GPS date & time
  int year;
  byte month, day, hour, minute, second, hundredths;

  gps.crack_datetime(
    &year, &month, &day,
    &hour, &minute, &second,
    &hundredths, &age
  );

  // GPS time not ready yet
  //if (year < 2020) return;

  // Fill struct
  GpsSample s;
  s.epoch  = gpsToEpoch(year, month, day, hour, minute, second);
  s.lat = (int32_t)(lat * 1e7);
  s.lon = (int32_t)(lon * 1e7);
  s.alt = (int32_t)(alt * 100);
  
  //Serial.printf("GPS logs (lat long alt):%u %d %d %d\n",s.epoch,s.lat,s.lon,s.alt);
  // Append to file
  File f = SPIFFS.open(GPS_FILE, FILE_APPEND);
  if (!f) return;

  f.write((uint8_t*)&s, sizeof(GpsSample));
  f.close();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Wire.begin();
  GPSSerial.begin(9600, SERIAL_8N1, 16, 17);
  
  if(!SPIFFS.begin(true)) {        // ← mount flash filesystem
  Serial.println("SPIFFS mount failed!");
  while(1);
  }
  SPIFFS.remove(ACCEL_FILE);
  SPIFFS.remove(GPS_FILE);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PWD);
  WiFi.setAutoReconnect(true);
  unsigned long t0 = millis();
  // monitoring wifi connection at the startup
  while (WiFi.status() != WL_CONNECTED &&
        millis() - t0 < 20000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED){
    Serial.println("\nWiFi connected");
    Serial.println(WiFi.localIP());
  } 
  else{
    Serial.println("\nWiFi failed;");
  }

  mpu.initialize();
  mpu.setSleepEnabled(false);
  mpu.setClockSource(MPU6050_CLOCK_PLL_ZGYRO);

  

  // ====== rate control ======
  // DLPF mode 42Hz -> internal sample rate = 1kHz
  // (1kHz / (1 + divider)) = output rate
  // (1kHz / (1 + 99)) = 10 Hz
  mpu.setDLPFMode(MPU6050_DLPF_BW_42);
  mpu.setRate(99); // 10 Hz

  // ====== FIFO RESET + CLEAN PACKET SOURCE ======
  // disable FIFO and DMP
  // FIFO is disabled for doing configurations
  // DMP wont be used 
  mpu.setDMPEnabled(false);
  mpu.setFIFOEnabled(false);

  // enabling accelerometer fifo and disabling gyro
  mpu.setAccelFIFOEnabled(true);
  mpu.setXGyroFIFOEnabled(false);
  mpu.setYGyroFIFOEnabled(false);
  mpu.setZGyroFIFOEnabled(false);


  // reset FIFO so alignment starts at packet 0
  mpu.resetFIFO();

  // enabling FIFO, it starts filling up right after
  mpu.setFIFOEnabled(true);

  Serial.println("FIFO accel at 10 Hz setup complete.");
  
}

unsigned long now;
unsigned long lastFilePrints = millis();
unsigned long lastUploadAttempt = millis();
unsigned long lastGpsLogAttempt = millis();

bool stopLoop = false;

void loop() {
  if (stopLoop) {
    return;
  }
  uint16_t fifoCount = mpu.getFIFOCount();

  //default timestamp value for gps module is around 1999
  //(1970/1/1 00:00 UTC + 944638632 seconds),then gps module 
  // is not fixed and does return defaults if this value is taken
  while (GPSSerial.available()) {
    gps.encode(GPSSerial.read());
  }

  // reading fifo when it exceeds a certain number of bytes
  // so that memory access happens less frequently
  if(fifoCount >= 120){
    readFifo(fifoCount);
  }

  if(millis() - lastGpsLogAttempt >= GPS_LOG_INTERVAL){
    lastGpsLogAttempt = millis();
    logGps();
  }
  // printing files for debug purposes in every 10s
  // should be commented with the printGpsFile and printAccelFile
  // functions in deployment since they acquire extra memory and processing times
  if(millis() - lastFilePrints > 10000){
    lastFilePrints=millis();
    Serial.println("PRINTING ACCEL FILE");
    printAccelFile(ACCEL_FILE);
    Serial.println("PRINTING GPS FILE");
    printGpsFile(GPS_FILE);
  }

  now = millis();
  if (now - lastUploadAttempt >= UPLOAD_INTERVAL) {
    lastUploadAttempt = now;
    if(WiFi.status() == WL_CONNECTED){

      
      sendFileToServer(ACCEL_FILE, "accel");
      sendFileToServer(GPS_FILE, "gps");
      stopLoop = true; 

      
      /*Serial.println("Checking server availability...");
      Checking server availability, to be implemented
      Serial.println("Checking server availability...");
      if (serverAvailable()) {
      }
      else {
        Serial.println("Server not available.");
      }*/
    }
    else{ // FOR DEBUG PURPOSE
        Serial.println("Not connected to WIFI");
    }
  }
}