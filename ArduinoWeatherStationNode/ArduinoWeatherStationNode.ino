#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_Sensor.h>
#include <LiquidCrystal_I2C.h>

#define DHTPIN 2
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x38, 16, 2);

String input = "";

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial);
  dht.begin();

  lcd.init();
  lcd.backlight();
}


void loop() {

  input = "";
  while (!(Serial.peek() == '\r' || Serial.peek() == '\n')) {
    if ((Serial.peek() != -1) && !(Serial.peek() == '\r' || Serial.peek() == '\n'))
      input = input + (char)Serial.read();
  }
  while (Serial.peek() == '\r' || Serial.peek() == '\n') {
    delay(10);
    Serial.read();
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(input);

  if (input.equalsIgnoreCase(F("GET TEMP"))) {
    Serial.print(F("TEMP "));
    Serial.print((double)dht.readTemperature());
    Serial.println(F(" END"));
  } else if (input.equalsIgnoreCase(F("GET HUMI"))) {
    Serial.print(F("HUMI "));
    Serial.print((double)dht.readHumidity());
    Serial.println(F(" END"));
  } else
    Serial.println(F("NAK"));
}

