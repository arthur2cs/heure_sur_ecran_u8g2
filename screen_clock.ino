// ── screen_clock.ino ──────────────────────────────────────────────────────────
// Affichage de l'heure et de l'animation arcade

void LocalTime() {
  if (!getLocalTime(&timeinfo)) {
    u8g2.clearBuffer();
    drawMultiLineText("Echec d'aquisition de l'heure", 0, 0);
    u8g2.sendBuffer();
  }
}

const uint8_t* getDigitBitmap(int digit) {
  if (digit >= 0 && digit <= 9) return digitBitmaps[digit];
  return NULL;
}

void drawDigit(const uint8_t* bitmap, uint8_t x, uint8_t y) {
  u8g2.drawXBMP(x, y, 25, 42, bitmap);
}

void drawHour(struct tm* timeinfo) {
  u8g2.clearBuffer();

  int hours   = timeinfo->tm_hour;
  int minutes = timeinfo->tm_min;

  const uint8_t* b;

  b = getDigitBitmap(hours / 10);   if (b) drawDigit(b,   0, 11);
  b = getDigitBitmap(hours % 10);   if (b) drawDigit(b,  29, 11);
  u8g2.drawXBMP(54, 11, 20, 42, colon);
  b = getDigitBitmap(minutes / 10); if (b) drawDigit(b,  74, 11);
  b = getDigitBitmap(minutes % 10); if (b) drawDigit(b, 102, 11);

  u8g2.sendBuffer();
}

int randomGaussianPositive(int mean, int stdDev) {
  int value;
  do {
    double u1 = random(32767) / 32768.0;
    double u2 = random(32767) / 32768.0;
    double z  = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
    value = static_cast<int>(mean + stdDev * z);
  } while (value < 0);
  return value;
}

void drawAnimation() {
  unsigned long currentMillis = millis();

  if (isInPause) {
    if (currentMillis - pauseStartTime < pauseDuration) return;
    isInPause = false;
    animationCounter++;
    lastAnimationUpdate = currentMillis;
  }

  if (currentMillis - lastAnimationUpdate >= ANIMATION_FRAME_DELAY) {
    lastAnimationUpdate = currentMillis;
    u8g2.clearBuffer();

    int idx = animationCounter % 11;
    switch (idx) {
      case 0:  u8g2.drawXBMP(0, 0, 128, 64, arcade1); break;
      case 1:  u8g2.drawXBMP(0, 0, 128, 64, arcade2); break;
      case 2:  u8g2.drawXBMP(0, 0, 128, 64, arcade3); break;
      case 3:  u8g2.drawXBMP(0, 0, 128, 64, arcade4); break;
      case 4:  u8g2.drawXBMP(0, 0, 128, 64, arcade5); break;
      case 5:  u8g2.drawXBMP(0, 0, 128, 64, arcade6); break;
      case 6:  u8g2.drawXBMP(0, 0, 128, 64, arcade5); break;
      case 7:  u8g2.drawXBMP(0, 0, 128, 64, arcade4); break;
      case 8:  u8g2.drawXBMP(0, 0, 128, 64, arcade3); break;
      case 9:  u8g2.drawXBMP(0, 0, 128, 64, arcade2); break;
      case 10:
        u8g2.drawXBMP(0, 0, 128, 64, arcade1);
        pauseDuration  = randomGaussianPositive(delayMean, delayStdDev);
        pauseStartTime = currentMillis;
        isInPause      = true;
        break;
    }

    u8g2.sendBuffer();
    if (idx != 10) animationCounter++;
  }
}
