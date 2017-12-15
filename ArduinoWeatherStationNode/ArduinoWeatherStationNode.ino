#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_Sensor.h>
#include <LiquidCrystal_I2C.h>


#define LSV_PIN A0
#define LMT_PIN A1
#define DHT_PIN 2
#define DHT_TYPE DHT11

DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(0x38, 16, 2);

const byte STAT_MASK = 15;
const byte DATA_MASK = 240;
byte state = 1;
//state vars
//error     0000 0000 000
//idle      0000 0001 001
//receiving 0000 0010 002
//running   0000 0100 004
//sending   0000 1000 008
//data vars
//temp      0001 0000 016
//humidity  0010 0000 032
//light     0100 0000 064
//undefined 1000 0000 128

String input = "";
String output = "";
short msgTemperature;
short msgHumidity;
short msgBrightness;
short rawTemperature;
short rawHumidity;
short rawBrightness;


void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  //setup wifi
  while (!Serial);
  //setup dht
  dht.begin();
  //setup lcd
  lcd.init();
  lcd.backlight();
}

void loop() {
  //delay(100);
  //Serial.println((state & DATA_MASK));
  //Serial.println((state & STAT_MASK));
  switch (state & STAT_MASK) {
    case 1:
      //idle
      if (Serial.available() > 0)
        state = (state & DATA_MASK) | 2;

      //update raw values
      Serial.println(String() + (double)dht.readTemperature() + " " + (analogRead(LMT_PIN) * (500.0 / 1024.0)) + " " + (abs((analogRead(LMT_PIN) * (-500.0 / 1024.0))-(double)dht.readTemperature())-40));
      rawTemperature = ((double)dht.readTemperature());// + (analogRead(LMT_PIN) * (500.0 / 1024.0) - 50.0);
      rawHumidity = dht.readHumidity();
      rawBrightness = analogRead(LSV_PIN);

      //update display
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("TEMP: "));
      lcd.print(rawTemperature);
      lcd.setCursor(0, 1);
      lcd.print(F("HUMI: "));
      lcd.print(rawHumidity);
      //lcd.println(brightness);
      delay(250);
      break;
    case 2:
      //receiving
      input = "";
      while (!(Serial.peek() == '\r' || Serial.peek() == '\n')) {
        if (((Serial.peek() != -1) && !(Serial.peek() == '\r' || Serial.peek() == '\n')) and (ram() > 256)) //leak protection
          input = input + (char)Serial.read();
      }
      while (Serial.peek() == '\r' || Serial.peek() == '\n') {
        delay(10);
        Serial.read();
      }


      if ((state & DATA_MASK) == 0) {
        if (input.equalsIgnoreCase(F("GET TEMP")))
          state = 16 | 4;
        else if (input.equalsIgnoreCase(F("GET HUMI")))
          state = 32 | 4;
        else if (input.equalsIgnoreCase(F("GET LUMI")))
          state = 64 | 4;
        else if (input.equalsIgnoreCase(F("GET FUMI")))
          state = 128 | 4;
        else
          state = 1;
      } else {
        if (input.equalsIgnoreCase(F("ACK")))
          state = 1;
        else if (input.equalsIgnoreCase(F("NACK")))
          state = (state & DATA_MASK) | 8;
      }

      break;
    case 4:
      //running
      switch (state & DATA_MASK) {
        case 16:
          //temp
          msgTemperature = rawTemperature;
          state = (state & DATA_MASK) | 8;
          break;
        case 32:
          //humidity
          msgHumidity = rawHumidity;
          state = (state & DATA_MASK) | 8;
          break;
        case 64:
          //light
          msgBrightness = rawBrightness;
          state = (state & DATA_MASK) | 8;
          break;
        case 128:
          //undefined
          state = (state & DATA_MASK) | 8;
          break;
        case 0:
        default:
          //error
          Serial.println(F("ERROR: in running"));
          state = 0;
          break;
      }
      break;
    case 8:
      //sending
      state = (state & DATA_MASK) | 1;
      switch (state & DATA_MASK) {
        case 16:
          //temp
          Serial.print(F("TEMP "));
          Serial.print(msgTemperature);
          Serial.println(F(" END"));
          break;
        case 32:
          //humidity
          Serial.print(F("HUMI "));
          Serial.print(msgHumidity);
          Serial.println(F(" END"));
          break;
        case 64:
          //light
          Serial.print(F("LUMI "));
          Serial.print(msgBrightness);
          Serial.println(F(" END"));
          break;
        case 128:
          //undefined
          Serial.print(F("FUMI "));
          Serial.print("undefined");
          Serial.println(F(" END"));
          break;
        case 0:
        default:
          //error
          Serial.println(F("ERROR: in sending"));
          state = 0;
          break;
      }
      break;
    case 0:
    default:
      //error
      Serial.println(F("ERROR: in main"));
      break;
  }
}

int ram() {
  extern int __heap_start, *__brkval;
  int v;
  //Serial.print(F("RAM:\t"));
  //Serial.print((int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval));
  //Serial.println(F(" b"));
  return ((int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval));
}

