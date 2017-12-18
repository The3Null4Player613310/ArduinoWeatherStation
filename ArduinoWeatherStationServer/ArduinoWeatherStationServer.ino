#include <IRremote.h>
#include <RedMP3HARDWARE.h>
#include <EasyNTPClient.h>
#include <Twitter.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>
#include <DS1302.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define CLK_PIN A0
#define DAT_PIN A1
#define RST_PIN A2
#define ETH_PIN 10
#define SDC_PIN 4
#define IRR_PIN 22

//define button codes
#define BTN_CHM 1045085//"fff25d" //ch-
#define BTN_CH 16736925//"ff629d" //ch
#define BTN_CHP 16769565//"ffe21d" //ch+
#define BTN_PREV 16720605//"ff22dd" //prev
#define BTN_NEXT 16712445//"ff02fd" //next
#define BTN_PLAYPAUSE 16761405//"ffc23d" //play/pause
#define BTN_M 16769055//"ffe01f" //-
#define BTN_P 16754775//"ffa857" //+
#define BTN_EQ 16748655//"ff906f" //eq
#define BTN_100P 16738407//"ff6867" //100+
#define BTN_200P 16756815//"ffb04f" //200+
#define BTN_0 16738455//"ff6897" //0
#define BTN_1 16724175//"ff30cf" //1
#define BTN_2 16718055//"ff18e7" //2
#define BTN_3 16743045//"ff7a85" //3
#define BTN_4 16716015//"ff10ef" //4
#define BTN_5 16726215//"ff38c7" //5
#define BTN_6 16734885//"ff5aa5" //6//re
#define BTN_7 16728765//"ff42bd" //7
#define BTN_8 16730805//"ff4ab5" //8
#define BTN_9 16732845//"ff52ad" //9

// define ports
#define DATA_PORT 6432
#define NTP_PORT 8888

//ntp related vars
const int NTP_PACKET_SIZE = 48;
byte packetBuffer[NTP_PACKET_SIZE];

//rtc related vars
DS1302 rtc(RST_PIN, DAT_PIN, CLK_PIN);
short timeH = 0, timeM = 0, timeS = 0;
short dateD = 0, dateM = 0, dateY = 0;
long timeU = 0;

//twitter vars
Twitter twitter("");

//networking vars
byte mac[] = {0x78, 0x7B, 0x8A, 0xB4, 0xA6, 0xBA};
IPAddress ip(192, 168, 0, 116);
IPAddress dns1(137, 149, 3, 1);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
//EthernetServer server(DATA_PORT);
EthernetClient client;
EthernetUDP Udp;
byte server[] = {47, 54, 204, 157};

//display vars
LiquidCrystal_I2C lcd(0x38, 20, 4);

//ir remote vars
IRrecv IR(IRR_PIN);
decode_results irResults;

//mp3 vars
MP3 mp3;// just ask for the library we got it.

//state machiene vars
byte state = 1;
//state vars
//error     0000 0000 000
//idle      0000 0001 001
//query     0000 0010 002
//running   0000 0100 004
//updating  0000 1000 008
//data vars
//temp      0001 0000 016
//humidity  0010 0000 032
//light     0100 0000 064
//undefined 1000 0000 128


void setup() {
  //pinMode(SDC_PIN, OUTPUT);
  pinMode(ETH_PIN, OUTPUT);

  //start serial connections
  Serial.begin(9600);
  mp3.begin();
  while (!Serial);
  lcd.init();
  setSDC();
  //while (!SD.begin(4));
  setETH();
  if (Ethernet.begin(mac) == 0) {
    Ethernet.begin(mac, ip, dns1, gateway, subnet);
  }
  //server.begin();

  //correct time on first boot
  correctTime();

  //start ir reciever
  IR.enableIRIn();

  //setup mp3
  delay(500);
  mp3.setVolume(0x1e); //safe max is 0x1e
  delay(50);
  mp3.playWithFileName(3, 2);

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

//note: twitter every 30mins

void loop() {
  //ram();
  //Serial.println("bing");
  tickClock();
  updateClockDisplay();
  parseIRCommand();
  /*
  if (client.connect(server, 6432)) {
    mp3.playWithFileName(4, 25);
    Serial.println("connected");
    //get temp
    Serial.println(getClientData("get temp"));
    //get humi
    Serial.println(getClientData("get humi"));
    //get lumi
    Serial.println(getClientData("get lumi"));
  } else Serial.println("connectionfailed");

  if (client) {
    client.stop();
    mp3.stopPlay();
    Serial.println("client disconnected");
  }*/
}

String getClientData(String input) {
  client.println(input);
  String output = "";
  while (!(client.peek() == '\r' || client.peek() == '\n')) {
    if (((client.peek() != -1) && !(client.peek() == '\r' || client.peek() == '\n')) and (ram() > 256)) //leak protection
      output = output + (char)client.read();
    else if (ram() <= 256)
      break;
    Serial.println(output);
  }
  while (client.peek() == '\r' || client.peek() == '\n') {
    delay(10);
    client.read();
  }
  Serial.println("bing");
  return output;
}

void parseIRCommand() {
  if (IR.decode(&irResults)) {

    //String code = String(irResults.value, HEX);
    //Serial.println(code);
    //iterate through list and find match then return command;

    switch (irResults.value) {
      case BTN_CHM:
        break;
      case BTN_CH:
        break;
      case BTN_CHP:
        break;
      case BTN_PREV:
        mp3.previousSong();
        break;
      case BTN_NEXT:
        mp3.nextSong();
        break;
      case BTN_PLAYPAUSE:
        mp3.stopPlay();
        break;
      case BTN_M:
        mp3.volumeDown();
        break;
      case BTN_P:
        mp3.volumeUp();
        break;
      case BTN_EQ:
        break;
      case BTN_100P:
        mp3.rewind();
        break;
      case BTN_200P:
        mp3.forward();
        break;
      case BTN_0:
        mp3.playWithFileName(4, 0);
        break;
      case BTN_1:
        mp3.playWithFileName(4, 1);
        break;
      case BTN_2:
        mp3.playWithFileName(4, 2);
        break;
      case BTN_3:
        mp3.playWithFileName(4, 3);
        break;
      case BTN_4:
        mp3.playWithFileName(4, 4);
        break;
      case BTN_5:
        mp3.playWithFileName(4, 5);
        break;
      case BTN_6:
        mp3.playWithFileName(4, 6);
        break;
      case BTN_7:
        mp3.playWithFileName(4, 7);
        break;
      case BTN_8:
        mp3.playWithFileName(4, 8);
        break;
      case BTN_9:
        mp3.playWithFileName(4, 9);
        break;
      default:
        Serial.println(irResults.value, HEX);
        //Serial.println("val:" + irResults.value);
        break;
    }
    IR.resume();
    delay(700);
  }
}

void sayNumber(int num) {

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
  if ((abs(timeS - timeT.sec) < 10) || ((timeS == 59) && (timeT.sec == 0)))
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
  digitalWrite(SDC_PIN, HIGH);
  digitalWrite(ETH_PIN, LOW);
}

void setSDC() {
  digitalWrite(SDC_PIN, LOW);
  digitalWrite(ETH_PIN, HIGH);
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
  //Serial.println(timeNTP);
  dateY = (timeNTP / 31557600);
  dateM = (0);
  dateD = (timeNTP / 86400);
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
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;

  //send packet
  Udp.beginPacket(address, 123);
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

int ram() {
  extern int __heap_start, *__brkval;
  int v;
  //Serial.print(F("RAM:\t"));
  //Serial.print((int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval));
  //Serial.println(F(" b"));
  return ((int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval));
}
