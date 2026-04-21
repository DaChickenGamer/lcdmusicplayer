#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

const int GREEN_LED_PIN = 4;
const int POTENTIOMETER_PIN = 33;
const int SPEAKER_PIN = 25;
const int IR_RECIEVE_PIN = 15;
const int xAxis = 34;
const int yAxis = 35;
const int btn = 32;

const int freq = 5000;
const int resolution = 12;

LiquidCrystal_I2C lcd(0x27, 16, 2);

IRrecv irrecv(IR_RECIEVE_PIN);
decode_results results;

String decodeKeyValue(uint64_t result);

void setup() {
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Hello!");

  ledcAttach(GREEN_LED_PIN, freq, resolution);

  pinMode(btn, INPUT);

  irrecv.enableIRIn();
}

void loop() {
  readValues();
  ledcWrite(GREEN_LED_PIN, 255)
  delay(100);
}

void readValues() {
  int potValue = analogRead(POTENTIOMETER_PIN);
  int xValue = analogRead(xAxis);
  int yValue = analogRead(yAxis);
  int btnValue = digitalRead(btn);

  Serial.printf("BTN: %d, X: %d, Y: %d, Pot: %d\n", btnValue, xValue, yValue, potValue);

  lcd.setCursor(0, 0);
  lcd.print("X:"); lcd.print(xValue);
  lcd.print(" Y:"); lcd.print(yAxis);
  lcd.setCursor(0, 1);
  lcd.print("P:"); lcd.print(potValue);
  lcd.print(" B:"); lcd.print(btnValue);

  if (irrecv.decode(&results)) {
    String key = decodeKeyValue(results.value);
    if (key != "ERROR") {
      Serial.println(key);
    }
    irrecv.resume();
  }
}

String decodeKeyValue(uint64_t result) {
  switch (result) {
    case 0xFF6897: return "0";
    case 0xFF30CF: return "1";
    case 0xFF18E7: return "2";
    case 0xFF7A85: return "3";
    case 0xFF10EF: return "4";
    case 0xFF38C7: return "5";
    case 0xFF5AA5: return "6";
    case 0xFF42BD: return "7";
    case 0xFF4AB5: return "8";
    case 0xFF52AD: return "9";
    case 0xFF906F: return "+";
    case 0xFFA857: return "-";
    case 0xFFE01F: return "EQ";
    case 0xFFB04F: return "U/SD";
    case 0xFF9867: return "CYCLE";
    case 0xFF22DD: return "PLAY/PAUSE";
    case 0xFF02FD: return "BACKWARD";
    case 0xFFC23D: return "FORWARD";
    case 0xFFA25D: return "POWER";
    case 0xFFE21D: return "MUTE";
    case 0xFF629D: return "MODE";
    case 0xFFFFFFFF: return "ERROR";
    default: return "ERROR";
  }
}