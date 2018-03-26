#include <WiFiClientSecure.h>
#include <WiFi.h>
#include <WiFiMulti.h>
WiFiMulti wifiMulti;

#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>

const char* ssid     = "WIFI_PASS";
const char* password = "WIFI_SSID";

// Uncomment to set to learn mode
#define MODE_TRACKING 1
#define LOCATION "living room"

#define GROUP_NAME "mygroup"

const char* host = "cloud.internalpositioning.com";
const char* ntpServer = "pool.ntp.org";

//#define DEBUG 1

void setup() {
  Serial.begin(115200);
  delay(10);

  Serial.println("Find3 ESP client by DatanoiseTV");

  // We start by connecting to a WiFi network

  wifiMulti.addAP(ssid, password);

  Serial.println("Connecting to WiFi...");
  if (wifiMulti.run() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    configTime(0, 0, ntpServer);
  }
}

unsigned long long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return(0);
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

  chipid = ESP.getEfuseMac(); //The chip ID is essentially its MAC address(length: 6 bytes).

  String chipIdStr = String((uint32_t)(chipid>>16));

  JsonObject& root = jsonBuffer.createObject();
  root["username"] = chipIdStr;
  root["group"] = GROUP_NAME;
  JsonArray& data = root.createNestedArray("wifi-fingerprint");

  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0) {
    Serial.println("no networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found:");
    for (int i = 0; i < n; ++i) {

       JsonObject& wifidata = data.createNestedObject();
       wifidata["rssi"] = WiFi.RSSI(i);
       wifidata["mac"] = WiFi.BSSIDstr(i);
    }

    uint64_t currentTime = getTime();
    #ifndef MODE_TRACKING
      root["location"] = LOCATION;
    #endif
    root["timestamp"] = getTime();
    root["password"] = "icanhaznopasswd";
    
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
    #ifndef MODE_TRACKING
    String url = "/train";
    #else
    String url = "/track";
    #endif

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("POST ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Content-Type: application/json\r\n" +
                 "Content-Length: " + request.length() + "\r\n\r\n" +
                 request +
                 "\r\n\n\r\n"
                );

    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 5000) {
        Serial.println(">>> Client Timeout !");
        client.stop();
        return;
      }
    }

    // Read all the lines of the reply from server and print them to Serial
    while (client.available()) {
      String line = client.readStringUntil('\r');
      Serial.print(line);
    }

    Serial.println();
    Serial.println("closing connection");
  }
}

void loop() {
  // Use WiFiClient class to create TCP connections
  SubmitWiFi();
}
