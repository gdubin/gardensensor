// To eliminate FW warning when using not latest nina-fw version
// To use whenever WiFi101-FirmwareUpdater-Plugin is not sync'ed with nina-fw version
#define WIFI_FIRMWARE_LATEST_VERSION        "1.4.8"

#include <SPI.h>
#include <WiFiNINA_Generic.h>
#include "arduino_secrets.h"

char ssid[] = SECRET_SSID; // Your network SSID, define in arduino_secrets.h
char pass[] = SECRET_PASS; // Your network password, define in arduino_secrets.h

int MOISTURE_SENSOR_PIN = 0;
int GREEN_LED = 1;
int RED_LED = 2;

int keyIndex = 0;                 // your network key Index number (needed only for WEP)

int status = WL_IDLE_STATUS;

WiFiServer server(80);

void setup()
{
  //Initialize serial and wait for port to open:
  Serial.begin(115200);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(MOISTURE_SENSOR_PIN, INPUT);

  initializeWiFi();
  server.begin();                           // start the web server on port 80
  printWiFiStatus();                        // you're connected now, so print out the status
}

float readVH400(int analogPin) {
  // This function returns Volumetric Water Content by converting the analogPin value to voltage
  // and then converting voltage to VWC using the piecewise regressions provided by the manufacturer
  // at http://www.vegetronix.com/Products/VH400/VH400-Piecewise-Curve.phtml
  
  // NOTE: You need to set analogPin to input in your setup block
  //   ex. pinMode(<analogPin>, INPUT);
  //   replace <analogPin> with the number of the pin you're going to read from.
  
  // Read value and convert to voltage  
  int sensor1DN = analogRead(analogPin);
  float sensorVoltage = sensor1DN*(3.0 / 1023.0);
  float VWC;
  
  // Calculate VWC
  if(sensorVoltage <= 1.1) {
    VWC = 10*sensorVoltage-1;
  } else if(sensorVoltage > 1.1 && sensorVoltage <= 1.3) {
    VWC = 25*sensorVoltage-17.5;
  } else if(sensorVoltage > 1.3 && sensorVoltage <= 1.82) {
    VWC = 48.08*sensorVoltage-47.5;
  } else if(sensorVoltage > 1.82) {
    VWC = 26.32*sensorVoltage-7.89;
  }
  return(VWC);
}

struct VH400 {
  double analogValue;
  double analogValue_sd;
  double voltage;
  double voltage_sd;
  double VWC;
  double VWC_sd;
};

struct VH400 readVH400_wStats(int analogPin, int nMeasurements = 100, int delayBetweenMeasurements = 50) {
  // This variant calculates the mean and standard deviation of 100 measurements over 5 seconds.
  // It reports mean and standard deviation for the analog value, voltage, and WVC.
  
  // This function returns Volumetric Water Content by converting the analogPin value to voltage
  // and then converting voltage to VWC using the piecewise regressions provided by the manufacturer
  // at http://www.vegetronix.com/Products/VH400/VH400-Piecewise-Curve.phtml
  
  // NOTE: You need to set analogPin to input in your setup block
  //   ex. pinMode(<analogPin>, INPUT);
  //   replace <analogPin> with the number of the pin you're going to read from.

  struct VH400 result;
  
  // Sums for calculating statistics
  int sensorDNsum = 0;
  double sensorVoltageSum = 0.0;
  double sensorVWCSum = 0.0;
  double sqDevSum_DN = 0.0;
  double sqDevSum_volts = 0.0;
  double sqDevSum_VWC = 0.0;

  // Arrays to hold multiple measurements
  int sensorDNs[nMeasurements];
  double sensorVoltages[nMeasurements];
  double sensorVWCs[nMeasurements];

  // Make measurements and add to arrays
  for (int i = 0; i < nMeasurements; i++) { 
    // Read value and convert to voltage 
    int sensorDN = analogRead(analogPin);
    double sensorVoltage = sensorDN*(3.0 / 1023.0);
        
    // Calculate VWC
    float VWC;
    if(sensorVoltage <= 1.1) {
      VWC = 10*sensorVoltage-1;
    } else if(sensorVoltage > 1.1 && sensorVoltage <= 1.3) {
      VWC = 25*sensorVoltage-17.5;
    } else if(sensorVoltage > 1.3 && sensorVoltage <= 1.82) {
      VWC = 48.08*sensorVoltage-47.5;
    } else if(sensorVoltage > 1.82) {
      VWC = 26.32*sensorVoltage-7.89;
    }

    // Add to statistics sums
    sensorDNsum += sensorDN;
    sensorVoltageSum += sensorVoltage;
    sensorVWCSum += VWC;

    // Add to arrays
    sensorDNs[i] = sensorDN;
    sensorVoltages[i] = sensorVoltage;
    sensorVWCs[i] = VWC;

    // Wait for next measurement
    delay(delayBetweenMeasurements);
  }

  // Calculate means
  double DN_mean = double(sensorDNsum)/double(nMeasurements);
  double volts_mean = sensorVoltageSum/double(nMeasurements);
  double VWC_mean = sensorVWCSum/double(nMeasurements);

  // Loop back through to calculate SD
  for (int i = 0; i < nMeasurements; i++) { 
    sqDevSum_DN += pow((DN_mean - double(sensorDNs[i])), 2);
    sqDevSum_volts += pow((volts_mean - double(sensorVoltages[i])), 2);
    sqDevSum_VWC += pow((VWC_mean - double(sensorVWCs[i])), 2);
  }
  double DN_stDev = sqrt(sqDevSum_DN/double(nMeasurements));
  double volts_stDev = sqrt(sqDevSum_volts/double(nMeasurements));
  double VWC_stDev = sqrt(sqDevSum_VWC/double(nMeasurements));

  // Setup the output struct
  result.analogValue = DN_mean;
  result.analogValue_sd = DN_stDev;
  result.voltage = volts_mean;
  result.voltage_sd = volts_stDev;
  result.VWC = VWC_mean;
  result.VWC_sd = VWC_stDev;

  // Return the result
  return(result);
}

void loop()
{
  WiFiClient client = server.available();   // listen for incoming clients

  if (client)
  {
    // if you get a client,
    Serial.println(F("New client"));           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client

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
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            client.print("Click <a href=\"/H\">here</a> turn the LED on pin 9 on<br>");
            client.print("Click <a href=\"/L\">here</a> turn the LED on pin 9 off<br>");

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
          digitalWrite(RED_LED, HIGH);               // GET /H turns the LED on
        }

        if (currentLine.endsWith("GET /L"))
        {
          digitalWrite(RED_LED, LOW);                // GET /L turns the LED off
        }

        if (currentLine.endsWith("GET /S"))
        {
          Serial.println(readVH400(MOISTURE_SENSOR_PIN));
        }
      }
    }

    // close the connection:
    client.stop();
    Serial.println(F("Client disonnected"));
  }
}

void initializeWiFi()
{
  while (!Serial && millis() < 5000);

  Serial.print(F("\nStart SimpleWebServerWiFi on "));
  Serial.println(WIFININA_GENERIC_VERSION);
  
  if (WiFi.status() == WL_NO_MODULE)
  {
    Serial.println(F("Communication with WiFi module failed!"));

    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();

  if (fv < WIFI_FIRMWARE_LATEST_VERSION)
  {
    Serial.print(F("Your current firmware NINA FW v"));
    Serial.println(fv);
    Serial.print(F("Please upgrade the firmware to NINA FW v"));
    Serial.println(WIFI_FIRMWARE_LATEST_VERSION);
  }

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED)
  {
    Serial.print(F("Attempting to connect to Network named: "));
    Serial.println(ssid);

    status = WiFi.begin(ssid, pass);
  }
  
  // Turn on the green LED to visually indicate we have a wifi connection.
  digitalWrite(GREEN_LED, HIGH);
}

void printWiFiStatus()
{
  // print the SSID of the network you're attached to:
  Serial.print(F("SSID: "));
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print(F("IP Address: "));
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print(F("Signal strength (RSSI):"));
  Serial.print(rssi);
  Serial.println(F(" dBm"));
  // print where to go in a browser:
  Serial.print(F("To see this page in action, open a browser to http://"));
  Serial.println(ip);
}
