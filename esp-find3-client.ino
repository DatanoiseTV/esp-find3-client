#include <WiFiClientSecure.h>
#include <WiFi.h>
#include <WiFiMulti.h>
WiFiMulti wifiMulti;

const char* ssid     = "WIFI_SSID";
const char* password = "WIFI_PASS";

const char* host = "cloud.internalpositioning.com";
const char* ntpServer = "pool.ntp.org";

uint64_t chipid;  

//#define DEBUG 1

void setup() {
  Serial.begin(115200);
  delay(10);

  // We start by connecting to a WiFi network

  wifiMulti.addAP(ssid, password);

  Serial.println("Connecting Wifi...");
  if (wifiMulti.run() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    configTime(0, 0, ntpServer);
  }

}

unsigned long getTime() {
time_t now;
struct tm timeinfo;
if (!getLocalTime(&timeinfo)) {
Serial.println("Failed to obtain time");
return(0);
}
time(&now);
return now;
}

void SubmitWiFi(void)
{
  String request;
  uint64_t chipid;  
  chipid = ESP.getEfuseMac();//The chip ID is essentially its MAC address(length: 6 bytes).

  String chipIdStr = String((uint32_t)(chipid>>16));

  request += "{\"username\": \"" + chipIdStr + "\", \"group\": \"jooo\", \"wifi-fingerprint\": [";

  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0) {
    Serial.println("no networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found:");
    for (int i = 0; i < n; ++i) {


      request += "{\"rssi\": ";
      request += WiFi.RSSI(i);
      request += ", \"mac\": \"";
      request += WiFi.BSSIDstr(i);
      request += "\"}";

      if( (i<n-1) )
      {
        request += ",";
      }

      #ifdef DEBUG
      Serial.println("+ @" + String(getTime()) + "000: #" + String(i) + ": " + WiFi.BSSIDstr(i) + " (" +  WiFi.RSSI(i) + ")" );
      #endif
    }

    String localTimeStr = String(getTime());

    request += "], \"location\": \"living room\", \"timestamp\": " + localTimeStr + "000, \"password\": \"frusciante_0128\"}";

    WiFiClientSecure client;
    const int httpsPort = 443;
    if (!client.connect(host, httpsPort)) {
      Serial.println("connection failed");
    }

    // We now create a URI for the request
    String url = "/track";

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
      //Serial.print(line);
    }

    Serial.println();
    Serial.println("closing connection");

  }
}

void loop() {
  // Use WiFiClient class to create TCP connections
  SubmitWiFi();

}
