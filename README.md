# esp-find3-client
Indoor Location using ESP8266/ESP32 and Find3 (https://github.com/schollz/find3)

Original repository at https://github.com/DatanoiseTV/esp-find3-client

## Quick start
* Download Arduino IDE
* Download ESP32 support for Arduino IDE
* Download and install ArduinoJSON library
* Fetch code and edit esp-find3-client.ino
* Change WiFi SSID and Password to reflect your WiFi Setup. You can add multiple APs.
* Change GROUP_NAME to your group name
* If you wish to enable BLE scanning, set ```USE_BLE``` to 1.
* Upload code to ESP8266/ESP32 and watch the Arduino Serial Terminal for debug info.

### Learning new locations

#### Android / iOS
The easiest way to learn new locations is using the Find3 app. Follow the official instructions at https://www.internalpositioning.com/doc/tracking_your_phone.md

#### Multiple ESP beacons
* Prepare each ESP beacon by changing the ```#define LOCATION "<YOUR_LOCATION>"``` to the location used for learning the individual beacon.
* Set ```#define MODE_LEARNING 1``` on the top of the code
* Put the corresponding beacons in the places / rooms you want to learn
* Power up beacons using USB power supplies or batteries and let it run for 5-10 minutes
* Disconnect all beacons and change ```#define MODE_LEARNING 0``` on the top of the code to go back to tracking mode.

## What does it do?

This code runs on an ESP8266 / ESP32 microcontroller, which has WiFi (and BLE on ESP32) on-board and is widely available from about 2€.

It connects to your WiFi and a Find3 server, scans continuously for neighboring WiFi access points and their signal strength and submits it to the server. The server uses machine learning to learn and estimate the location from this information.


This way the ESP is used as a "WiFi-Beacon" for Indoor Location.

## Is it stable?

It was a short project of mine done in a matter or a couple of hours.
Any bug reports and/or forks are welcome. Things seem to work fairly good so far.
Use it at your own risk. This comes with no warranty.

## Technical details
It performs the following steps:

* Connect to WiFi
* Synchronize time using NTP (needed for timestamps)
* Scan for WiFi APs (**and now also BLE on ESP32**)
* Prepare JSON to submit to server
* Submit JSON with WiFi BSSIDs and BLE adresses and their corresponding RSSI to Server

## Contact

You can contact me at find32dev (at) ext.no-route.org.
