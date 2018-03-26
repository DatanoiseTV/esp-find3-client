/*
  This file is part of esp-find3-client by Sylwester aka DatanoiseTV.
  The original source can be found at https://github.com/DatanoiseTV/esp-find3-client.

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

#include <WiFiClientSecure.h>
#include <WiFi.h>
#include <WiFiMulti.h>
WiFiMulti wifiMulti;

#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>

const char* ssid     = "WIFI_SSID";
const char* password = "WIFI_PASS";

// Uncomment to set to learn mode
//#define MODE_TRACKING 1
#define LOCATION "living room"

#define GROUP_NAME "jooox"

// Important! BLE + WiFi Support does not fit in standard partition table.
// Manual experimental changes are needed.
// See https://desire.giesecke.tk/index.php/2018/01/30/change-partition-size/
//#define USE_BLE 1
#define BLE_SCANTIME 30

#ifdef USE_BLE
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#endif

const char* host = "cloud.internalpositioning.com";
const char* ntpServer = "pool.ntp.org";

//#define DEBUG 1

String chipIdStr;

void setup() {
  Serial.begin(115200);
  delay(10);

  #ifdef USE_BLE
  Serial.println("Find3 ESP client by DatanoiseTV (WiFi + BLE support.)");
  #else
  Serial.println("Find3 ESP client by DatanoiseTV (WiFi support WITHOUT BLE.)");
  #endif
  
  chipIdStr = String((uint32_t)(ESP.getEfuseMac()>>16));
  Serial.print("[ INFO ]\tChipID is: ");
  Serial.println(chipIdStr);

  wifiMulti.addAP(ssid, password);

  Serial.println("[ INFO ]\tConnecting to WiFi..");
  if (wifiMulti.run() == WL_CONNECTED) {
    Serial.println("[ INFO ]\tWiFi connection established.");
    Serial.print("[ INFO ]\tIP address: ");
    Serial.println(WiFi.localIP());
    configTime(0, 0, ntpServer);
  }
}

unsigned long long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("[ ERROR ]\tFailed to obtain time via NTP.");
    return(0);
  }
  else
  {
    Serial.println("[ INFO ]\tSuccessfully obtained time via NTP.");
  }
  time(&now);
  unsigned long long uTime = (uintmax_t)now;
  return uTime * 1000UL;
}

void SubmitWiFi(void)
{
  String request;
  uint64_t chipid;  

  DynamicJsonBuffer jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();
  root["d"] = chipIdStr;
  root["f"] = GROUP_NAME;
  JsonObject& data = root.createNestedObject("s");

  Serial.println("[ INFO ]\t WiFi scan starting..");
  int n = WiFi.scanNetworks();
  Serial.println("[ INFO ]\tWiFi Scan finished.");
  if (n == 0) {
    Serial.println("[ ERROR ]\tNo networks found");
  } else {
    Serial.print("[ INFO ]\t");
    Serial.print(n);
    Serial.println(" WiFi networks found.");
    JsonObject& wifi_network = data.createNestedObject("wifi");
    for (int i = 0; i < n; ++i) {
      wifi_network[WiFi.BSSIDstr(i)] = WiFi.RSSI(i);
    }

    #ifdef USE_BLE
    Serial.println("[ INFO ]\t BLE scan starting..");
    BLEDevice::init("");
    BLEScan* pBLEScan = BLEDevice::getScan(); //create new scan
    pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
    BLEScanResults foundDevices = pBLEScan->start(BLE_SCANTIME);

    Serial.print("[ INFO ]\t");
    Serial.print(foundDevices.getCount());
    Serial.println(" BLE devices found.");

    JsonObject& bt_network = data.createNestedObject("bluetooth");
    for(int i=0; i<foundDevices.getCount(); i++)
    {
      std::string mac = foundDevices.getDevice(i).getAddress().toString();
      bt_network[(String)mac.c_str()] = (int)foundDevices.getDevice(i).getRSSI();
    }
    #endif

    uint64_t currentTime = getTime();
    #ifndef MODE_TRACKING
      root["l"] = LOCATION;
    #endif
    root["t"] = getTime();
    
    root.printTo(request);

    #ifdef DEBUG
    Serial.println(request);
    #endif
    
    WiFiClientSecure client;
    const int httpsPort = 443;
    if (!client.connect(host, httpsPort)) {
      Serial.println("connection failed");
    }

    // We now create a URI for the request
    String url = "/data";

    Serial.print("[ INFO ]\tRequesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("POST ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Content-Type: application/json\r\n" +
                 "Content-Length: " + request.length() + "\r\n\r\n" +
                 request +
                 "\r\n\r\n"
                );

    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 5000) {
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
  }
}

void loop() {
  // Use WiFiClient class to create TCP connections
  SubmitWiFi();
}
