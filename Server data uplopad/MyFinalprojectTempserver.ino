#include <SPI.h>
#include <SD.h>

#include <DS1302.h>

#define RST 6
#define DAT 5
#define CLK 3
#include <Adafruit_Sensor.h>

#include <DHT.h>
//#include <DHT_U.h>

/*
  Web Server

 A simple web server that shows the value of the analog input pins.
 using an Arduino Wiznet Ethernet shield.

 Circuit:
 * Ethernet shield attached to pins 10, 11, 12, 13
 * Analog inputs attached to pins A0 through A5 (optional)

 created 18 Dec 2009
 by David A. Mellis
 modified 9 Apr 2012
 by Tom Igoe
 modified 02 Sept 2015
 by Arturo Guadalupi

 */

#include <SPI.h>
#include <Ethernet.h>

#define DHTPIN 2     // what digital pin we're connected to
#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT22   

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);

File myFile;

DS1302 rtc(RST, DAT, CLK);
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  while(!SD.begin(4))
  dht.begin();

  // start the Ethernet connection and the server:
  Ethernet.begin(mac);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
  delay(1000);  //change
}


void loop() {
  // listen for incoming clients
  EthernetClient client = server.available();
      Time ti = rtc.time(); // parameterless, returns a Time object
      float te = dht.readTemperature();
      float hu= dht.readHumidity();
      

  // if the file opened okay, write to it:
  if(millis()%1000>990){
  myFile = SD.open("test.txt", FILE_WRITE);
  if (myFile&&!(isnan(te)||isnan(hu))) {
    Serial.print("Writing to test.txt...");
    myFile.println(String(ti.yr) + "-" + String(ti.mon) + "-" + String(ti.date) + " " + String(ti.hr) + ":" + String(ti.min) + ":" + String(ti.sec)+","+te+","+hu);
    // close the file:
    myFile.close();
    Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }
  }
  
  if (client) {
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          // output the value of each analog input pin
          //for (int analogChannel = 0; analogChannel < 6; analogChannel++) {
          //  int sensorReading = analogRead(analogChannel);
          //  client.print("analog input ");
          //  client.print(analogChannel);
          //  client.print(" is ");
          //  client.print(sensorReading);
          // client.println("<br />");
          //}
          client.println("<table border=\"1\">");
          client.println("<tr><th>time stamp</th><th>temp</th><th>humidity</th></tr>");
          //loop begin

          myFile = SD.open("test.txt");
          if (myFile) {
            Serial.println("test.txt:");

            // read from the file until there's nothing else in it:
            client.print("<tr><td>");
          char c;
          while (myFile.available()) {
            if(c=='\n')
              client.print("<tr><td>");
            c  = myFile.read();
            if(c == ',')
              client.print("</td><td>");
            else if(c=='\r'){
              c=myFile.read();
              client.println("</td></tr>");
            }
            else
              client.print(c);
           }
           client.print("</table>");
         // close the file:
          myFile.close();
          } else {
          // if the file didn't open, print an error:
            Serial.println("error opening test.txt");
            }
            
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}

