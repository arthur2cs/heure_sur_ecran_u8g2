#include <Arduino.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include "time.h"
#include "blink.h"
#include "digits.h"

U8G2_SSD1306_128X64_NONAME_F_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/ 22, /* data=*/ 21, /* cs=*/ 5, /* dc=*/ 4, /* reset=*/ 2);

// Configuration WiFi et NTP
const char* ssid = "Livebox-1F20";
const char* password = "uuaCwurTqKHF3xtwt7";
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

// Pins boutons
const int btnNextPin = 15;   // Bouton "next" (existant)
const int btnMenuPin = 13;   // Bouton "menu / entrer" (nouveau)

// États de l'application
enum AppScreen { SCREEN_CLOCK, SCREEN_MENU, SCREEN_WIFI };
AppScreen currentScreen = SCREEN_CLOCK;

// Menu principal
const int MENU_ITEMS = 2;
const char* menuLabels[MENU_ITEMS] = { "Horloge / Anim", "Reseaux WiFi" };
int menuIndex = 0;  // item surligné

// Menu WiFi
int wifiNetworkCount = 0;
int wifiMenuIndex = 0;  // 0 = flèche retour, 1..n = réseaux

// Variables d'état horloge
struct tm timeinfo;
bool AnimationOn = false;

// Animation
int animationCounter = 0;
unsigned long lastAnimationUpdate = 0;
const unsigned long ANIMATION_FRAME_DELAY = 100;
unsigned long pauseDuration = 0;
unsigned long pauseStartTime = 0;
bool isInPause = false;
unsigned long delayMean = 6000;
unsigned long delayStdDev = 1000;

// Gestion des boutons (anti-rebond)
int lastNextState = HIGH;
int nextState    = HIGH;
int lastMenuState = HIGH;
int menuBtnState  = HIGH;
unsigned long lastDebounceNext = 0;
unsigned long lastDebounceMenu = 0;
const unsigned long DEBOUNCE_DELAY = 50;

// Gestion de l'horloge
unsigned long lastClockUpdate = 0;
const unsigned long CLOCK_UPDATE_INTERVAL = 1000;



// Tableau de correspondance chiffre -> bitmap
const uint8_t* digitBitmaps[] = {
  digit0,
  digit1,
  digit2,
  digit3,
  digit4,
  digit5,
  digit6,
  digit7,
  digit8,
  digit9
  // ... Ajoutez les correspondances pour les autres chiffres
};


const char* wifiStatusStr(wl_status_t status) {
  switch (status) {
    case WL_IDLE_STATUS:     return "Idle";
    case WL_NO_SSID_AVAIL:   return "Reseau introuvable";
    case WL_SCAN_COMPLETED:  return "Scan OK";
    case WL_CONNECTED:       return "Connecte";
    case WL_CONNECT_FAILED:  return "Mauvais mot de passe";
    case WL_CONNECTION_LOST: return "Connexion perdue";
    case WL_DISCONNECTED:    return "Deconnecte";
    default:                 return "Inconnu";
  }
}

void NTPstart() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(500);
  WiFi.begin(ssid, password);

  char msg[64];
  snprintf(msg, sizeof(msg), "Connexion a \"%s\"...", ssid);

  unsigned long wifiTimeout = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - wifiTimeout > 10000) {
      u8g2.clearBuffer();
      drawMultiLineText("Echec connexion WiFi", 0, 0);
      u8g2.sendBuffer();
      delay(2000);
      return;  // bascule sur l'horloge (00:00)
    }
    u8g2.clearBuffer();
    drawMultiLineText(msg, 0, 0);
    u8g2.sendBuffer();
    delay(500);
  }

  // Succès
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  u8g2.clearBuffer();
  drawMultiLineText("WiFi connecte !", 0, 0);
  u8g2.sendBuffer();
  delay(1500);  // bref message de succès puis horloge
}


void LocalTime() {
  if (!getLocalTime(&timeinfo)) {
    u8g2.clearBuffer();
    drawMultiLineText("Echec d'aquisition de l'heure", 0, 0);
    u8g2.sendBuffer();
  }
}

// ── Helpers boutons ───────────────────────────────────────────────────────────

// Retourne true si le bouton vient d'être pressé (front descendant, debounced)
bool readButton(int pin, int& lastRaw, int& stableState, unsigned long& lastDebounce) {
  int reading = digitalRead(pin);
  if (reading != lastRaw) lastDebounce = millis();
  lastRaw = reading;
  if ((millis() - lastDebounce) > DEBOUNCE_DELAY) {
    if (reading != stableState) {
      stableState = reading;
      if (stableState == LOW) return true;  // front descendant = appui
    }
  }
  return false;
}

// ── Dessin menu principal ─────────────────────────────────────────────────────

void drawMainMenu() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  uint8_t lineH = 13;
  uint8_t yStart = 10;

  // Titre
  u8g2.drawStr(0, yStart, "-- MENU --");

  for (int i = 0; i < MENU_ITEMS; i++) {
    uint8_t y = yStart + (i + 1) * lineH;
    if (i == menuIndex) {
      // Surbrillance : rectangle blanc, texte noir
      u8g2.setDrawColor(1);
      u8g2.drawBox(0, y - 9, 128, lineH);
      u8g2.setDrawColor(0);
      u8g2.drawStr(2, y, menuLabels[i]);
      u8g2.setDrawColor(1);
    } else {
      u8g2.drawStr(2, y, menuLabels[i]);
    }
  }
  u8g2.sendBuffer();
}

// ── Dessin menu WiFi ──────────────────────────────────────────────────────────

void drawWifiMenu() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  uint8_t lineH = 11;

  // Flèche retour (index 0) — toujours visible en haut à gauche
  if (wifiMenuIndex == 0) {
    u8g2.setDrawColor(1);
    u8g2.drawBox(0, 0, 20, lineH);
    u8g2.setDrawColor(0);
    u8g2.drawStr(2, 9, "<");
    u8g2.setDrawColor(1);
  } else {
    u8g2.drawStr(2, 9, "<");
  }

  // Liste des réseaux (index 1..wifiNetworkCount)
  // On affiche jusqu'à 4 réseaux, avec défilement
  int visibleStart = max(0, wifiMenuIndex - 4);  // décalage si beaucoup de réseaux
  for (int i = 0; i < min(wifiNetworkCount, 4); i++) {
    int netIdx = visibleStart + i;
    if (netIdx >= wifiNetworkCount) break;

    uint8_t y = lineH + (i + 1) * lineH;
    int listIndex = netIdx + 1;  // index dans le menu (0 = retour, 1+ = réseaux)

    char label[32];
    snprintf(label, sizeof(label), "%s (%d)", WiFi.SSID(netIdx).c_str(), WiFi.RSSI(netIdx));

    if (wifiMenuIndex == listIndex) {
      u8g2.setDrawColor(1);
      u8g2.drawBox(0, y - 9, 128, lineH);
      u8g2.setDrawColor(0);
      u8g2.drawStr(2, y, label);
      u8g2.setDrawColor(1);
    } else {
      u8g2.drawStr(2, y, label);
    }
  }
  u8g2.sendBuffer();
}

// ── Gestion des boutons selon l'écran actif ───────────────────────────────────

void handleButtons() {
  bool pressedNext = readButton(btnNextPin, lastNextState, nextState, lastDebounceNext);
  bool pressedMenu = readButton(btnMenuPin, lastMenuState, menuBtnState, lastDebounceMenu);

  switch (currentScreen) {

    case SCREEN_CLOCK:
      if (pressedMenu) {
        // Ouvrir le menu principal
        menuIndex = 0;
        currentScreen = SCREEN_MENU;
        drawMainMenu();
      }
      if (pressedNext) {
        // Comportement existant : toggle animation
        AnimationOn = !AnimationOn;
        if (AnimationOn) { animationCounter = 0; isInPause = false; }
      }
      break;

    case SCREEN_MENU:
      if (pressedNext) {
        menuIndex = (menuIndex + 1) % MENU_ITEMS;
        drawMainMenu();
      }
      if (pressedMenu) {
        if (menuIndex == 0) {
          // Horloge / Anim
          currentScreen = SCREEN_CLOCK;
        } else if (menuIndex == 1) {
          // Menu WiFi — on scanne d'abord
          u8g2.clearBuffer();
          drawMultiLineText("Scan WiFi...", 0, 0);
          u8g2.sendBuffer();
          wifiNetworkCount = WiFi.scanNetworks();
          wifiMenuIndex = 0;
          currentScreen = SCREEN_WIFI;
          drawWifiMenu();
        }
      }
      break;

    case SCREEN_WIFI:
      if (pressedNext) {
        wifiMenuIndex = (wifiMenuIndex + 1) % (wifiNetworkCount + 1);
        drawWifiMenu();
      }
      if (pressedMenu) {
        if (wifiMenuIndex == 0) {
          // Flèche retour → menu principal
          menuIndex = 0;
          currentScreen = SCREEN_MENU;
          drawMainMenu();
        }
        // (on pourrait connecter au réseau sélectionné dans une future version)
      }
      break;
  }
}

// Fonction pour obtenir le bitmap correspondant à un chiffre
const uint8_t* getDigitBitmap(int digit) {
  if (digit >= 0 && digit <= 9) {
    return digitBitmaps[digit];
  }
  return NULL;  // Retourne NULL si le chiffre n'a pas de bitmap correspondant
}

void drawDigit(const uint8_t* bitmap, uint8_t x, uint8_t y) {
  u8g2.drawXBMP(x, y, 25, 42, bitmap);
}

void drawHour(struct tm* timeinfo) {
  u8g2.clearBuffer();

  // Extract hours and minutes
  int hours = timeinfo->tm_hour;
  int minutes = timeinfo->tm_min;

  // Draw first digit of hours
  const uint8_t* hourDigit1Bitmap = getDigitBitmap(hours / 10);
  if (hourDigit1Bitmap != NULL) {
    drawDigit(hourDigit1Bitmap, 0, 11);
  }

  // Draw second digit of hours with 3 pixels spacing
  const uint8_t* hourDigit2Bitmap = getDigitBitmap(hours % 10);
  if (hourDigit2Bitmap != NULL) {
    drawDigit(hourDigit2Bitmap, 29, 11);
  }

  // Draw colon separator

  u8g2.drawXBMP(54, 11, 20, 42, colon);


  // Draw first digit of minutes with 3 pixels spacing
  const uint8_t* minuteDigit1Bitmap = getDigitBitmap(minutes / 10);
  if (minuteDigit1Bitmap != NULL) {
    drawDigit(minuteDigit1Bitmap, 74, 11);
  }

  // Draw second digit of minutes with 3 pixels spacing
  const uint8_t* minuteDigit2Bitmap = getDigitBitmap(minutes % 10);
  if (minuteDigit2Bitmap != NULL) {
    drawDigit(minuteDigit2Bitmap, 102, 11);
  }

  u8g2.sendBuffer();
}

void drawMultiLineText(const char* text, uint8_t startX, uint8_t startY) {
  u8g2.setFont(u8g2_font_ncenB08_tr);

  uint8_t cols = u8g2.getDisplayWidth() / u8g2.getMaxCharWidth();
  uint8_t lineHeight = u8g2.getFontAscent() - u8g2.getFontDescent();
  uint8_t currentLine = 1;

  const char* wordDelimiters = " \t\n\r";  // Define word delimiters

  char textCopy[strlen(text) + 1];
  strcpy(textCopy, text);

  char* token = strtok(textCopy, wordDelimiters);
  while (token != nullptr) {
    uint8_t textWidth = u8g2.getStrWidth(token);
    if (startX + textWidth > u8g2.getDisplayWidth()) {
      startX = 0;
      currentLine++;
    }
    u8g2.setCursor(startX, startY + currentLine * lineHeight);
    u8g2.print(token);
    startX += textWidth + u8g2.getMaxCharWidth(); // Add a space width

    // Move to the next line if the current line is full
    if (startX >= u8g2.getDisplayWidth()) {
      startX = 0;
      currentLine++;
    }

    token = strtok(nullptr, wordDelimiters);
  }
}

int randomGaussianPositive(int mean, int stdDev) {
  int value;
  do {
    double u1 = random(32767) / 32768.0; // random number between 0 and 1
    double u2 = random(32767) / 32768.0; // random number between 0 and 1

    double z = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2); // Box-Muller transform

    value = static_cast<int>(mean + stdDev * z);
  } while (value < 0);

  return value;
}

void drawAnimation() {
  unsigned long currentMillis = millis();
  
  // Gestion de la pause
  if (isInPause) {
    if (currentMillis - pauseStartTime < pauseDuration) {
      return; // On reste en pause
    } else {
      // Fin de la pause
      isInPause = false;
      animationCounter++;
      lastAnimationUpdate = currentMillis;
    }
  }
  
  // Mise à jour de l'animation
  if (currentMillis - lastAnimationUpdate >= ANIMATION_FRAME_DELAY) {
    lastAnimationUpdate = currentMillis;
    
    u8g2.clearBuffer();
    
    int imageIndex = animationCounter % 11;
    switch (imageIndex) {
      case 0:
        u8g2.drawXBMP(0, 0, 128, 64, arcade1);
        break;
      case 1:
        u8g2.drawXBMP(0, 0, 128, 64, arcade2);
        break;
      case 2:
        u8g2.drawXBMP(0, 0, 128, 64, arcade3);
        break;
      case 3:
        u8g2.drawXBMP(0, 0, 128, 64, arcade4);
        break;
      case 4:
        u8g2.drawXBMP(0, 0, 128, 64, arcade5);
        break;
      case 5:
        u8g2.drawXBMP(0, 0, 128, 64, arcade6);
        break;
      case 6:
        u8g2.drawXBMP(0, 0, 128, 64, arcade5);
        break;
      case 7:
        u8g2.drawXBMP(0, 0, 128, 64, arcade4);
        break; 
      case 8:
        u8g2.drawXBMP(0, 0, 128, 64, arcade3);
        break;
      case 9:
        u8g2.drawXBMP(0, 0, 128, 64, arcade2);
        break;
      case 10:
        u8g2.drawXBMP(0, 0, 128, 64, arcade1);
        // Démarrer une pause aléatoire après la dernière frame
        pauseDuration = randomGaussianPositive(delayMean, delayStdDev);
        pauseStartTime = currentMillis;
        isInPause = true;
        break;
    }
    
    u8g2.sendBuffer();
    
    // Incrémenter le compteur sauf pour la frame 10 (qui gère sa propre incrémentation après la pause)
    if (imageIndex != 10) {
      animationCounter++;
    }
  }
}

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