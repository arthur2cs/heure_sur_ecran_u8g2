// ── screen_password.ino ───────────────────────────────────────────────────────
// Clavier de saisie du mot de passe WiFi

// 4 lignes × 10 colonnes. Touches spéciales encodées :
//   \x01 = Shift/Caps    \x02 = Basculer spéciaux/chiffres
//   \x08 = Backspace     \x0D = Valider
//   \x20 = Espace

const char KB_LOWER[4][10] = {
  {'q','w','e','r','t','y','u','i','o','p'},
  {'a','s','d','f','g','h','j','k','l','-'},
  {'\x01','z','x','c','v','b','n','m','\x08','\x0D'},
  {'\x02','.',',','!','?','@','_','/','\'','\x0D'}
};
const char KB_UPPER[4][10] = {
  {'Q','W','E','R','T','Y','U','I','O','P'},
  {'A','S','D','F','G','H','J','K','L','-'},
  {'\x01','Z','X','C','V','B','N','M','\x08','\x0D'},
  {'\x02','.',',','!','?','@','_','/','\'','\x0D'}
};
const char KB_SPECIAL[4][10] = {
  {'1','2','3','4','5','6','7','8','9','0'},
  {'!','@','#','$','%','^','&','*','(',')'},
  {'-','_','=','+','[',']','{','}','\x08','\x0D'},
  {'\x02',';',':','"','<','>',',','.','\x20','\x0D'}
};

const uint8_t KB_COL_W[10] = {12,12,12,12,12,12,12,12,14,14};
const uint8_t KB_ROW_H     = 12;
const uint8_t KB_Y_START   = 16;

// ── Helpers internes ──────────────────────────────────────────────────────────

const char* kbGetRow(int row) {
  switch (kbLayout) {
    case KB_LAY_UPPER:
    case KB_LAY_CAPS:  return KB_UPPER[row];
    case KB_LAY_SPECIAL: return KB_SPECIAL[row];
    default:             return KB_LOWER[row];
  }
}

uint8_t kbColX(int c) {
  uint8_t x = 0;
  for (int i = 0; i < c; i++) x += KB_COL_W[i];
  return x;
}

const char* kbSpecialLabel(char ch) {
  switch ((uint8_t)ch) {
    case 0x01:
      if (kbLayout == KB_LAY_CAPS)    return "[^]";  // caps lock actif
      if (kbLayout == KB_LAY_UPPER)   return "v^";   // one-shot upper
      return "^^";                                    // lower
    case 0x02: return (kbLayout == KB_LAY_SPECIAL) ? "az" : "#1";
    case 0x08: return "<X";
    case 0x0D: return "OK";
    case 0x20: return "___";
    default:   return "?";
  }
}

// ── Dessin ────────────────────────────────────────────────────────────────────

void drawPasswordScreen() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x7_tr);

  // Bouton retour "<" (surligné si kbRow == -1)
  if (kbRow == -1) {
    u8g2.setDrawColor(1);
    u8g2.drawBox(0, 0, 14, 12);
    u8g2.setDrawColor(0);
    u8g2.drawStr(3, 9, "<");
    u8g2.setDrawColor(1);
  } else {
    u8g2.drawFrame(0, 0, 14, 12);
    u8g2.drawStr(3, 9, "<");
  }

  // Bandeau mot de passe avec scroll automatique
  u8g2.drawFrame(15, 0, 113, 12);
  u8g2.setClipWindow(16, 1, 127, 11);

  int pwdPixelW = pwdLen * 6;   // police 5x7 ≈ 6 px/char
  int textX     = (pwdPixelW < 111) ? 17 : (127 - pwdPixelW - 1);
  u8g2.drawStr(textX, 9, pwdBuffer);

  int curX = textX + pwdPixelW;
  if (curX < 126) u8g2.drawVLine(curX, 2, 8);
  u8g2.setMaxClipWindow();

  // Grille de touches
  for (int row = 0; row < 4; row++) {
    const char* keys = kbGetRow(row);
    for (int col = 0; col < 10; col++) {
      char    ch = keys[col];
      uint8_t x  = kbColX(col);
      uint8_t y  = KB_Y_START + row * KB_ROW_H;
      uint8_t w  = KB_COL_W[col];
      bool sel   = (row == kbRow && col == kbCol);

      if (sel) {
        u8g2.setDrawColor(1); u8g2.drawBox(x, y, w, KB_ROW_H - 1);
        u8g2.setDrawColor(0);
      } else {
        u8g2.setDrawColor(1); u8g2.drawFrame(x, y, w, KB_ROW_H - 1);
      }

      char singleChar[2] = {ch, 0};
      bool isSpecial = ((uint8_t)ch < 0x20 || (uint8_t)ch == 0x20);
      const char* label = isSpecial ? kbSpecialLabel(ch) : singleChar;

      uint8_t lw = u8g2.getStrWidth(label);
      u8g2.drawStr(x + (w - lw) / 2, y + KB_ROW_H - 3, label);
      u8g2.setDrawColor(1);
    }
  }

  u8g2.sendBuffer();
}

// ── Action sur la touche sélectionnée ────────────────────────────────────────

void kbPressKey() {
  const char* keys = kbGetRow(kbRow);
  char ch = keys[kbCol];

  switch ((uint8_t)ch) {

    case 0x01:  // Shift
      if (kbLayout == KB_LAY_LOWER) {
        // Lower → one-shot upper
        kbLayout = KB_LAY_UPPER;
      } else if (kbLayout == KB_LAY_UPPER) {
        // One-shot upper (pas encore tapé de lettre) → caps lock
        kbLayout = KB_LAY_CAPS;
      } else if (kbLayout == KB_LAY_CAPS) {
        // Caps lock → retour lower
        kbLayout = KB_LAY_LOWER;
      }
      break;

    case 0x02:  // Basculer spéciaux ↔ alpha
      kbLayout = (kbLayout == KB_LAY_SPECIAL) ? KB_LAY_LOWER : KB_LAY_SPECIAL;
      break;

    case 0x08:  // Backspace
      if (pwdLen > 0) { pwdLen--; pwdBuffer[pwdLen] = '\0'; }
      break;

    case 0x0D:  // Valider → connexion
      {
        u8g2.clearBuffer();
        char msg[64];
        snprintf(msg, sizeof(msg), "Connexion a \"%s\"...", WiFi.SSID(pwdSelectedNet).c_str());
        drawMultiLineText(msg, 0, 0);
        u8g2.sendBuffer();

        // Sauvegarde le ssid et le mot de passe en flash pour le prochain démarrage
        strncpy(ssid,     WiFi.SSID(pwdSelectedNet).c_str(), sizeof(ssid) - 1);
        ssid[sizeof(ssid) - 1] = '\0';
        strncpy(password, pwdBuffer, sizeof(password) - 1);
        password[sizeof(password) - 1] = '\0';
        prefs.begin("wifi", false);
        prefs.putString("ssid",     ssid);
        prefs.putString("password", password);
        prefs.end();

        syncTime();  // connexion + configTzTime + bascule sur SCREEN_CLOCK
        return;
      }

    case 0x20:  // Espace
      if (pwdLen < 63) { pwdBuffer[pwdLen++] = ' '; pwdBuffer[pwdLen] = '\0'; }
      break;

    default:    // Caractère normal
      if (pwdLen < 63) {
        pwdBuffer[pwdLen++] = ch;
        pwdBuffer[pwdLen]   = '\0';
        // One-shot : repasse en lower uniquement si on était en upper simple
        if (kbLayout == KB_LAY_UPPER) kbLayout = KB_LAY_LOWER;
        // En CAPS : on reste en majuscule
      }
      break;
  }
  drawPasswordScreen();
}
