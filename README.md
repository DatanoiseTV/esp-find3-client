# esp-find3-client
Indoor Location using ESP8266/ESP32 and Find3 (https://github.com/schollz/find3)

## What does it do?

This code runs on an ESP8266 / ESP32 microcontroller, which has WiFi on-board and is widely available from about 2€.

It connects to your WiFi and a Find3 server, scans continuously for neighboring WiFi access points and their signal strength and submits it to the server. The server uses machine learning to learn the location from this information.


This way the ESP is used as a "WiFi-Beacon" for Indoor Location.

## How stable is this?

The code running on the ESP is very hackisk. It was a short project of mine done in a matter or 1-2 hours.
Any forks are welcome.

## How does the ESP part work in detail?
* Connect to WiFi
* Synchronize time using NTP (needed for timestamps)
* Scan for WiFi APs
* Prepare JSON to submit to server
* Submit JSON with BSSIDs and their corresponding RSSI to Server
