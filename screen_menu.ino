// ── screen_menu.ino ───────────────────────────────────────────────────────────
// Menu principal + utilitaire drawMultiLineText

void drawMultiLineText(const char* text, uint8_t startX, uint8_t startY) {
  u8g2.setFont(u8g2_font_ncenB08_tr);

  uint8_t lineHeight = u8g2.getFontAscent() - u8g2.getFontDescent();
  uint8_t currentLine = 1;
  const char* wordDelimiters = " \t\n\r";

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
    startX += textWidth + u8g2.getMaxCharWidth();
    if (startX >= u8g2.getDisplayWidth()) { startX = 0; currentLine++; }
    token = strtok(nullptr, wordDelimiters);
  }
}

void drawMainMenu() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  const uint8_t lineH   = 13;
  const uint8_t yStart  = 10;
  const int     visible = 4;  // nb de lignes visibles

  u8g2.drawStr(0, yStart, "-- MENU --");

  // Fenetre glissante : on garde menuIndex visible
  if (menuIndex < menuScrollTop)                   menuScrollTop = menuIndex;
  if (menuIndex >= menuScrollTop + visible)        menuScrollTop = menuIndex - visible + 1;

  for (int i = 0; i < visible; i++) {
    int idx = menuScrollTop + i;
    if (idx >= MENU_ITEMS) break;
    uint8_t y = yStart + (i + 1) * lineH;
    if (idx == menuIndex) {
      u8g2.setDrawColor(1);
      u8g2.drawBox(0, y - 9, 128, lineH);
      u8g2.setDrawColor(0);
      u8g2.drawStr(2, y, menuLabels[idx]);
      u8g2.setDrawColor(1);
    } else {
      u8g2.drawStr(2, y, menuLabels[idx]);
    }
  }
  u8g2.sendBuffer();
}
