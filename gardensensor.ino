// To eliminate FW warning when using not latest nina-fw version
// To use whenever WiFi101-FirmwareUpdater-Plugin is not sync'ed with nina-fw version
#define WIFI_FIRMWARE_LATEST_VERSION        "1.4.8"

// If you want to enable ThingSpeak support, set this to 1.  If you do, please set the appropriate channel
// and API keys secrets in the arduino_secrets.h file.
#define ENABLE_THINGSPEAK 1

// This will control the frequency in which we transmit records to ThingSpeak.  The free
// plans only support so many transmission per day/year so it's good to keep these spread
// out a bit.  This setting is the number of minutes to wait between transmissions.
#define THINGSPEAK_TRANSMIT_FREQUENCY 1 // minute

// The threshold before alerting for soil moisture.
#define THRESHOLD_MOISTURE 10

#include <SPI.h>
#include <WiFiNINA_Generic.h>
#include <ArduinoJson.h>
#include "ThingSpeak.h"
#include "time.h"
#include "arduino_secrets.h"
#include "VH400.h"
#include "networking.h"
#include "web.h"

unsigned long myChannelNumber = SECRET_THINGSPEAK_CHANNEL;
const char * myWriteAPIKey = SECRET_THINGSPEAK_APIKEY;

const char* ssid = SECRET_SSID; // Your network SSID, define in arduino_secrets.h
const char* pass = SECRET_PASS; // Your network password, define in arduino_secrets.h

// Definitions of the various components we have attached to the Arduino.
const int MOISTURE_SENSOR_PIN = 0;
const int GREEN_LED_PIN = 1;
const int RED_LED_PIN = 2;

// Our holder for the last time we transmitted our readings to a third-party service.
unsigned long lastReadingTransmission = 0;

// The server for handling real-time requests.
WiFiServer server(80);
WiFiClient  client;

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

  server.begin();            // start the web server on port 80
  ThingSpeak.begin(client);  // Initialize ThingSpeak
  printWiFiStatus();         // you're connected now, so print out the status
}

void loop()
{
  // Get the readings for this loop.
  float reading = readVH400(MOISTURE_SENSOR_PIN);

  if ( reading < THRESHOLD_MOISTURE )
  {
      digitalWrite(RED_LED_PIN, HIGH);

      // TODO: Trigger automatic watering and/or alerting
  }
  else
  {
      digitalWrite(RED_LED_PIN, LOW);
  }

  // First, we'll handle any incoming HTTP requests for real-time data.
  handleHttpRequests(server, reading);

  #if ENABLE_THINGSPEAK
  // At specified intervals, we'll transmit these readings up to ThingSpeak if it's configured.
  // Alternatively, you could put some type of other third-party transmit here.  It's time constrained
  // so we don't overwhelm the limits on whatever third-party service we're using.  You can adjust
  // the interval by using the THINGSPEAK_TRANSMIT_FREQUENCY (minutes) definition at the top of this file.
  long current = millis();
  if ( lastReadingTransmission == 0 || 
       current >= lastReadingTransmission + ( THINGSPEAK_TRANSMIT_FREQUENCY * 1000 * 60 ) )
  {
    // Update our last run timestamp.
    lastReadingTransmission = current;

    // Write to ThingSpeak. There are up to 8 fields in a channel, allowing you to store up to 8 different
    // pieces of information in a channel.  Here, we write to field 1.
    int responseCode = ThingSpeak.writeField(myChannelNumber, 1, reading, myWriteAPIKey);
    if ( responseCode == 200 )
    {
      Serial.println("ThingSpeak channel update successful.");
    }
    else
    {
      Serial.println("Problem updating ThingSpeak channel. Received HTTP response code " + String(responseCode));
    }
    
    // Do the transmission.
    Serial.println("This is when I would do something.");
  }
  #endif
}
