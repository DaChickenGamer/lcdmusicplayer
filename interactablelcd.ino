#include <IRremote.hpp>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------------- LCD ----------------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------------- PINS ----------------
const int IR_PIN = 27;
const int xAxis = 34;
const int audioPin = 25;
const int ledPin = 32;

// ---------------- STATE ----------------
enum MenuState { SELECT_GENRE, SELECT_SONG, PLAYING };
MenuState menuState = SELECT_GENRE;

String genres[] = {"8-Bit", "Spooky", "Heroic"};

String songs[3][2] = {
  {"Mario", "Tetris"},
  {"Ghost", "Skull"},
  {"Quest", "Victory"}
};

int currentGenre = -1;
int currentSong = 0;

// ---------------- IR ----------------
#define IR_1  0x0C
#define IR_2  0x18
#define IR_3  0x5E
#define IR_PWR 0x45
#define IR_PLUS 0x15
#define IR_MINUS 0x09
#define IR_PLAY 0x44

// ---------------- AUDIO ----------------
int volume = 5; // 0–10

int marioMelody[]   = {659, 659, 0, 659, 0, 523, 659, 784};
int tetrisMelody[]  = {659, 494, 523, 587, 523, 494, 440, 440};
int ghostMelody[]   = {392, 0, 392, 330, 392, 0, 262, 0};
int skullMelody[]   = {262, 294, 330, 294, 262, 0, 196, 0};
int questMelody[]   = {440, 494, 523, 587, 659, 587, 523, 494};
int victoryMelody[] = {784, 659, 523, 659, 784, 880, 784, 0};

// ---------------- AUDIO ENGINE ----------------
int* melody;
int noteIndex = 0;
unsigned long noteTimer = 0;
bool playingSong = false;

// ---------------- LED STATE ----------------
int ledBrightness = 0;
bool ledFlashActive = false;
unsigned long ledFlashTimer = 0;

// ---------------- LEDC CHANNELS ----------------
const int audioChannel = 0;
const int ledChannel = 1;

// ---------------- HELPERS ----------------
int volumeToAmp() {
  return map(volume, 0, 10, 0, 255);
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();

  pinMode(xAxis, INPUT);

  IrReceiver.begin(IR_PIN, ENABLE_LED_FEEDBACK);

  // ESP32 v3 LEDC setup
  ledcAttach(ledPin, 5000, 8);
  ledcAttach(audioPin, 2000, 8);

  updateDisplay();
}

// ---------------- LOOP ----------------
void loop() {
  handleIR();
  handleJoystick();
  updateSong();
  updateLedFlash();
}

// ---------------- IR ----------------
void handleIR() {
  if (!IrReceiver.decode()) return;

  uint8_t cmd = IrReceiver.decodedIRData.command;

  switch (cmd) {

    case IR_1:
      currentGenre = 0;
      currentSong = 0;
      menuState = SELECT_SONG;
      break;

    case IR_2:
      currentGenre = 1;
      currentSong = 0;
      menuState = SELECT_SONG;
      break;

    case IR_3:
      currentGenre = 2;
      currentSong = 0;
      menuState = SELECT_SONG;
      break;

    case IR_PWR:
      currentGenre = -1;
      menuState = SELECT_GENRE;
      break;

    case IR_PLUS:
      volume++;
      if (volume > 10) volume = 10;
      triggerLedFlash();
      break;

    case IR_MINUS:
      volume--;
      if (volume < 0) volume = 0;
      triggerLedFlash();
      break;

    case IR_PLAY:
      if (menuState == SELECT_SONG && currentGenre >= 0) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Starting...");
        delay(300);
        startSong();
      }
      break;
  }

  IrReceiver.resume();
  updateDisplay();
}

// ---------------- JOYSTICK ----------------
void handleJoystick() {
  if (menuState != SELECT_SONG) return;

  int xVal = analogRead(xAxis);

  if (xVal < 1000) currentSong = 0;
  if (xVal > 3000) currentSong = 1;

  updateDisplay();
}

// ---------------- START SONG ----------------
void startSong() {

  if (currentGenre == 0 && currentSong == 0) melody = marioMelody;
  else if (currentGenre == 0 && currentSong == 1) melody = tetrisMelody;
  else if (currentGenre == 1 && currentSong == 0) melody = ghostMelody;
  else if (currentGenre == 1 && currentSong == 1) melody = skullMelody;
  else if (currentGenre == 2 && currentSong == 0) melody = questMelody;
  else melody = victoryMelody;

  noteIndex = 0;
  noteTimer = millis();
  playingSong = true;

  menuState = PLAYING;
  updateDisplay();
}

// ---------------- AUDIO LOOP ----------------
void updateSong() {
  if (!playingSong) return;

  if (millis() - noteTimer > 300) {

    noteTimer = millis();
    noteIndex++;

    if (noteIndex >= 8) {
      playingSong = false;
      ledcWrite(audioPin, 0);
      menuState = SELECT_SONG;
      updateDisplay();
      return;
    }

    int freq = melody[noteIndex];

    if (freq > 0 && volume > 0) {
      ledcWriteTone(audioPin, freq);
      ledcWrite(audioPin, volumeToAmp());
    } else {
      ledcWrite(audioPin, 0);
    }

    // LED ALWAYS mirrors volume (but NOT forced in loop anymore)
    ledBrightness = volumeToAmp();
    ledcWrite(ledPin, ledBrightness);
  }
}

// ---------------- LED FLASH (100ms on volume change) ----------------
void triggerLedFlash() {
  ledBrightness = volumeToAmp();
  ledcWrite(ledPin, ledBrightness);

  ledFlashActive = true;
  ledFlashTimer = millis();
}

void updateLedFlash() {
  if (ledFlashActive && millis() - ledFlashTimer > 100) {
    ledFlashActive = false;
    ledcWrite(ledPin, ledBrightness);
  }
}

// ---------------- DISPLAY ----------------
String lastLine0 = "";
String lastLine1 = "";

void updateDisplay() {

  String line0;
  String line1;

  if (menuState == SELECT_GENRE) {
    line0 = "Music App";
    line1 = "Pick Genre 1-3";
  }

  else if (menuState == SELECT_SONG && currentGenre >= 0) {
    line0 = genres[currentGenre];

    line1 = "";
    line1 += (currentSong == 0 ? ">" : " ");
    line1 += songs[currentGenre][0];
    line1 += " ";
    line1 += (currentSong == 1 ? ">" : " ");
    line1 += songs[currentGenre][1];
  }

  else {
    line0 = ">>> PLAYING <<<";
    line1 = songs[currentGenre][currentSong];
  }

  if (line0 != lastLine0) {
    lcd.setCursor(0, 0);
    lcd.print("                ");
    lcd.setCursor(0, 0);
    lcd.print(line0);
    lastLine0 = line0;
  }

  if (line1 != lastLine1) {
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print(line1);
    lastLine1 = line1;
  }
}