#include <Arduino.h>
#include <HTTPClient.h>
#include <dummy.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <time.h>
#include "secrets.h"
#include "images.h"

U8G2_SSD1322_NHD_256X64_F_8080 u8g2(U8G2_R0, 4, 5, 6, 7, 8, 9, 10, 11, 12, 15, 13, 14);

const char* ntpServer = "uk.pool.ntp.org";
const long gmtOffset_sec = GMTOFFSET;
const int daylightOffset_sec = DAYLIGHTOFFSET;

const String lat = LATITUDE;
const String lon = LONGDITUDE;
String currentWeatherEndpoint = "https://api.openweathermap.org/data/2.5/weather?lat={lat}&lon={lon}&appid={key}&units=metric";
const String weatherapiKey = OPENWEATHERMAPKEY;

const char* wifissid = SECRET_SSID;
const char* wifipass = SECRET_PW;

unsigned long currentMillis;

unsigned long weatherMillisInterval = 600000;
unsigned long lastWeatherMillis;

unsigned long refreshRateMillisInterval = 500;
unsigned long lastRefreshMillis;

unsigned long wifiRetryMillisInterval = 10000;
unsigned long wifiRetryMillis;
int frame;

String tempStr;

struct WeatherData {
  bool isValid;
  String weatherDesc;
  float temp;
  float tempFeelsLike;
  int humidity;
};

String getFormattedTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "--:--:--";
  }
  char timeBuffer[10];
  strftime(timeBuffer, sizeof(timeBuffer), "%H:%M", &timeinfo);
  return String(timeBuffer);
}

String getFormattedDate() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "Syncing...";
  }
  char dateBuffer[12];
  strftime(dateBuffer, sizeof(dateBuffer), "%d/%m/%Y", &timeinfo);
  return String(dateBuffer);
}

WeatherData latestWeather;

String getRawWeather() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    HTTPClient http;

    client.setInsecure();

    Serial.println("Fetching weather data...");

    if (http.begin(client, currentWeatherEndpoint)) {
	int httpResponseCode = http.GET();

	if (httpResponseCode > 0) {
	  Serial.print("HTTP Response Code: ");
	  Serial.println(httpResponseCode);

	  return http.getString();

	} else {
	  Serial.print("Error code: ");
	  Serial.println(httpResponseCode);

	  return "";
	}
    } else {
	Serial.println("Error");
	return "";
    }
  } else {
    Serial.println("WiFi has disconnected!");
    return "";
  }
}


WeatherData deserializeRawWeatherData(String rawWeatherData) {
  WeatherData currentData;
  currentData.isValid = false;

  if (rawWeatherData == "") {
    return currentData;
  }

  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, rawWeatherData);

  if (error) {
    Serial.print("Weather data deserialization failed: ");
    Serial.print(error.c_str());
    return currentData;
  }

  currentData.isValid = true;

  currentData.weatherDesc = doc["weather"][0]["main"].as<String>();
  Serial.print("Weather Description:");
  Serial.println(currentData.weatherDesc);

  currentData.temp = doc["main"]["temp"];
  Serial.print("Temperature: ");
  Serial.println(currentData.temp);

  currentData.humidity = doc["main"]["humidity"];
  Serial.print("Humidity: ");
  Serial.println(currentData.humidity);

  currentData.tempFeelsLike = doc["main"]["feels_like"];
  return currentData;
}

void setup() {
  Serial.begin(115200);
  u8g2.begin(); !

  WiFi.begin(wifissid, wifipass);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n Connected to WiFi successfully!");

  currentWeatherEndpoint.replace("{lat}", lat);
  currentWeatherEndpoint.replace("{lon}", lon);
  currentWeatherEndpoint.replace("{key}", weatherapiKey);

  Serial.print("API endpoint: ");
  Serial.println(currentWeatherEndpoint);

  Serial.println("Syncing time with NTP Server...");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  lastWeatherMillis = millis() - weatherMillisInterval;
}

void loop() {
  currentMillis = millis();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected! Attempting to reconnect...");
    WiFi.begin(wifissid, wifipass);

    delay(10000);

    if (WiFi.status() != WL_CONNECTED && (currentMillis - wifiRetryMillis >= wifiRetryMillisInterval)) {
	Serial.println("WiFi did not connect successfully after 10 seconds. Continuing.");
    }
  }
  //fetch and display weather
  if (currentMillis - lastWeatherMillis >= weatherMillisInterval && WiFi.status() == WL_CONNECTED) {
    lastWeatherMillis = currentMillis;

    String rawWeatherData = getRawWeather();

    if (rawWeatherData != "") {
	Serial.println("Weather data successfully fetched, deserializing...");

    latestWeather = deserializeRawWeatherData(rawWeatherData);

	if (latestWeather.isValid == true) {
	  Serial.println("Weather data successfully deserialised.");
	} else {
	  Serial.println("Weather data deserialisation failed.");
	}
    } else {
	Serial.println("Failed to fetch weather data.");
    }
  }
  //drawing onto screen
  if (currentMillis - lastRefreshMillis >= refreshRateMillisInterval) {
    u8g2.clearBuffer();

    u8g2.drawRFrame(0, 0, 256, 64, 3);

    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(4, 9, "DASHBOARD");

    if (WiFi.status() != WL_CONNECTED) {
	u8g2.setFont(u8g2_font_4x6_tr);
	u8g2.drawStr(60, 9, "WiFi disconnected, readings will be inaccurate.");
    }


    //horizontal line under "DASHBOARD"
    u8g2.drawLine(4, 10, 145, 10);

    //vertical devider between time/date and weather data
    u8g2.drawLine(93, 12, 93, 63);

    u8g2.drawLine(146, 10, 146, 63);

    //line under temp
    u8g2.drawLine(93, 28, 145, 28);

    //line under feels like temp
    u8g2.drawLine(93, 46, 145, 46);

    u8g2.setFont(u8g2_font_6x13_tf);
    u8g2.drawXBM(96, 14, 7, 12, img_thermometer);

    tempStr = String(latestWeather.temp, 1) + "°C";
    u8g2.drawUTF8(105, 24, tempStr.c_str());

    u8g2.drawXBM(97, 50, 9, 12, img_humidity);
    tempStr = String(latestWeather.humidity) + "%";
    u8g2.drawStr(109, 60, tempStr.c_str());

    u8g2.drawXBM(96, 32, 11, 12, img_tempFeelsLike);
    tempStr = String(latestWeather.tempFeelsLike, 1) + "°C";
    u8g2.drawUTF8(109, 42, tempStr.c_str());

    String timeStr = getFormattedTime();
    String dateStr = getFormattedDate();

    u8g2.setFont(u8g2_font_profont29_tr);
    u8g2.drawStr(8, 36, timeStr.c_str());

    u8g2.setFont(u8g2_font_profont17_tr);
    u8g2.drawStr(3, 56, dateStr.c_str());

    u8g2.sendBuffer();
    lastRefreshMillis = millis();
  }
}
