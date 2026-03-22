// ── screen_timeset.ino ────────────────────────────────────────────────────────
// Réglage manuel de l'heure : 4 chiffres sélectionnables + retour / valider
//
// tsDigit :  0=H1  1=H2  2=M1  3=M2  4=bouton retour  5=bouton valider
// Navigation : Next avance la sélection (boucle)
//              Menu incrémente le chiffre sélectionné (ou active retour/valider)

// ── Limites maximales par chiffre ─────────────────────────────────────────────
// H1 : 0-2   H2 : 0-9 (mais si H1==2 alors H2 : 0-3)
// M1 : 0-5   M2 : 0-9

static void tsIncrDigit() {
  switch (tsDigit) {
    case 0: // H1 : 0-2
      tsH1 = (tsH1 + 1) % 3;
      break;
    case 1: // H2 : 0-9 (0-3 si H1==2)
      {
        int maxH2 = (tsH1 == 2) ? 4 : 10;
        tsH2 = (tsH2 + 1) % maxH2;
      }
      break;
    case 2: // M1 : 0-5
      tsM1 = (tsM1 + 1) % 6;
      break;
    case 3: // M2 : 0-9
      tsM2 = (tsM2 + 1) % 10;
      break;
  }
}

void drawTimeSetScreen() {
  u8g2.clearBuffer();

  // ── Bouton retour en haut à gauche ────────────────────────────────────────
  if (tsDigit == 4) {
    u8g2.setDrawColor(1);
    u8g2.drawBox(0, 0, 14, 10);
    u8g2.setDrawColor(0);
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(2, 8, "<");
    u8g2.setDrawColor(1);
  } else {
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(2, 8, "<");
  }

  // ── Bouton valider en bas à droite ────────────────────────────────────────
  if (tsDigit == 5) {
    u8g2.setDrawColor(1);
    u8g2.drawBox(114, 54, 14, 10);
    u8g2.setDrawColor(0);
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(116, 62, "OK");
    u8g2.setDrawColor(1);
  } else {
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(116, 62, "OK");
  }

  // ── Les 4 grands chiffres ─────────────────────────────────────────────────
  // Positions identiques à drawHour() : H1=0, H2=29, ':'=54, M1=74, M2=102
  // On souligne le chiffre sélectionné avec un cadre
  const uint8_t digitY = 11;
  const uint8_t digitW = 25;
  const uint8_t digitH = 42;

  // H1
  {
    const uint8_t* b = getDigitBitmap(tsH1);
    if (b) drawDigit(b, 0, digitY);
    if (tsDigit == 0) u8g2.drawFrame(0, digitY, digitW, digitH);
  }
  // H2
  {
    const uint8_t* b = getDigitBitmap(tsH2);
    if (b) drawDigit(b, 29, digitY);
    if (tsDigit == 1) u8g2.drawFrame(29, digitY, digitW, digitH);
  }
  // ':'
  u8g2.drawXBMP(54, digitY, 20, digitH, colon);
  // M1
  {
    const uint8_t* b = getDigitBitmap(tsM1);
    if (b) drawDigit(b, 74, digitY);
    if (tsDigit == 2) u8g2.drawFrame(74, digitY, digitW, digitH);
  }
  // M2
  {
    const uint8_t* b = getDigitBitmap(tsM2);
    if (b) drawDigit(b, 102, digitY);
    if (tsDigit == 3) u8g2.drawFrame(102, digitY, digitW, digitH);
  }

  u8g2.sendBuffer();
}

// Applique l'heure saisie à timeinfo et bascule sur l'horloge
void tsApply() {
  int h = tsH1 * 10 + tsH2;
  int m = tsM1 * 10 + tsM2;

  // On construit une struct tm à partir de l'heure actuelle pour garder la date
  getLocalTime(&timeinfo, 0);
  timeinfo.tm_hour = h;
  timeinfo.tm_min  = m;
  timeinfo.tm_sec  = 0;

  // Ajuste le temps système (mktime normalise la struct tm)
  time_t t = mktime(&timeinfo);
  struct timeval tv = { t, 0 };
  settimeofday(&tv, nullptr);

  currentScreen = SCREEN_CLOCK;
}
