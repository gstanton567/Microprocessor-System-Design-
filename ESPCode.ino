#include <ESP8266WiFi.h>

// Replace with your network credentials
const char* ssid     = "gstanton11";
const char* password = "gstanton11";

WiFiServer server(80);
String header;

unsigned long currentTime = millis();
unsigned long previousTime = 0; 
const long timeoutTime = 2000;

void setup() {
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  //Predefined Delay length, this way we don't get caught in a loop
  delay(10000);
  if(WiFi.status() == WL_CONNECTED)
  {
      server.begin();
  }else
  {
    Serial.println("NO CONNECTION");
  }
}

void loop(){
  WiFiClient client = server.available();
  String stats = String(7);
  if(Serial.available() > 0)
  {
    stats = Serial.readStringUntil('\n');
    if(stats == "Status"){
      if(WiFi.status() != WL_CONNECTED) {
        Serial.print("Wi-Fi Not Connected\n");
      } else{
        Serial.println("");
        Serial.println("WiFi connected.");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
      }
    }
  }
  if (client) {                             // If a new client connects,
    String currentLine = "";                // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
      currentTime = millis();         
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // turns the GPIOs on and off
            if (header.indexOf("GET /1/") >= 0) {
              Serial.write("1");
            } else if (header.indexOf("GET /2/") >= 0) {
              Serial.write("2");
            } else if (header.indexOf("GET /3/") >= 0) {
              Serial.write("3");
            } else if (header.indexOf("GET /4/") >= 0) {
              Serial.write("4");
            }else if (header.indexOf("GET /5/") >= 0) {
              Serial.write("5");
            }else if (header.indexOf("GET /6/") >= 0) {
              Serial.write("6");
            }else if (header.indexOf("GET /7/") >= 0) {
              Serial.write("7");
            }else if (header.indexOf("GET /8/") >= 0) {
              Serial.write("8");
            }else if (header.indexOf("GET /9/") >= 0) {
              Serial.write("9");
            }else if (header.indexOf("GET /A/") >= 0) {
              Serial.write("A");
            }else if (header.indexOf("GET /B/") >= 0) {
              Serial.write("B");
            }else if (header.indexOf("GET /C/") >= 0) {
              Serial.write("C");
            }else if (header.indexOf("GET /D/") >= 0) {
              Serial.write("D");
            }else if (header.indexOf("GET /E/") >= 0) {
              Serial.write("E");
            }else if (header.indexOf("GET /F/") >= 0) {
              Serial.write("F");
            }else if (header.indexOf("GET /0/") >= 0) {
              Serial.write("0");
            }
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button{background-color: #000000; border: none; color: white; padding: 4px 10px;text-decoration: none;");
            client.println("font-size: 15px; margin: 4px; cursor: pointer;}.button2 {background-color: #77878A;}</style>");
            client.println("</head><body><h1>ECEN-4330 Board </h1>");
            client.println("<p><a href=\"/1/\"><button class=\"button\">1</button></a><a href=\"/2/\"><button class=\"button\">2</button>");
            client.println("<a href=\"/3/\"><button class=\"button\">3</button></a><a href=\"/A/\"><button class=\"button\">A</button></a></p>");
            client.println("<p><a href=\"/4/\"><button class=\"button\">4</button></a><a href=\"/5/\"><button class=\"button\">5</button>");
            client.println("<a href=\"/6/\"><button class=\"button\">6</button></a><a href=\"/B/\"><button class=\"button\">B</button></a></p>");
            client.println("<p><a href=\"/7/\"><button class=\"button\">7</button></a><a href=\"/8/\"><button class=\"button\">8</button>");
            client.println("<a href=\"/9/\"><button class=\"button\">9</button></a><a href=\"/C/\"><button class=\"button\">C</button></a></p>");
            client.println("<p><a href=\"/F/\"><button class=\"button\">F</button></a><a href=\"/0/\"><button class=\"button\">0</button>");
            client.println("<a href=\"/E/\"><button class=\"button\">E</button></a><a href=\"/D/\"><button class=\"button\">D</button></a></p>");
            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
  }
}
