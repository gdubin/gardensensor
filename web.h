void handleHttpRequests(WiFiServer server, float reading)
{
  WiFiClient client = server.available();   // listen for incoming clients
  bool match = false;

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
            if (match)
            {
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type: application/json");
              client.println();
            }
            else
            {
              client.println("HTTP/1.1 404 Not Found");
              client.println("Content-type: application/json");
              client.println();

              doc["status"] = "NOT_FOUND";
            }

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

        if (currentLine.endsWith("GET /reading"))
        {
          match = true;

          doc["status"] = "OK";
          doc["moisture"] = reading;
        }

        if (currentLine.endsWith("GET /network"))
        {
            match = true;

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