# Garden Sensor

This repository contains the implementation details and code for a garden sensor.  Depending on the enclosure you use, it could be used inside or out.  The current implementation is strictly for soil moisture readings but, in the future, this will hopefully support temperature, humidity, light levels and perhaps some automated watering functionality.

The project currently leverages a Metro M4 Airlift Lite for WiFi support.  The corresponding code will start a webserver on port 80 and allow you to query for sensor readings.  The readings are returned in a JSON format.

*Example Exchanges TBD*
### Hardware

Parts

- [Arduino Metro M4 Airlift Lite](https://www.adafruit.com/product/4000)
- LED (Green)
- LED (Red)
- 220 ohm resistors (2)
- [VH400 Soil Moisture Sensor](https://vegetronix.com/soil-moisture-sensor)

Diagram


![[sketch.png]]

### Software

If you're new to Arduino development, please start [here](https://docs.arduino.cc/learn/starting-guide/getting-started-arduino/).

The repository consists of two files:

| File              | Description                                                                                                                                     |
| ----------------- | ----------------------------------------------------------------------------------------------------------------------------------------------- |
| gardensensor.ino  | This is the main source code that will need to be uploaded to your Arduino.                                                                     |
| arduino_secrets.h | This file will contain your local WiFi networks SSID and password.  Any changes to this file should not be persisted back into this repository. |

