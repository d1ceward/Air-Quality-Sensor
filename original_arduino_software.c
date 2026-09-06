/*
This is the code for the AirGradient DIY PRO Air Quality Sensor with an ESP8266 Microcontroller.

It is a high quality sensor showing PM2.5, CO2, Temperature and Humidity on a small display and can send data over Wifi.

The codes needs the following libraries installed:
“WifiManager by tzapu, tablatronix” tested with version 2.0.14-beta
“U8g2” by oliver tested with version 2.33.15
“SGP30” by Rob Tilaart tested with Version 0.1.5

Configuration:
Please set in the code below the configuration parameters.
*/

#include <AirGradient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include "SGP30.h"
#include <U8g2lib.h>

// Device hostname
const char* deviceId = "";

// Wifi
const char* ssid = "";
const char* password = "";
const int port = 9926;

// NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org");
const String weekDays[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

// Uncomment/Comment the line below to configure a static IP address.
#define staticip
#ifdef staticip
IPAddress static_ip(192, 168, 1, 2);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(192, 168, 1, 1);
#endif

unsigned long currentMillis = 0;

const int statsOledInterval = 6000;
const int clockOledInterval = 4000;
unsigned long previousOled = 0;
bool showClock = false;

const int tvocInterval = 2500;
unsigned long previousTVOC = 0;
int TVOC = 0;

const int co2Interval = 5000;
unsigned long previousCo2 = 0;
int Co2 = 0;

const int pm25Interval = 40000;
const int pm25SleepInterval = 180000;
unsigned long previousPm25 = 0;
bool pm25Sleeping = false;
int pm25Tmp = 0;
int pm25 = 0;

const int tempHumInterval = 2500;
unsigned long previousTempHum = 0;
float temp = 0;
int hum = 0;

AirGradient ag = AirGradient();
SGP30 SGP;

// Display bottom right
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// Replace above if you have display on top left
//U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R2, /* reset=*/ U8X8_PIN_NONE);

ESP8266WebServer server(port);

// Calculate PM2.5 US AQI
int PM_TO_AQI_US(int pm02) {
  if (pm02 <= 12.0) return ((50 - 0) / (12.0 - .0) * (pm02 - .0) + 0);
  else if (pm02 <= 35.4) return ((100 - 50) / (35.4 - 12.0) * (pm02 - 12.0) + 50);
  else if (pm02 <= 55.4) return ((150 - 100) / (55.4 - 35.4) * (pm02 - 35.4) + 100);
  else if (pm02 <= 150.4) return ((200 - 150) / (150.4 - 55.4) * (pm02 - 55.4) + 150);
  else if (pm02 <= 250.4) return ((300 - 200) / (250.4 - 150.4) * (pm02 - 150.4) + 200);
  else if (pm02 <= 350.4) return ((400 - 300) / (350.4 - 250.4) * (pm02 - 250.4) + 300);
  else if (pm02 <= 500.4) return ((500 - 400) / (500.4 - 350.4) * (pm02 - 350.4) + 400);
  else return 500;
};

String generateMetrics() {
  String message = "";
  String idString = "{id=\"" + String(deviceId) + "\",mac=\"" + WiFi.macAddress().c_str() + "\"}";

  message += "# HELP tvoc TVOC value, in ppm\n";
  message += "# TYPE tvoc gauge\n";
  message += "tvoc";
  message += idString;
  message += String(TVOC);
  message += "\n";

  message += "# HELP co2 CO2 value, in ppm\n";
  message += "# TYPE co2 gauge\n";
  message += "co2";
  message += idString;
  message += String(Co2);
  message += "\n";

  message += "# HELP pm25 Particulate Matter PM2.5 value\n";
  message += "# TYPE pm25 gauge\n";
  message += "pm25";
  message += idString;
  message += String(pm25);
  message += "\n";

  message += "# HELP aqius US Air Quality Index value\n";
  message += "# TYPE aqius gauge\n";
  message += "aqius";
  message += idString;
  message += String(PM_TO_AQI_US(pm25));
  message += "\n";

  message += "# HELP temp Temperature, in degrees Celsius\n";
  message += "# TYPE temp gauge\n";
  message += "temp";
  message += idString;
  message += String(temp);
  message += "\n";

  message += "# HELP hum Relative humidity, in percent\n";
  message += "# TYPE hum gauge\n";
  message += "hum";
  message += idString;
  message += String(hum);
  message += "\n";

  return message;
}

void setup() {
  Serial.begin(115200);

  // Init screen librairy
  u8g2.setBusClock(100000);
  u8g2.begin();

  setupWifi();
  setupServer();

  // Display default message
  drawStringsOLED("Warming up the", "sensors...", "");

  // Init sensors
  Serial.println(SGP.begin());
  SGP.GenericReset();

  // Init ntp config
  timeClient.begin();
  timeClient.setTimeOffset(3600 * 2);
  timeClient.setUpdateInterval(900000); // update time every 15 min

  ag.CO2_Init();
  ag.PMS_Init();
  ag.TMP_RH_Init(0x44);
}

void loop() {
  timeClient.update();

  currentMillis = millis();

  updatePm25();
  updateCo2();
  updateTVOC();
  updateTempHum();

  if (showClock) {
    if (currentMillis - previousOled >= statsOledInterval) {
      previousOled = currentMillis;

      displayClockOLED();
      showClock = false;
    }
  } else {
    if (currentMillis - previousOled >= clockOledInterval) {
      previousOled = currentMillis;

      displaySensorsOLED();
      showClock = true;
    }
  }

  server.handleClient();
}

void setupWifi() {
  // Set static IP address if configured.
#ifdef staticip
  WiFi.config(static_ip, gateway, subnet, dns);
#endif

  // Set WiFi mode to client (without this it may try to act as an AP).
  WiFi.mode(WIFI_STA);

  // Configure Hostname
  if ((deviceId != NULL) && (deviceId[0] == '\0')) {
    Serial.printf("No Device ID is Defined, Defaulting to board defaults");
  } else {
    wifi_station_set_hostname(deviceId);
    WiFi.setHostname(deviceId);
  }

  // Setup and wait for WiFi.
  WiFi.begin(ssid, password);
  Serial.println("");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    drawStringsOLED("Trying to", "connect...", "");
    Serial.println(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());
  Serial.print("Hostname: ");
  Serial.println(WiFi.hostname());
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  
  for (uint i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  
  server.send(404, "text/html", message);
}

void handleRoot() {
  server.send(200, "text/plain", generateMetrics());
}

void setupServer() {
  server.on("/", handleRoot);
  server.on("/metrics", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();
}

void drawStringsOLED(String ln1, String ln2, String ln3) {
  u8g2.firstPage();

  do {
    u8g2.setFont(u8g2_font_t0_16_tf);
    u8g2.drawStr(1, 10, String(ln1).c_str());
    u8g2.drawStr(1, 30, String(ln2).c_str());
    u8g2.drawStr(1, 50, String(ln3).c_str());
  } while (u8g2.nextPage());
}

void displaySensorsOLED() {
  u8g2.firstPage();
  do {
    // Draw lines
    u8g2.drawLine(2, 14, 126, 14);
    u8g2.drawLine(41, 14, 41, 64);
    u8g2.drawLine(86, 14, 86, 64);

    // Draw strings with 16 font
    u8g2.setFont(u8g2_font_t0_16_tf);
    u8g2.drawUTF8(2, 10, String(String(temp) + "°C").c_str());
    u8g2.drawStr(96, 10, String(String(hum) + "%").c_str());
    u8g2.drawStr(2, 47, String(Co2).c_str());
    u8g2.drawStr(46, 47, String(pm25).c_str());
    u8g2.drawStr(91, 47, String(TVOC).c_str());

    // Draw strings with 14 font
    u8g2.setFont(u8g2_font_t0_14_tf);
    u8g2.drawStr(2, 30, String("CO2").c_str());
    u8g2.drawStr(2, 62, String("ppm").c_str());

    u8g2.drawStr(46, 30, String("PM2.5").c_str());
    u8g2.drawUTF8(46, 62, String("ug/m³").c_str());

    u8g2.drawStr(91, 30, String("TVOC").c_str());
    u8g2.drawStr(91, 62, String("ppb").c_str());
  } while (u8g2.nextPage());
}

void updatePm25() {
  if (pm25Sleeping) {
    if (currentMillis - previousPm25 >= pm25SleepInterval) {
      previousPm25 = currentMillis;
      pm25Sleeping = false;
      ag.wakeUp();
    }
  } else {
    if (currentMillis - previousPm25 >= pm25Interval) {
      pm25Tmp = ag.getPM2_Raw();

      if (pm25Tmp < 0)
        return ;

      previousPm25 = currentMillis;
      pm25 = pm25Tmp;

      pm25Sleeping = true;
      ag.sleep();
    }
  }
}

void updateCo2() {
  if (currentMillis - previousCo2 >= co2Interval) {
    previousCo2 = currentMillis;

    Co2 = ag.getCO2_Raw();
  }
}

void updateTVOC() {
  if (currentMillis - previousTVOC >= tvocInterval) {
    previousTVOC = currentMillis;

    SGP.measure(true);
    TVOC = SGP.getTVOC();
  }
}

void updateTempHum() {
  if (currentMillis - previousTempHum >= tempHumInterval) {
    previousTempHum = currentMillis;

    TMP_RH result = ag.periodicFetchData();
    temp = result.t;
    hum = result.rh;
  }
}

unsigned int getYear() {
  time_t rawtime = timeClient.getEpochTime();
  struct tm * ti;
  ti = localtime (&rawtime);
  unsigned int year = ti->tm_year + 1900;
  return year;
}

unsigned int getMonth() {
  time_t rawtime = timeClient.getEpochTime();
  struct tm * ti;
  ti = localtime(&rawtime);
  unsigned int month = ti->tm_mon + 1;

  return month;
}

unsigned int getDate() {
  time_t rawtime = timeClient.getEpochTime();
  struct tm * ti;
  ti = localtime(&rawtime);
  unsigned int month = ti->tm_mday;

  return month;
}

void displayClockOLED() {
  unsigned int year = getYear();
  unsigned int month = getMonth();
  unsigned int day = getDate();
  unsigned int hour = timeClient.getHours();
  unsigned int minute = timeClient.getMinutes();
  unsigned int second = timeClient.getSeconds();

  String weekDay = weekDays[timeClient.getDay()];
  String fYear = String(year);
  String fDate = (day < 10 ? "0" : "") + String(day) + "/" + (month < 10 ? "0" : "") + String(month);
  String fTime = (hour < 10 ? "0" : "") + String(hour) + ":" + (minute < 10 ? "0" : "") + String(minute);

  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_helvB18_tn);
    u8g2.drawStr(11, 21, fDate.c_str());
    u8g2.setFont(u8g2_font_helvB10_tr);
    u8g2.drawStr(77, 12, fYear.c_str());
    u8g2.drawStr(77, 26, weekDay.c_str());
    u8g2.setFont(u8g2_font_helvB24_tn);
    u8g2.drawStr(16, 56, fTime.c_str());
  } while (u8g2.nextPage());
}
