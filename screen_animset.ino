// ── screen_animset.ino ────────────────────────────────────────────────────────
// Réglage de la moyenne et de l'écart-type de l'animation (pause inter-frames)
// asRow : 0=retour  1=moyenne  2=écart-type  3=OK
//
// Next  : avance la sélection : retour → moy< → moy> → écart< → écart> → OK → retour
// Menu  : sur retour → menu sans sauver
//         sur </>    → décrémente/incrémente (pas de sauvegarde immédiate)
//         sur OK     → sauvegarde en flash + bascule horloge
//
// Valeurs en millisecondes en interne, affichées en secondes (1 décimale)
// Bornes : 1 s (1000 ms) … 1000 s (1 000 000 ms), pas de 1 s

// ── Helpers ───────────────────────────────────────────────────────────────────
static void asDrawRow(uint8_t y, const char* label, unsigned long valueMs, bool selected, int col) {
  u8g2.setFont(u8g2_font_5x7_tr);

  // Label (titre)
  u8g2.setDrawColor(1);
  u8g2.drawStr(0, y - 1, label);

  // Ligne de contrôle : [<]  XX.X s  [>]
  uint8_t rowY = y + 8;

  // "<" : surligné si selected && col==0
  if (selected && col == 0) {
    u8g2.setDrawColor(1);
    u8g2.drawBox(0, rowY - 7, 10, 9);
    u8g2.setDrawColor(0);
    u8g2.drawStr(2, rowY, "<");
    u8g2.setDrawColor(1);
  } else {
    u8g2.drawStr(2, rowY, "<");
  }

  // Valeur en secondes avec 1 décimale
  char buf[16];
  unsigned long s   = valueMs / 1000;
  unsigned long dec = (valueMs % 1000) / 100;
  snprintf(buf, sizeof(buf), "%lu.%lu s", s, dec);
  uint8_t valW = u8g2.getStrWidth(buf);
  u8g2.setDrawColor(1);
  u8g2.drawStr(14, rowY, buf);

  // ">" : surligné si selected && col==1
  uint8_t arrowX = 14 + valW + 2;
  if (selected && col == 1) {
    u8g2.setDrawColor(1);
    u8g2.drawBox(arrowX - 1, rowY - 7, 10, 9);
    u8g2.setDrawColor(0);
    u8g2.drawStr(arrowX + 1, rowY, ">");
    u8g2.setDrawColor(1);
  } else {
    u8g2.setDrawColor(1);
    u8g2.drawStr(arrowX + 1, rowY, ">");
  }
}

void drawAnimSetScreen() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x7_tr);

  // ── Bouton retour en haut à gauche ────────────────────────────────────────
  if (asRow == 0) {
    u8g2.setDrawColor(1);
    u8g2.drawBox(0, 0, 14, 10);
    u8g2.setDrawColor(0);
    u8g2.drawStr(2, 8, "<");
    u8g2.setDrawColor(1);
  } else {
    u8g2.drawStr(2, 8, "<");
  }

  // ── Ligne Moyenne (y=14 pour le label, y+8=22 pour les flèches) ──────────
  asDrawRow(22, "Moyenne", delayMean,   asRow == 1, asCol);

  // ── Ligne Ecart-type (y=40 pour le label, y+8=48 pour les flèches) ───────────
  asDrawRow(42, "Ecart-type", delayStdDev, asRow == 2, asCol);

  // ── Bouton OK en bas à droite ─────────────────────────────────────────────
  if (asRow == 3) {
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

  u8g2.sendBuffer();
}

// Sauvegarde les valeurs en flash
void asSave() {
  prefs.begin("anim", false);
  prefs.putULong("mean",   delayMean);
  prefs.putULong("stddev", delayStdDev);
  prefs.end();
}
