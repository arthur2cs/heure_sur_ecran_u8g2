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

// Variables d'état
struct tm timeinfo;
const int buttonPin = 15;
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

// Gestion du bouton (anti-rebond)
int lastButtonState = HIGH;
int buttonState = HIGH;
unsigned long lastDebounceTime = 0;
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

void wifiScan() {
  u8g2.clearBuffer();
  drawMultiLineText("Scan WiFi...", 0, 0);
  u8g2.sendBuffer();

  int n = WiFi.scanNetworks();
  if (n == 0) {
    u8g2.clearBuffer();
    drawMultiLineText("Aucun reseau trouve", 0, 0);
    u8g2.sendBuffer();
    delay(5000);
    return;
  }

  // Affiche les réseaux trouvés un par un
  for (int i = 0; i < n; i++) {
    u8g2.clearBuffer();
    char msg[64];
    snprintf(msg, sizeof(msg), "%d: %s (%d dBm)", i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i));
    drawMultiLineText(msg, 0, 0);
    u8g2.sendBuffer();
    delay(2000);
  }
}

void NTPstart() {
  // Scan pour vérifier que le réseau est visible
  WiFi.mode(WIFI_STA);
  wifiScan();

  // Reset avant de (re)connecter
  WiFi.disconnect(true);
  delay(500);
  WiFi.begin(ssid, password);

  unsigned long wifiTimeout = millis();
  while (WiFi.status() != WL_CONNECTED) {
    wl_status_t status = WiFi.status();
    if (millis() - wifiTimeout > 15000) {  // Timeout après 15 secondes
      u8g2.clearBuffer();
      drawMultiLineText(wifiStatusStr(status), 0, 0);
      u8g2.sendBuffer();
      delay(3000);
      return;
    }
    u8g2.clearBuffer();
    char msg[64];
    snprintf(msg, sizeof(msg), "Wifi: %s", wifiStatusStr(status));
    drawMultiLineText(msg, 0, 0);
    u8g2.sendBuffer();
    delay(500);
  }

  // Initialize and synchronize the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}


void LocalTime() {
  if (!getLocalTime(&timeinfo)) {
    u8g2.clearBuffer();
    drawMultiLineText("Echec d'aquisition de l'heure", 0, 0);
    u8g2.sendBuffer();
  }
}

// Fonction de gestion du bouton avec anti-rebond
void handleButton() {
  int reading = digitalRead(buttonPin);
  
  // Si l'état du bouton a changé, réinitialiser le timer de debounce
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  // Si le temps de debounce est écoulé
  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    // Si l'état du bouton a vraiment changé
    if (reading != buttonState) {
      buttonState = reading;
      
      // Si le bouton est pressé (LOW car INPUT_PULLUP)
      if (buttonState == LOW) {
        AnimationOn = !AnimationOn;
        // Réinitialiser les compteurs lors du changement de mode
        if (AnimationOn) {
          animationCounter = 0;
          isInPause = false;
        }
      }
    }
  }
  
  lastButtonState = reading;
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
  pinMode(buttonPin, INPUT_PULLUP);
  NTPstart();

}

void loop() {
  // Toujours vérifier l'état du bouton
  handleButton();
  
  if (AnimationOn) {
    // Mode animation
    drawAnimation();
  } else {
    // Mode horloge - mise à jour toutes les secondes
    unsigned long currentMillis = millis();
    if (currentMillis - lastClockUpdate >= CLOCK_UPDATE_INTERVAL) {
      lastClockUpdate = currentMillis;
      LocalTime();
      drawHour(&timeinfo);
    }
  }
}