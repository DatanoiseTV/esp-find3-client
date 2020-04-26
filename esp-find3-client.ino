/*
  This file is part of esp-find3-client by Sylwester aka DatanoiseTV.
  The original source can be found at https://github.com/DatanoiseTV/esp-find3-client.

  26/04/2020: Adjustements by Wizardry and Steamworks.

  esp-find3-client is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  esp-find3-client is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with esp-find3-client.  If not, see <http://www.gnu.org/licenses/>.
*/

///////////////////////////////////////////////////////////////////////////
//                             CONFIGURATION                             //
///////////////////////////////////////////////////////////////////////////

// Set to the WiFi AP name.
#define WIFI_SSID ""
// Set to the WiFi AP password.
#define WIFI_PSK ""

// Set to 1 for learning mode.
#define MODE_LEARNING 0
#define LOCATION ""

// Family name.
#define FAMILY_NAME ""

// BLE requires large app partition or the sketch will not fit.
// Please pick either of:
// * Tools -> Partition scheme -> Minimal SPIFFS (1.9MB APP / 190KB SPIFFS)
// * Tools -> Partition scheme -> Huge App (3MB No OTA / 1MB SPIFFS)
// Set to 1 to enable BLE.
#define USE_BLE 0
#define BLE_SCANTIME 5

// Set to 1 for power saving. This will make it run less often.
#define USE_DEEPSLEEP 0

// 20 currently results in an interval of 45s
#define TIME_TO_SLEEP  20        /* Time ESP32 will go to sleep (in seconds) */

// Official server: cloud.internalpositioning.com
#define FIND_HOST "cloud.internalpositioning.com"
// Official port: 443 and SSL set to 1
#define FIND_PORT 443
// Whether to use SSL for the HTTP connection.
// Set to 1 for official cloud server.
#define USE_HTTP_SSL 1
// Timeout connecting to find3 server expressed in milliseconds.
#define HTTP_TIMEOUT 2500

// The NTP server to use for time updates.
#define NTP_HOST "pool.ntp.org"
// The offset in seconds from UTC, ie: 3600 for +1 Hour.
#define UTC_OFFSET 0

// Set to 1 to enable. Used for verbose debugging.
#define DEBUG 0

///////////////////////////////////////////////////////////////////////////
//                              INTERNALS                                //
///////////////////////////////////////////////////////////////////////////

#ifdef ARDUINO_ARCH_ESP32
#include <WiFiClientSecure.h>
#include <WiFi.h>
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#else
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <time.h>
ESP8266WiFiMulti wifiMulti;
#endif
#include <WiFiUdp.h>
#include <NTPClient.h>

#if defined(ARDUINO_ARCH_ESP8266)
#define GET_CHIP_ID() String(ESP.getChipId(), HEX)
#elif defined(ARDUINO_ARCH_ESP32)
#define GET_CHIP_ID() String(((uint16_t)(ESP.getEfuseMac()>>32)), HEX)
#endif

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */

#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>

// Automagically disable BLE on ESP8266
#if defined(ARDUINO_ARCH_ESP8266)
#define USE_BLE 0
#endif

#if (USE_BLE == 1)
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      // Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
    }
};
#endif

#ifdef ARDUINO_ARCH_ESP32
RTC_DATA_ATTR int bootCount = 0;
#endif

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_HOST, UTC_OFFSET, 60000);

/*
  Method to print the reason by which ESP32
  has been awaken from sleep
*/
void print_wakeup_reason() {
#ifdef ARDUINO_ARCH_ESP32
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();
  ++bootCount;
  Serial.println("[ INFO ]\tBoot number: " + String(bootCount));

  switch (wakeup_reason)
  {
    case 1  : Serial.println("[ INFO ]\tWakeup caused by external signal using RTC_IO"); break;
    case 2  : Serial.println("[ INFO ]\tWakeup caused by external signal using RTC_CNTL"); break;
    case 3  : Serial.println("[ INFO ]\tWakeup caused by timer"); break;
    case 4  : Serial.println("[ INFO ]\tWakeup caused by touchpad"); break;
    case 5  : Serial.println("[ INFO ]\tWakeup caused by ULP program"); break;
    default : Serial.println("[ INFO ]\tWakeup was not caused by deep sleep"); break;
  }
#endif
}

void SubmitWiFi(void)
{
  String request;

  StaticJsonDocument<256> jsonBuffer;
  JsonObject root = jsonBuffer.to<JsonObject>();

  root["d"] = "esp-" + GET_CHIP_ID();
  root["f"] = FAMILY_NAME;
  root["t"] = timeClient.getEpochTime();
  JsonObject data = root.createNestedObject("s");

  Serial.println("[ INFO ]\tWiFi scan starting..");
  int n = WiFi.scanNetworks(false, true);
  Serial.println("[ INFO ]\tWiFi Scan finished.");
  if (n == 0) {
    Serial.println("[ ERROR ]\tNo networks found");
  } else {
    Serial.print("[ INFO ]\t");
    Serial.print(n);
    Serial.println(" WiFi networks found.");
    JsonObject wifi_network = data.createNestedObject("wifi");
    for (int i = 0; i < n; ++i) {
      wifi_network[WiFi.BSSIDstr(i)] = WiFi.RSSI(i);
    }

#if (USE_BLE == 1)
    Serial.println("[ INFO ]\tBLE scan starting..");
    BLEDevice::init("");
    BLEScan* pBLEScan = BLEDevice::getScan(); // create new scan
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true); // active scan uses more power, but get results faster
    BLEScanResults foundDevices = pBLEScan->start(BLE_SCANTIME);

    Serial.print("[ INFO ]\t");
    Serial.print(foundDevices.getCount());
    Serial.println(" BLE devices found.");

    JsonObject bt_network = data.createNestedObject("bluetooth");
    for (int i = 0; i < foundDevices.getCount(); i++)
    {
      std::string mac = foundDevices.getDevice(i).getAddress().toString();
      bt_network[(String)mac.c_str()] = (int)foundDevices.getDevice(i).getRSSI();
    }
#else
    Serial.println("[ INFO ]\tBLE scan skipped (BLE disabled)..");
#endif // USE_BLE

#if (MODE_LEARNING == 1)
    root["l"] = LOCATION;
#endif

    serializeJson(root, request);

#if (DEBUG == 1)
    Serial.println("[ DEBUG ]\t" + request);
#endif

#if (USE_HTTP_SSL == 1)
    WiFiClientSecure client;
#else
    WiFiClient client;
#endif
    if (!client.connect(FIND_HOST, FIND_PORT)) {
      Serial.println("[ WARN ]\tConnection to server failed...");
      return;
    }

    // We now create a URI for the request
    String url = "/data";

    Serial.print("[ INFO ]\tRequesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("POST ") + url + " HTTP/1.1\r\n" +
                 "Host: " + FIND_HOST + "\r\n" +
                 "Content-Type: application/json\r\n" +
                 "Content-Length: " + request.length() + "\r\n\r\n" +
                 request +
                 "\r\n\r\n"
                );

    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > HTTP_TIMEOUT) {
        Serial.println("[ ERROR ]\tHTTP Client Timeout !");
        client.stop();
        return;
      }
    }

    // Check HTTP status
    char status[60] = {0};
    client.readBytesUntil('\r', status, sizeof(status));
    if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
      Serial.print(F("[ ERROR ]\tUnexpected Response: "));
      Serial.println(status);
      return;
    }
    else
    {
      Serial.println(F("[ INFO ]\tGot a 200 OK."));
    }

    char endOfHeaders[] = "\r\n\r\n";
    if (!client.find(endOfHeaders)) {
      Serial.println(F("[ ERROR ]\t Invalid Response"));
      return;
    }
    else
    {
      Serial.println("[ INFO ]\tLooks like a valid response.");
    }

    Serial.println("[ INFO ]\tClosing connection.");
    Serial.println("=============================================================");

#if (USE_DEEPSLEEP == 1)
#if defined(ARDUINO_ARCH_ESP8266)
    ESP.deepSleep(TIME_TO_SLEEP * uS_TO_S_FACTOR);
#elif defined(ARDUINO_ARCH_ESP32)
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    esp_deep_sleep_start();
#endif
#endif
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

#if (USE_BLE == 1)
  Serial.println("Find3 ESP client by DatanoiseTV (WiFi + BLE support.)");
#else
  Serial.println("Find3 ESP client by DatanoiseTV (WiFi support WITHOUT BLE.)");
#endif

  print_wakeup_reason();

  Serial.print("[ INFO ]\tChipID is: ");
  Serial.println("esp-" + GET_CHIP_ID());

  wifiMulti.addAP(WIFI_SSID, WIFI_PSK);

  Serial.println("[ INFO ]\tConnecting to WiFi..");
  if (wifiMulti.run() == WL_CONNECTED) {
    Serial.println("[ INFO ]\tWiFi connection established.");
    Serial.print("[ INFO ]\tIP address: ");
    Serial.println(WiFi.localIP());
  }

  timeClient.begin();
}

void loop() {
  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("[ WARN ]\tWiFi not connected, retrying...");
    delay(1000);
    return;
  }
  timeClient.update();
  SubmitWiFi();
}
