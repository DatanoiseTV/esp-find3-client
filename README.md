# esp-find3-client
Indoor Location using ESP8266/ESP32 and Find3 (https://github.com/schollz/find3)

Original repository at https://github.com/DatanoiseTV/esp-find3-client

## What does it do?

This code runs on an ESP8266 / ESP32 microcontroller, which has WiFi (and BLE on ESP32) on-board and is widely available from about 2€.

It connects to your WiFi and a Find3 server, scans continuously for neighboring WiFi access points and their signal strength and submits it to the server. The server uses machine learning to learn and estimate the location from this information.


This way the ESP is used as a "WiFi-Beacon" for Indoor Location.

## How stable is this?

It was a short project of mine done in a matter or a couple of hours.
Any bug reports and/or forks are welcome.

## How does the ESP part work in detail?
* Connect to WiFi
* Synchronize time using NTP (needed for timestamps)
* Scan for WiFi APs (**and now also BLE on ESP32**)
* Prepare JSON to submit to server
* Submit JSON with WiFi BSSIDs and BLE adresses and their corresponding RSSI to Server
