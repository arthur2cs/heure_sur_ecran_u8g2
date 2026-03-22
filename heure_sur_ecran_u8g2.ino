#include <Arduino.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include "time.h"
#include "blink.h"
#include "digits.h"

// ── Écran ─────────────────────────────────────────────────────────────────────
U8G2_SSD1306_128X64_NONAME_F_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/ 22, /* data=*/ 21, /* cs=*/ 5, /* dc=*/ 4, /* reset=*/ 2);

// ── Configuration WiFi / NTP ──────────────────────────────────────────────────
const char* ssid              = "Livebox-1F20";
const char* password          = "uuaCwurTqKHF3xtwt7";
const char* ntpServer         = "pool.ntp.org";
const long  gmtOffset_sec     = 3600;
const int   daylightOffset_sec = 3600;

// ── Pins boutons ──────────────────────────────────────────────────────────────
const int btnNextPin = 15;   // Bouton "next"
const int btnMenuPin = 13;   // Bouton "menu / entrer"

// ── États de l'application ────────────────────────────────────────────────────
enum AppScreen { SCREEN_CLOCK, SCREEN_MENU, SCREEN_WIFI, SCREEN_PASSWORD };
AppScreen currentScreen = SCREEN_CLOCK;

// ── Menu principal ────────────────────────────────────────────────────────────
const int   MENU_ITEMS = 2;
const char* menuLabels[MENU_ITEMS] = { "Horloge / Anim", "Reseaux WiFi" };
int menuIndex = 0;

// ── Menu WiFi ─────────────────────────────────────────────────────────────────
int wifiNetworkCount = 0;
int wifiMenuIndex    = 0;   // 0 = flèche retour, 1..n = réseaux

// ── Clavier (layouts définis dans screen_password.ino) ───────────────────────
enum KbLayout { KB_LAY_LOWER, KB_LAY_UPPER, KB_LAY_SPECIAL };
KbLayout kbLayout      = KB_LAY_LOWER;
int      kbRow         = -1;
int      kbCol         = 0;
char     pwdBuffer[64] = "";
int      pwdLen        = 0;
int      pwdSelectedNet = -1;

// ── Horloge ───────────────────────────────────────────────────────────────────
struct tm    timeinfo;
bool         AnimationOn      = false;
unsigned long lastClockUpdate  = 0;
const unsigned long CLOCK_UPDATE_INTERVAL = 1000;

// ── Animation ─────────────────────────────────────────────────────────────────
int           animationCounter    = 0;
unsigned long lastAnimationUpdate = 0;
const unsigned long ANIMATION_FRAME_DELAY = 100;
unsigned long pauseDuration   = 0;
unsigned long pauseStartTime  = 0;
bool          isInPause       = false;
unsigned long delayMean       = 6000;
unsigned long delayStdDev     = 1000;

// ── Anti-rebond boutons ───────────────────────────────────────────────────────
int  lastNextState   = HIGH;
int  nextState       = HIGH;
int  lastMenuState   = HIGH;
int  menuBtnState    = HIGH;
unsigned long lastDebounceNext = 0;
unsigned long lastDebounceMenu = 0;
const unsigned long DEBOUNCE_DELAY = 50;

// ── Tableau bitmaps chiffres ──────────────────────────────────────────────────
const uint8_t* digitBitmaps[] = {
  digit0, digit1, digit2, digit3, digit4,
  digit5, digit6, digit7, digit8, digit9
};

// ── Setup / Loop ──────────────────────────────────────────────────────────────

void setup() {
  u8g2.begin();
  u8g2.setPowerSave(0);
  pinMode(btnNextPin, INPUT_PULLUP);
  pinMode(btnMenuPin, INPUT_PULLUP);
  NTPstart();
}

void loop() {
  handleButtons();

  if (currentScreen == SCREEN_CLOCK) {
    if (AnimationOn) {
      drawAnimation();
    } else {
      unsigned long currentMillis = millis();
      if (currentMillis - lastClockUpdate >= CLOCK_UPDATE_INTERVAL) {
        lastClockUpdate = currentMillis;
        LocalTime();
        drawHour(&timeinfo);
      }
    }
  }
}