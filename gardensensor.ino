// To eliminate FW warning when using not latest nina-fw version
// To use whenever WiFi101-FirmwareUpdater-Plugin is not sync'ed with nina-fw version
#define WIFI_FIRMWARE_LATEST_VERSION        "1.4.8"

#include <SPI.h>
#include <WiFiNINA_Generic.h>
#include <ArduinoJson.h>
#include "arduino_secrets.h"
#include "VH400.h"
#include "networking.h"
#include "web.h"

char ssid[] = SECRET_SSID; // Your network SSID, define in arduino_secrets.h
char pass[] = SECRET_PASS; // Your network password, define in arduino_secrets.h

const int MOISTURE_SENSOR_PIN = 0;
const int GREEN_LED_PIN = 1;
const int RED_LED_PIN = 2;

WiFiServer server(80);

void setup()
{
  // Initialize serial and wait for port to open:
  Serial.begin(115200);

  // Setup the various pins we'll be using.
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(MOISTURE_SENSOR_PIN, INPUT);

  if ( initializeWiFi(ssid, pass) == WL_CONNECTED )
  {
    // Turn on the green LED to visually indicate we have a wifi connection.
    digitalWrite(GREEN_LED_PIN, HIGH);
  }

  server.begin();                           // start the web server on port 80
  printWiFiStatus();                        // you're connected now, so print out the status
}

void loop()
{
  float reading = readVH400(MOISTURE_SENSOR_PIN);

  handleHttpRequests(server, reading);
}

