// To eliminate FW warning when using not latest nina-fw version
// To use whenever WiFi101-FirmwareUpdater-Plugin is not sync'ed with nina-fw version
#define WIFI_FIRMWARE_LATEST_VERSION        "1.4.8"

#include <SPI.h>
#include <WiFiNINA_Generic.h>
#include <ArduinoJson.h>
#include "arduino_secrets.h"
#include "VH400.h"
#include "networking.h"

char ssid[] = SECRET_SSID; // Your network SSID, define in arduino_secrets.h
char pass[] = SECRET_PASS; // Your network password, define in arduino_secrets.h

int MOISTURE_SENSOR_PIN = 0;
int GREEN_LED_PIN = 1;
int RED_LED_PIN = 2;

int keyIndex = 0;                 // your network key Index number (needed only for WEP)

WiFiServer server(80);

void setup()
{
  //Initialize serial and wait for port to open:
  Serial.begin(115200);
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
  WiFiClient client = server.available();   // listen for incoming clients

  if (client)
  {
    // if you get a client,
    Serial.println(F("New client"));           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    
    // For the response payload.
    JsonDocument doc;

    while (client.connected())
    {
      // loop while the client's connected
      if (client.available())
      {
        // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor

        if (c == '\n')
        {
          // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0)
          {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type: application/json");
            client.println();

            serializeJson(doc, client);

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          }
          else
          {
            // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        }
        else if (c != '\r')
        {
          // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        if (currentLine.endsWith("GET /H"))
        {
          digitalWrite(RED_LED_PIN, HIGH);               // GET /H turns the LED on
          doc["status"] = "OK";
        }

        if (currentLine.endsWith("GET /L"))
        {
          digitalWrite(RED_LED_PIN, LOW);                // GET /L turns the LED off
          doc["status"] = "OK";
        }

        if (currentLine.endsWith("GET /S"))
        {
          Serial.println(readVH400(MOISTURE_SENSOR_PIN));
          doc["status"] = "OK";
        }

        if (currentLine.endsWith("GET /network"))
        {
            doc["status"] = "OK";
            doc["ssid"] = WiFi.SSID();
            doc["rssi"] = WiFi.RSSI();
            doc["ip"] = WiFi.localIP();
        } 
      }
    }

    // close the connection:
    client.stop();
    Serial.println(F("Client disonnected"));
  }
}

