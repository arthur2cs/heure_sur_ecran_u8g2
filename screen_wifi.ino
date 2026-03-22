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
