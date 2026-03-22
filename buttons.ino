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
          syncTime();  // se connecte, met l'heure à jour, bascule sur SCREEN_CLOCK
        } else if (menuIndex == 2) {
          // Heure manuelle : initialise les chiffres à l'heure courante
          getLocalTime(&timeinfo, 0);
          tsH1 = timeinfo.tm_hour / 10;
          tsH2 = timeinfo.tm_hour % 10;
          tsM1 = timeinfo.tm_min  / 10;
          tsM2 = timeinfo.tm_min  % 10;
          tsDigit = 4;  // démarre sur la flèche retour
          currentScreen = SCREEN_TIMESET;
          drawTimeSetScreen();
        } else if (menuIndex == 3) {
          u8g2.clearBuffer();
          drawMultiLineText("Scan WiFi...", 0, 0);
          u8g2.sendBuffer();
          wifiNetworkCount = WiFi.scanNetworks();
          wifiMenuIndex    = 0;
          currentScreen    = SCREEN_WIFI;
          drawWifiMenu();
        } else if (menuIndex == 4) {
          asRow = 0;
          asCol = 0;
          currentScreen = SCREEN_ANIMSET;
          drawAnimSetScreen();
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

    // ── Réglage heure manuelle ────────────────────────────────────────────────
    case SCREEN_TIMESET:
      if (pressedNext) {
        // Ordre : retour(4) → H1(0) → H2(1) → M1(2) → M2(3) → OK(5) → retour(4)...
        const int tsOrder[] = {4, 0, 1, 2, 3, 5};
        const int tsOrderLen = 6;
        int cur = 0;
        for (int i = 0; i < tsOrderLen; i++) { if (tsOrder[i] == tsDigit) { cur = i; break; } }
        tsDigit = tsOrder[(cur + 1) % tsOrderLen];
        drawTimeSetScreen();
      }
      if (pressedMenu) {
        if (tsDigit == 4) {
          currentScreen = SCREEN_MENU;
          drawMainMenu();
        } else if (tsDigit == 5) {
          tsApply();
        } else {
          tsIncrDigit();
          drawTimeSetScreen();
        }
      }
      break;

    // ── Réglage animation ──────────────────────────────────────────────────
    case SCREEN_ANIMSET:
      if (pressedNext) {
        // Ordre : retour(0) → moy<(1,0) → moy>(1,1) → écart<(2,0) → écart>(2,1) → OK(3) → retour(0)
        if (asRow == 0) {
          asRow = 1; asCol = 0;
        } else if (asRow == 1 && asCol == 0) {
          asCol = 1;
        } else if (asRow == 1 && asCol == 1) {
          asRow = 2; asCol = 0;
        } else if (asRow == 2 && asCol == 0) {
          asCol = 1;
        } else if (asRow == 2 && asCol == 1) {
          asRow = 3; asCol = 0;
        } else { // asRow == 3 (OK)
          asRow = 0; asCol = 0;
        }
        drawAnimSetScreen();
      }
      if (pressedMenu) {
        if (asRow == 0) {
          // Retour au menu sans sauvegarder
          currentScreen = SCREEN_MENU;
          drawMainMenu();
        } else if (asRow == 3) {
          // OK : sauvegarde et bascule sur l'horloge
          asSave();
          currentScreen = SCREEN_CLOCK;
        } else {
          // Incrémente ou décrémente la valeur sélectionnée
          unsigned long* val = (asRow == 1) ? &delayMean : &delayStdDev;
          const unsigned long STEP = 1000;
          const unsigned long VMIN = 1000;
          const unsigned long VMAX = 1000000;
          if (asCol == 0) {
            if (*val > VMIN) *val -= STEP; else *val = VMIN;
          } else {
            if (*val < VMAX) *val += STEP; else *val = VMAX;
          }
          drawAnimSetScreen();
        }
      }
      break;
  }
}
