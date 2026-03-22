// ── buttons.ino ───────────────────────────────────────────────────────────────
// Lecture anti-rebond des boutons et routage selon l'écran actif

// Retourne true au front descendant (appui) avec anti-rebond
bool readButton(int pin, int& lastRaw, int& stableState, unsigned long& lastDebounce) {
  int reading = digitalRead(pin);
  if (reading != lastRaw) lastDebounce = millis();
  lastRaw = reading;
  if ((millis() - lastDebounce) > DEBOUNCE_DELAY) {
    if (reading != stableState) {
      stableState = reading;
      if (stableState == LOW) return true;
    }
  }
  return false;
}

// Avance d'une position dans la grille du clavier
void kbAdvanceCursor() {
  if (kbRow == -1) {
    kbRow = 0; kbCol = 0;
  } else {
    kbCol++;
    if (kbCol >= 10) {
      kbCol = 0;
      kbRow++;
      if (kbRow >= 4) kbRow = -1;
    }
  }
}

void handleButtons() {
  bool pressedNext = readButton(btnNextPin, lastNextState, nextState,    lastDebounceNext);
  bool pressedMenu = readButton(btnMenuPin, lastMenuState, menuBtnState, lastDebounceMenu);

  // Suivi appui long bouton Next (uniquement dans l'écran clavier)
  int rawNext = digitalRead(btnNextPin);
  if (currentScreen == SCREEN_PASSWORD) {
    if (rawNext == LOW) {
      if (!nextIsHeld) {
        // Front descendant déjà géré par readButton ; on démarre le chrono
        nextIsHeld     = true;
        nextPressStart = millis();
        lastAutoScroll = millis();
      } else if (millis() - nextPressStart >= LONG_PRESS_DELAY) {
        // Appui long actif : défilement auto
        if (millis() - lastAutoScroll >= AUTO_SCROLL_RATE) {
          lastAutoScroll = millis();
          kbAdvanceCursor();
          drawPasswordScreen();
        }
      }
    } else {
      nextIsHeld = false;
    }
  } else {
    nextIsHeld = false;
  }

  switch (currentScreen) {

    // ── Horloge / Animation ───────────────────────────────────────────────────
    case SCREEN_CLOCK:
      if (pressedMenu) {
        menuIndex = 0;
        currentScreen = SCREEN_MENU;
        drawMainMenu();
      }
      if (pressedNext) {
        AnimationOn = !AnimationOn;
        if (AnimationOn) { animationCounter = 0; isInPause = false; }
      }
      break;

    // ── Menu principal ────────────────────────────────────────────────────────
    case SCREEN_MENU:
      if (pressedNext) {
        menuIndex = (menuIndex + 1) % MENU_ITEMS;
        drawMainMenu();
      }
      if (pressedMenu) {
        if (menuIndex == 0) {
          currentScreen = SCREEN_CLOCK;
        } else if (menuIndex == 1) {
          u8g2.clearBuffer();
          drawMultiLineText("Scan WiFi...", 0, 0);
          u8g2.sendBuffer();
          wifiNetworkCount = WiFi.scanNetworks();
          wifiMenuIndex    = 0;
          currentScreen    = SCREEN_WIFI;
          drawWifiMenu();
        }
      }
      break;

    // ── Liste des réseaux WiFi ────────────────────────────────────────────────
    case SCREEN_WIFI:
      if (pressedNext) {
        wifiMenuIndex = (wifiMenuIndex + 1) % (wifiNetworkCount + 1);
        drawWifiMenu();
      }
      if (pressedMenu) {
        if (wifiMenuIndex == 0) {
          menuIndex = 0;
          currentScreen = SCREEN_MENU;
          drawMainMenu();
        } else {
          pwdSelectedNet  = wifiMenuIndex - 1;
          pwdLen          = 0;
          pwdBuffer[0]    = '\0';
          kbRow           = -1;
          kbCol           = 0;
          kbLayout        = KB_LAY_LOWER;
          currentScreen   = SCREEN_PASSWORD;
          drawPasswordScreen();
        }
      }
      break;

    // ── Saisie du mot de passe ────────────────────────────────────────────────
    case SCREEN_PASSWORD:
      if (pressedNext) {
        // Simple clic : avance d'une touche (le défilement auto est géré au-dessus)
        kbAdvanceCursor();
        drawPasswordScreen();
      }
      if (pressedMenu) {
        if (kbRow == -1) {
          currentScreen = SCREEN_WIFI;
          drawWifiMenu();
        } else {
          kbPressKey();
        }
      }
      break;
  }
}
