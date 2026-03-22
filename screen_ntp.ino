// ── screen_ntp.ino ────────────────────────────────────────────────────────────
// Connexion WiFi + synchronisation NTP → bascule sur l'horloge

void syncTime() {
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
      // configTzTime même sans WiFi : évite que getLocalTime() bloque dans loop()
      configTzTime("UTC0", ntpServer);
      currentScreen = SCREEN_CLOCK;
      return;
    }
    u8g2.clearBuffer();
    drawMultiLineText(msg, 0, 0);
    u8g2.sendBuffer();
    delay(500);
  }

  configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", ntpServer);
  u8g2.clearBuffer();
  drawMultiLineText("WiFi connecte !", 0, 0);
  u8g2.sendBuffer();
  delay(1500);
  currentScreen = SCREEN_CLOCK;
}
