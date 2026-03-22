// ── screen_wifi.ino ───────────────────────────────────────────────────────────
// Liste des réseaux WiFi + connexion au démarrage (NTPstart)

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
      return;
    }
    u8g2.clearBuffer();
    drawMultiLineText(msg, 0, 0);
    u8g2.sendBuffer();
    delay(500);
  }

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  u8g2.clearBuffer();
  drawMultiLineText("WiFi connecte !", 0, 0);
  u8g2.sendBuffer();
  delay(1500);
}

void drawWifiMenu() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  const uint8_t lineH = 11;

  // Flèche retour (surlignée si index 0)
  if (wifiMenuIndex == 0) {
    u8g2.setDrawColor(1);
    u8g2.drawBox(0, 0, 20, lineH);
    u8g2.setDrawColor(0);
    u8g2.drawStr(2, 9, "<");
    u8g2.setDrawColor(1);
  } else {
    u8g2.drawStr(2, 9, "<");
  }

  // Fenêtre glissante : on affiche 4 réseaux autour de la sélection
  int visibleStart = max(0, wifiMenuIndex - 4);
  for (int i = 0; i < min(wifiNetworkCount, 4); i++) {
    int netIdx    = visibleStart + i;
    if (netIdx >= wifiNetworkCount) break;

    uint8_t y     = lineH + (i + 1) * lineH;
    int listIndex = netIdx + 1;

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
