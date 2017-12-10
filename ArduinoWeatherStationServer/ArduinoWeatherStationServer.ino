#include <EasyNTPClient.h>
#include <Twitter.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SD.h>
#include <SPI.h>
#include <DS1302.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define RST_PIN A0
#define DAT_PIN A1
#define CLK_PIN A2
#define ETH_CS 10
#define SDC_CS 4

#define DATA_PORT 6432
#define NTP_PORT 8888

const int NTP_PACKET_SIZE = 48;
byte packetBuffer[ NTP_PACKET_SIZE];

DS1302 rtc(RST_PIN, DAT_PIN, CLK_PIN);
short timeH = 0, timeM = 0, timeS = 0;
long timeU = 0;

File curFile;

Twitter twitter("");

byte mac[] = {0x78, 0x7B, 0x8A, 0xB4, 0xA6, 0xBA};
IPAddress ip(192, 168, 0, 116);
IPAddress dns1(137, 149, 3, 1);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
EthernetServer server(DATA_PORT);
EthernetClient client;
EthernetUDP Udp;

byte node[][4] = {{0, 0, 0, 0}};

LiquidCrystal_I2C lcd(0x38, 20, 4);


void setup() {
  pinMode(SDC_CS, OUTPUT);
  pinMode(ETH_CS, OUTPUT);

  Serial.begin(9600);
  while (!Serial);
  lcd.init();
  setSDC();
  while (!SD.begin(4));
  setETH();
  if (Ethernet.begin(mac) == 0) {
    Ethernet.begin(mac, ip, dns1, gateway, subnet);
  }
  server.begin();

  //correct time on first boot
  correctTime();

  //setup display
  lcd.backlight();
  lcd.setCursor(12, 0);
  lcd.print("00:00:00");

  lcd.setCursor(0, 2);
  lcd.print("Avg Tem:");
  lcd.setCursor(10, 2);
  lcd.print("Cur Tem:");
  lcd.setCursor(0, 3);
  lcd.print("Avg Hum:");
  lcd.setCursor(10, 3);
  lcd.print("Cur Hum:");

}


void loop() {
  tickClock();
  updateClockDisplay();

  if (((timeM % 30 == 0) && timeS == 0) && false) {
    //30 mins twitter
  }

  if ((true || timeM % 5 == 0) && timeS == 0 && false )
  {
    for (int i = 0; i < 2; i++)
      if (client.connect(IPAddress(node[i][0], node[i][1], node[i][2], node[i][3]), 80)) {
        Serial.println("connected");
        client.println("REQ STATUS");
        do {
          char c = client.read();
          Serial.print(c);
          delay(10);
        } while (client.available());
      }
  }

  //curFile = SD.open("test.txt", FILE_WRITE);
  //if (curFile) {
  //curFile.println();
  //   curFile.close();
  //} else {
  //   Serial.println("error opening test.txt");
  // }

  if (client) {
    client.stop();
    Serial.println("client disconnected");
  }

  // if (client.connect(timeserver, DATA_PORT)) {
  //   Serial.println("connected");
  //   // Make a HTTP request:
  //   client.println("GET /search?q=arduino HTTP/1.0");
  //   client.println();
  // }
  // else {
  //   // kf you didn't get a connection to the server:
  //   //Serial.println("connection failed");
  // }
}

void updateClockDisplay() {
  //update clock
  lcd.setCursor(12, 0);
  if (timeH < 10)lcd.print(0);
  lcd.print(timeH);
  lcd.setCursor(15, 0);
  if (timeM < 10)lcd.print(0);
  lcd.print(timeM);
  lcd.setCursor(18, 0);
  if (timeS < 10)lcd.print(0);
  lcd.print(timeS);
}

void tickClock() {
  Time timeT = rtc.time(); // parameterless, returns a Time object
  delay(10);
  if ((abs(timeS - timeT.sec) < 5) || ((timeS == 59) && (timeT.sec == 0)))
    timeS = timeT.sec;
  if ((abs(timeM - timeT.min) < 3) || ((timeM == 59) && (timeT.min == 0)))
    timeM = timeT.min;
  if ((abs(timeH - timeT.hr) < 2) || ((timeH == 23) && (timeT.hr == 0)))
    timeH = timeT.hr;
  Serial.println(String(' ') + timeT.hr + " " + timeT.min + " " + timeT.sec + " " + timeH + " " + timeM + " " + timeS);
}

void postToTwitter(String msg) {
  int len = msg.length();
  char cmsg[len];
  msg.toCharArray(cmsg, len);
  if (twitter.post(cmsg)) {
    int status = twitter.wait(&Serial);
    if (status == 200) {
      Serial.println("OK.");
    } else {
      Serial.print("Failed : code ");
      Serial.println(status);
    }
  } else {
    Serial.println("Connection failed.");
  }
}

void setETH() {
  digitalWrite(SDC_CS, HIGH);
  digitalWrite(ETH_CS, LOW);
}

void setSDC() {
  digitalWrite(SDC_CS, LOW);
  digitalWrite(ETH_CS, HIGH);
}

void correctTime() {
  //get time from server
  setETH();
  Udp.begin(NTP_PORT);
  do {
    sendNTPpacket("time.nist.gov");
    if (Udp.parsePacket() == 0)
      delay(5000);
  } while (Udp.parsePacket() == 0);
  Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
  unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
  unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
  unsigned long timeNTP = highWord << 16 | lowWord;
  Udp.stop();
  Serial.println(timeNTP);
  timeH = (timeNTP / 3600) % 24;
  timeM = (timeNTP / 60) % 60;
  timeS = (timeNTP % 60);

  //setup time system
  rtc.writeProtect(false);
  rtc.halt(false);
  //delay(1000);
  Time t(1900, 01, 01, timeH, timeM, timeS, Time::kMonday);
  rtc.time(t);
  //delay(1000);
  Time timeT = rtc.time(); // parameterless, returns a Time object
  delay(50);
  //timeH = timeT.hr % 24;
  //timeM = timeT.min % 60;
  //timeS = timeT.sec % 60;
}

void sendNTPpacket(char* address) {
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  //fill packet buffer
  packetBuffer[0] = 0b11100011;
  packetBuffer[1] = 0; //stratum/level
  packetBuffer[2] = 6; //polling interveral
  packetBuffer[3] = 0xEC; //percision
  packetBuffer[12]  = 49; 
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  //send packet
  Udp.beginPacket(address, 123);
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}