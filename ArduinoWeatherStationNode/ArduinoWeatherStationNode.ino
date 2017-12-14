#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_Sensor.h>
#include <LiquidCrystal_I2C.h>

#define DHTPIN 2
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
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
double temperature;
double humidity;
double brightness;


void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial);
  dht.begin();

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
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("TEMP: "));
      lcd.print(temperature);
      lcd.setCursor(0, 1);
      lcd.print(F("HUMI: "));
      lcd.print(humidity);
      //lcd.println(brightness);
      delay(500);
      break;
    case 2:
      //receiving
      input = "";
      while (!(Serial.peek() == '\r' || Serial.peek() == '\n')) {
        if ((Serial.peek() != -1) && !(Serial.peek() == '\r' || Serial.peek() == '\n'))
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
          temperature = (double)dht.readTemperature();
          state = (state & DATA_MASK) | 8;
          break;
        case 32:
          //humidity
          humidity = (double)dht.readHumidity();
          state = (state & DATA_MASK) | 8;
          break;
        case 64:
          //light
          brightness = 0;
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
          Serial.print(temperature);
          Serial.println(F(" END"));
          break;
        case 32:
          //humidity
          Serial.print(F("HUMI "));
          Serial.print(humidity);
          Serial.println(F(" END"));
          break;
        case 64:
          //light
          Serial.print(F("LUMI "));
          Serial.print(brightness);
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

