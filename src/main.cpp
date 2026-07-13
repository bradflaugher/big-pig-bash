#include <Arduboy2.h>
#include <EEPROM.h>

Arduboy2 arduboy;
BeepPin1 sound;

constexpr uint8_t FPS = 30;
constexpr uint8_t MAX_ENEMIES = 6;
constexpr uint8_t MAX_SHOTS = 9;
constexpr uint8_t MAX_SPARKS = 12;
constexpr uint8_t MAX_HP = 6;
constexpr uint8_t SAVE_ADDR = EEPROM_STORAGE_SPACE_START;

enum Mode : uint8_t { TITLE, STORY, PLAY, REWARD, BOSS_CARD, HURT_CARD, VICTORY, PAUSED };
enum FoeType : uint8_t { CROW, MOLE, BEE, CROW_KING, MUD_MONSTER };
enum Gift : uint8_t { RAPID, TRIPLE, TOUGH, TURBO };

struct Foe {
  int8_t x, y;
  int8_t vx, vy;
  uint8_t type, hp, clock, flash;
  bool alive;
};

struct Shot {
  int8_t x, y, vx, vy;
  uint8_t life;
  bool alive;
};

struct Spark {
  int8_t x, y, vx, vy;
  uint8_t life;
};

struct SaveData {
  uint8_t mark;
  uint8_t bestWave;
  uint8_t crowns;
  uint8_t check;
};

Foe foes[MAX_ENEMIES];
Shot shots[MAX_SHOTS];
Spark sparks[MAX_SPARKS];

Mode mode = TITLE;
Mode modeBeforePause = PLAY;
int8_t pigX = 60;
int8_t pigY = 40;
int8_t aimX = 1;
int8_t aimY = 0;
uint8_t hp = MAX_HP;
uint8_t wave = 1;
uint8_t spawned = 0;
uint8_t beaten = 0;
uint8_t waveTarget = 0;
uint8_t spawnClock = 0;
uint8_t shotClock = 0;
uint8_t bashCharge = 90;
uint8_t invincible = 0;
uint8_t screenShake = 0;
uint8_t cardClock = 0;
uint8_t rewardPick = 0;
uint8_t giftA = 0;
uint8_t giftB = 1;
uint8_t rapidLevel = 0;
uint8_t tripleLevel = 0;
uint8_t toughLevel = 0;
uint8_t turboLevel = 0;
uint8_t musicStep = 0;
uint8_t musicClock = 0;
uint16_t score = 0;
SaveData saveData;

const uint16_t melody[] PROGMEM = {
  1910, 1516, 1275, 954, 1135, 1275, 1516, 1702,
  1910, 1516, 1275, 1135, 954, 1275, 1516, 0
};

const char gift0[] PROGMEM = "QUICK";
const char gift1[] PROGMEM = "TRIPLE";
const char gift2[] PROGMEM = "TOUGH";
const char gift3[] PROGMEM = "TURBO";
const char *const giftNames[] PROGMEM = {gift0, gift1, gift2, gift3};

bool overlap(int8_t ax, int8_t ay, uint8_t aw, uint8_t ah,
             int8_t bx, int8_t by, uint8_t bw, uint8_t bh) {
  return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

uint8_t foeWidth(uint8_t type) { return type >= CROW_KING ? 22 : 10; }
uint8_t foeHeight(uint8_t type) { return type >= CROW_KING ? 18 : 10; }
int8_t stepToward(int8_t delta) { return delta > 0 ? 1 : (delta < 0 ? -1 : 0); }

uint8_t smaller(uint8_t a, uint8_t b) { return a < b ? a : b; }
uint8_t larger(uint8_t a, uint8_t b) { return a > b ? a : b; }

void playTone(uint16_t count, uint8_t duration) {
  sound.tone(count, duration);
  musicClock = 5;
}

void addSpark(int8_t x, int8_t y, int8_t vx, int8_t vy, uint8_t life = 10) {
  for (uint8_t i = 0; i < MAX_SPARKS; ++i) {
    if (!sparks[i].life) {
      sparks[i] = {x, y, vx, vy, life};
      return;
    }
  }
}

void burstSparks(int8_t x, int8_t y) {
  addSpark(x, y, 2, 0); addSpark(x, y, -2, 0);
  addSpark(x, y, 0, 2); addSpark(x, y, 0, -2);
  addSpark(x, y, 1, 1); addSpark(x, y, -1, 1);
}

void clearActors() {
  for (uint8_t i = 0; i < MAX_ENEMIES; ++i) foes[i].alive = false;
  for (uint8_t i = 0; i < MAX_SHOTS; ++i) shots[i].alive = false;
  for (uint8_t i = 0; i < MAX_SPARKS; ++i) sparks[i].life = 0;
}

void saveProgress() {
  if (wave > saveData.bestWave) saveData.bestWave = wave;
  saveData.mark = 0xB7;
  saveData.check = saveData.mark ^ saveData.bestWave ^ saveData.crowns ^ 0x5A;
  EEPROM.put(SAVE_ADDR, saveData);
}

void loadProgress() {
  EEPROM.get(SAVE_ADDR, saveData);
  if (saveData.mark != 0xB7 ||
      saveData.check != (uint8_t)(saveData.mark ^ saveData.bestWave ^ saveData.crowns ^ 0x5A)) {
    saveData = {0xB7, 1, 0, 0};
    saveProgress();
  }
}

void beginWave(bool refill) {
  clearActors();
  pigX = 60;
  pigY = 39;
  spawned = beaten = spawnClock = shotClock = 0;
  bashCharge = 90;
  invincible = 30;
  if (refill) hp = smaller(MAX_HP, hp + 2);
  waveTarget = (wave == 4 || wave == 8) ? 1 : 4 + wave;
  mode = (wave == 4 || wave == 8) ? BOSS_CARD : PLAY;
  cardClock = 0;
  saveProgress();
}

void newAdventure() {
  wave = 1;
  hp = MAX_HP;
  score = 0;
  rapidLevel = tripleLevel = toughLevel = turboLevel = 0;
  beginWave(false);
}

void spawnFoe() {
  for (uint8_t i = 0; i < MAX_ENEMIES; ++i) {
    if (foes[i].alive) continue;
    uint8_t type;
    if (wave == 4) type = CROW_KING;
    else if (wave == 8) type = MUD_MONSTER;
    else type = (spawned + wave) % smaller(3, 1 + wave / 2);

    uint8_t edge = random(4);
    int8_t x = (edge == 0) ? 3 : (edge == 1 ? 115 : random(8, 116));
    int8_t y = (edge == 2) ? 14 : (edge == 3 ? 52 : random(16, 52));
    if (type >= CROW_KING) { x = 101; y = 27; }
    uint8_t health = type == CROW_KING ? 18 : (type == MUD_MONSTER ? 24 : 1 + wave / 3);
    foes[i] = {x, y, 0, 0, type, health, (uint8_t)random(30), 0, true};
    ++spawned;
    playTone(type >= CROW_KING ? 2550 : 3032, 2);
    return;
  }
}

void shootOne(int8_t vx, int8_t vy) {
  for (uint8_t i = 0; i < MAX_SHOTS; ++i) {
    if (!shots[i].alive) {
      shots[i] = {(int8_t)(pigX + 7), (int8_t)(pigY + 6), vx, vy, 42, true};
      return;
    }
  }
}

void shoot() {
  int8_t sx = aimX * 3;
  int8_t sy = aimY * 3;
  shootOne(sx, sy);
  if (tripleLevel) {
    if (aimX) { shootOne(sx, 1); shootOne(sx, -1); }
    else { shootOne(1, sy); shootOne(-1, sy); }
  }
  shotClock = larger(4, 10 - rapidLevel * 2);
  playTone(1516, 2);
}

void damageFoe(Foe &foe, uint8_t amount) {
  if (!foe.alive) return;
  foe.flash = 4;
  if (foe.hp > amount) {
    foe.hp -= amount;
    playTone(2271, 2);
    return;
  }
  foe.alive = false;
  ++beaten;
  score += foe.type >= CROW_KING ? 500 : 50;
  screenShake = foe.type >= CROW_KING ? 10 : 3;
  burstSparks(foe.x + foeWidth(foe.type) / 2, foe.y + foeHeight(foe.type) / 2);
  playTone(foe.type >= CROW_KING ? 758 : 1135, 6);
}

void bigBash() {
  if (bashCharge < 90) return;
  bashCharge = 0;
  invincible = 18;
  screenShake = 8;
  playTone(638, 8);
  for (uint8_t i = 0; i < MAX_ENEMIES; ++i) {
    if (!foes[i].alive) continue;
    int16_t dx = foes[i].x + foeWidth(foes[i].type) / 2 - (pigX + 7);
    int16_t dy = foes[i].y + foeHeight(foes[i].type) / 2 - (pigY + 7);
    if (dx * dx + dy * dy < 625) damageFoe(foes[i], 2 + turboLevel);
  }
  for (uint8_t i = 0; i < 8; ++i)
    addSpark(pigX + 7, pigY + 7, (i % 3) - 1, ((i + 1) % 3) - 1, 14);
}

void hurtPig() {
  if (invincible) return;
  invincible = 40 + toughLevel * 8;
  screenShake = 8;
  playTone(3821, 8);
  if (--hp == 0) {
    mode = HURT_CARD;
    cardClock = 0;
    clearActors();
  }
}

void updateFoes() {
  if (spawned < waveTarget && !spawnClock) {
    spawnFoe();
    spawnClock = (wave == 4 || wave == 8) ? 22 : 36;
  }
  if (spawnClock) --spawnClock;

  for (uint8_t i = 0; i < MAX_ENEMIES; ++i) {
    Foe &f = foes[i];
    if (!f.alive) continue;
    ++f.clock;
    if (f.flash) --f.flash;
    int8_t dx = pigX - f.x;
    int8_t dy = pigY - f.y;
    uint8_t pace = f.type == MOLE ? 3 : (f.type >= CROW_KING ? 2 : 4);

    bool teleported = false;
    if (f.type == BEE) {
      if (!(f.clock % 3)) f.x += stepToward(dx);
      if (!(f.clock % 2)) f.y += stepToward(dy);
    } else if (f.type == CROW_KING) {
      if (!(f.clock % pace)) { f.x += stepToward(dx); f.y += stepToward(dy); }
      if (!(f.clock % 48)) {
        f.x = pigX < 58 ? 99 : 7;
        f.y = random(17, 44);
        teleported = true;
        burstSparks(f.x + 10, f.y + 8);
      }
    } else if (f.type == MUD_MONSTER) {
      if (f.clock % 36 < 12) {
        f.x += stepToward(dx);
        if (!(f.clock & 1)) f.y += stepToward(dy);
      }
    } else {
      if (!(f.clock % pace)) f.x += stepToward(dx);
      if (!((f.clock + i) % pace)) f.y += stepToward(dy);
    }

    uint8_t inset = f.type >= CROW_KING ? 3 : 1;
    if (!teleported && overlap(pigX + 2, pigY + 3, 11, 10, f.x + inset, f.y + inset,
                foeWidth(f.type) - inset * 2, foeHeight(f.type) - inset * 2)) hurtPig();
  }
}

void updateShots() {
  for (uint8_t i = 0; i < MAX_SHOTS; ++i) {
    Shot &s = shots[i];
    if (!s.alive) continue;
    s.x += s.vx;
    s.y += s.vy;
    if (!--s.life || s.x < 1 || s.x > 125 || s.y < 11 || s.y > 62) {
      s.alive = false;
      continue;
    }
    for (uint8_t j = 0; j < MAX_ENEMIES; ++j) {
      Foe &f = foes[j];
      if (f.alive && overlap(s.x, s.y, 3, 3, f.x, f.y, foeWidth(f.type), foeHeight(f.type))) {
        s.alive = false;
        damageFoe(f, 1);
        addSpark(s.x, s.y, -s.vx / 2, -s.vy / 2, 7);
        break;
      }
    }
  }
}

void updateSparks() {
  for (uint8_t i = 0; i < MAX_SPARKS; ++i) {
    if (!sparks[i].life) continue;
    sparks[i].x += sparks[i].vx;
    sparks[i].y += sparks[i].vy;
    --sparks[i].life;
  }
}

void finishWave() {
  if (wave == 8) {
    mode = VICTORY;
    ++saveData.crowns;
    saveProgress();
    cardClock = 0;
    playTone(954, 14);
    return;
  }
  giftA = wave % 4;
  giftB = (wave + 2 + (wave & 1)) % 4;
  for (uint8_t tries = 0; tries < 4; ++tries) {
    bool maxed = (giftA == TRIPLE && tripleLevel) || (giftA == RAPID && rapidLevel == 3) ||
                  (giftA == TOUGH && toughLevel == 2) || (giftA == TURBO && turboLevel == 2);
    if (!maxed) break;
    giftA = (giftA + 1) & 3;
  }
  for (uint8_t tries = 0; tries < 4; ++tries) {
    bool maxed = (giftB == TRIPLE && tripleLevel) || (giftB == RAPID && rapidLevel == 3) ||
                  (giftB == TOUGH && toughLevel == 2) || (giftB == TURBO && turboLevel == 2);
    if (!maxed && giftB != giftA) break;
    giftB = (giftB + 1) & 3;
  }
  rewardPick = 0;
  mode = REWARD;
  cardClock = 0;
}

void updatePlay() {
  if (arduboy.pressed(A_BUTTON | B_BUTTON) &&
      (arduboy.justPressed(A_BUTTON) || arduboy.justPressed(B_BUTTON))) {
    modeBeforePause = mode;
    mode = PAUSED;
    return;
  }
  if (arduboy.justPressed(B_BUTTON)) bigBash();

  int8_t speed = 1;
  if (arduboy.pressed(LEFT_BUTTON)) { pigX -= speed; aimX = -1; aimY = 0; }
  if (arduboy.pressed(RIGHT_BUTTON)) { pigX += speed; aimX = 1; aimY = 0; }
  if (arduboy.pressed(UP_BUTTON)) { pigY -= speed; aimX = 0; aimY = -1; }
  if (arduboy.pressed(DOWN_BUTTON)) { pigY += speed; aimX = 0; aimY = 1; }
  pigX = constrain(pigX, 3, 111);
  pigY = constrain(pigY, 13, 49);

  if (shotClock) --shotClock;
  if (arduboy.pressed(A_BUTTON) && !shotClock) shoot();
  if (bashCharge < 90) bashCharge = smaller(90, bashCharge + 1 + turboLevel);
  if (invincible) --invincible;
  if (screenShake) --screenShake;
  updateFoes();
  updateShots();
  updateSparks();
  if (beaten == waveTarget) finishWave();
}

void chooseGift() {
  uint8_t gift = rewardPick ? giftB : giftA;
  if (gift == RAPID) rapidLevel = smaller(3, rapidLevel + 1);
  else if (gift == TRIPLE) tripleLevel = 1;
  else if (gift == TOUGH) { toughLevel = smaller(2, toughLevel + 1); hp = MAX_HP; }
  else turboLevel = smaller(2, turboLevel + 1);
  ++wave;
  playTone(954, 9);
  beginWave(true);
}

void updateMusic() {
  if (musicClock) { --musicClock; return; }
  if (sound.duration || mode == HURT_CARD || mode == PAUSED) return;
  uint16_t note = pgm_read_word(&melody[musicStep]);
  musicStep = (musicStep + 1) & 15;
  musicClock = mode == PLAY ? 7 : 5;
  if (note) sound.tone(note, mode == PLAY ? 2 : 3);
}

void printCentered(const __FlashStringHelper *text, int8_t y) {
  arduboy.setCursor(64 - strlen_P((PGM_P)text) * 3, y);
  arduboy.print(text);
}

void drawPig(int8_t x, int8_t y, bool big = false) {
  uint8_t w = big ? 20 : 14;
  uint8_t h = big ? 14 : 11;
  int8_t bob = (arduboy.frameCount >> 2) & 1;
  y += bob;
  arduboy.fillRoundRect(x + 1, y + 4, w - 2, h - 4, 3, WHITE);
  arduboy.fillCircle(x + w - 5, y + 5, big ? 5 : 4, WHITE);
  arduboy.fillTriangle(x + w - 8, y + 2, x + w - 6, y - 2, x + w - 3, y + 2, WHITE);
  arduboy.drawPixel(x + w - 3, y + 4, BLACK);
  arduboy.fillRect(x + w - 2, y + 7, 3, 2, BLACK);
  arduboy.drawPixel(x + w - 1, y + 7, WHITE);
  arduboy.drawCircle(x + 1, y + 6, 2, BLACK);
  arduboy.fillRect(x + 3, y + h - 1, 2, 3, WHITE);
  arduboy.fillRect(x + w - 6, y + h - 1, 2, 3, WHITE);
}

void drawCrow(int8_t x, int8_t y, bool king) {
  uint8_t flap = (arduboy.frameCount >> 2) & 1;
  uint8_t size = king ? 8 : 4;
  arduboy.fillCircle(x + size, y + size, size, WHITE);
  arduboy.fillTriangle(x + size * 2 - 1, y + size - 2, x + size * 2 + 5, y + size,
                       x + size * 2 - 1, y + size + 2, WHITE);
  arduboy.drawPixel(x + size + 2, y + size - 2, BLACK);
  int8_t wingY = flap ? y + size : y + 1;
  arduboy.fillTriangle(x + size, y + size, x - size / 2, wingY,
                       x + 3, y + size + 3, WHITE);
  if (king) {
    arduboy.drawLine(x + 3, y + 1, x + 4, y - 4);
    arduboy.drawLine(x + 4, y - 4, x + 7, y);
    arduboy.drawLine(x + 7, y, x + 10, y - 4);
    arduboy.drawLine(x + 10, y - 4, x + 13, y + 1);
  }
}

void drawMole(int8_t x, int8_t y) {
  arduboy.fillRoundRect(x + 1, y + 3, 9, 7, 3, WHITE);
  arduboy.fillTriangle(x + 8, y + 5, x + 13, y + 7, x + 8, y + 9, WHITE);
  arduboy.drawPixel(x + 7, y + 5, BLACK);
  arduboy.drawFastHLine(x, y + 10, 12, WHITE);
}

void drawBee(int8_t x, int8_t y) {
  arduboy.drawCircle(x + 3, y + 2, 3, WHITE);
  arduboy.drawCircle(x + 8, y + 2, 3, WHITE);
  arduboy.fillRoundRect(x + 2, y + 4, 8, 6, 3, WHITE);
  arduboy.drawFastVLine(x + 5, y + 5, 5, BLACK);
  arduboy.drawFastVLine(x + 8, y + 5, 4, BLACK);
  arduboy.drawPixel(x + 10, y + 6, WHITE);
}

void drawMudMonster(int8_t x, int8_t y) {
  uint8_t wobble = (arduboy.frameCount >> 3) & 1;
  arduboy.fillRoundRect(x, y + 5 - wobble, 22, 13 + wobble, 5, WHITE);
  arduboy.fillCircle(x + 5, y + 5, 5, WHITE);
  arduboy.fillCircle(x + 16, y + 6, 6, WHITE);
  arduboy.fillCircle(x + 7, y + 7, 2, BLACK);
  arduboy.fillCircle(x + 16, y + 7, 2, BLACK);
  arduboy.drawFastHLine(x + 7, y + 14, 9, BLACK);
  arduboy.drawPixel(x + 3, y + 17, BLACK);
  arduboy.drawPixel(x + 18, y + 16, BLACK);
}

void drawPen() {
  arduboy.drawFastHLine(0, 10, 128, WHITE);
  arduboy.drawFastHLine(0, 62, 128, WHITE);
  for (uint8_t x = 2; x < 128; x += 16) {
    arduboy.drawFastVLine(x, 10, 5, WHITE);
    arduboy.drawFastVLine(x + 8, 58, 5, WHITE);
  }
  arduboy.drawCircle(24, 30, 7, WHITE);
  arduboy.drawCircle(24, 30, 4, WHITE);
  arduboy.drawPixel(25, 28, WHITE);
  arduboy.drawCircle(91, 45, 10, WHITE);
  arduboy.drawCircle(91, 45, 6, WHITE);
  for (uint8_t i = 0; i < 7; ++i)
    arduboy.drawPixel((i * 19 + wave * 7) % 126, 14 + (i * 11) % 45, WHITE);
}

void drawHud() {
  for (uint8_t i = 0; i < MAX_HP; ++i) {
    int8_t x = 2 + i * 6;
    if (i < hp) {
      arduboy.fillRect(x, 2, 4, 4, WHITE);
      arduboy.drawPixel(x, 2, BLACK);
      arduboy.drawPixel(x + 3, 2, BLACK);
    } else arduboy.drawRect(x, 2, 4, 4, WHITE);
  }
  arduboy.setCursor(44, 1);
  arduboy.print(F("PEN "));
  arduboy.print(wave);
  arduboy.drawRect(94, 2, 31, 5, WHITE);
  uint8_t fill = bashCharge / 3;
  if (fill) arduboy.fillRect(95, 3, smaller(29, fill), 3, WHITE);
  if (bashCharge == 90 && (arduboy.frameCount & 8)) arduboy.drawRect(92, 0, 35, 9, WHITE);
}

void drawPlay() {
  int8_t ox = screenShake ? ((arduboy.frameCount & 1) ? 1 : -1) : 0;
  int8_t oy = screenShake && (arduboy.frameCount & 2) ? 1 : 0;
  drawPen();
  for (uint8_t i = 0; i < MAX_SHOTS; ++i) {
    if (!shots[i].alive) continue;
    arduboy.fillCircle(shots[i].x + ox, shots[i].y + oy, 2, WHITE);
    arduboy.drawPixel(shots[i].x + ox, shots[i].y + oy, BLACK);
  }
  for (uint8_t i = 0; i < MAX_ENEMIES; ++i) {
    Foe &f = foes[i];
    if (!f.alive || (f.flash && (f.flash & 1))) continue;
    if (f.type == CROW) drawCrow(f.x + ox, f.y + oy, false);
    else if (f.type == MOLE) drawMole(f.x + ox, f.y + oy);
    else if (f.type == BEE) drawBee(f.x + ox, f.y + oy);
    else if (f.type == CROW_KING) drawCrow(f.x + ox, f.y + oy, true);
    else drawMudMonster(f.x + ox, f.y + oy);
    if (f.type >= CROW_KING) {
      uint8_t maxBoss = f.type == CROW_KING ? 18 : 24;
      arduboy.drawRect(36, 12, 56, 4, WHITE);
      arduboy.fillRect(37, 13, (uint16_t)54 * f.hp / maxBoss, 2, WHITE);
    }
  }
  for (uint8_t i = 0; i < MAX_SPARKS; ++i) {
    if (!sparks[i].life) continue;
    arduboy.drawLine(sparks[i].x, sparks[i].y, sparks[i].x - sparks[i].vx,
                     sparks[i].y - sparks[i].vy, WHITE);
  }
  if (!invincible || !(arduboy.frameCount & 2)) drawPig(pigX + ox, pigY + oy);
  drawHud();
}

void drawTitle() {
  for (uint8_t i = 0; i < 18; ++i) {
    uint8_t x = (i * 29 + arduboy.frameCount / 3) & 127;
    uint8_t y = 2 + (i * 17) % 59;
    arduboy.drawPixel(x, y, WHITE);
  }
  arduboy.setTextSize(2);
  arduboy.setCursor(17, 5);
  arduboy.print(F("BIG PIG"));
  arduboy.setCursor(28, 22);
  arduboy.print(F("BASH!"));
  arduboy.setTextSize(1);
  drawPig(50, 39, true);
  drawCrow(88, 41, false);
  arduboy.setCursor(2, 56); arduboy.print(F("BEST:")); arduboy.print(saveData.bestWave);
  arduboy.setCursor(92, 56); arduboy.print(F("WINS:")); arduboy.print(saveData.crowns);
  if (arduboy.frameCount & 16) printCentered(F("A:GO"), 56);
}

void drawStory() {
  drawPig(4, 7, true);
  arduboy.setCursor(32, 4);
  arduboy.print(F("HUNTER'S PIGPEN"));
  arduboy.setCursor(34, 14);
  arduboy.print(F("NEEDS A HERO!"));
  arduboy.drawFastHLine(5, 27, 118, WHITE);
  arduboy.setCursor(7, 33); arduboy.print(F("D-PAD"));
  arduboy.setCursor(50, 33); arduboy.print(F("A"));
  arduboy.setCursor(88, 33); arduboy.print(F("B"));
  arduboy.setCursor(4, 43); arduboy.print(F("MOVE"));
  arduboy.setCursor(39, 43); arduboy.print(F("ACORNS"));
  arduboy.setCursor(82, 43); arduboy.print(F("BIG BASH"));
  if (arduboy.frameCount & 16) printCentered(F("A: LET'S GO!"), 56);
}

void printGift(uint8_t gift, int8_t x, bool picked) {
  arduboy.drawRoundRect(x, 24, 60, 30, 4, WHITE);
  if (picked) arduboy.drawRoundRect(x + 2, 26, 56, 26, 3, WHITE);
  arduboy.setCursor(x + 3, 30);
  char buffer[13];
  strcpy_P(buffer, (PGM_P)pgm_read_ptr(&giftNames[gift]));
  arduboy.print(buffer);
  arduboy.setCursor(x + 12, 42);
  if (gift == RAPID) arduboy.print(F("FASTER!"));
  else if (gift == TRIPLE) arduboy.print(F("3 SHOTS"));
  else if (gift == TOUGH) arduboy.print(F("HEAL UP"));
  else arduboy.print(F("BASH++"));
}

void drawReward() {
  printCentered(F("PICK A PIG POWER!"), 5);
  arduboy.drawLine(64, 18, 64, 58, WHITE);
  printGift(giftA, 1, rewardPick == 0);
  printGift(giftB, 67, rewardPick == 1);
  arduboy.fillTriangle(rewardPick ? 92 : 26, 58, rewardPick ? 98 : 32, 58,
                       rewardPick ? 95 : 29, 62, WHITE);
}

void drawBossCard() {
  arduboy.drawRect(3, 3, 122, 58, WHITE);
  printCentered(F("WARNING!"), 8);
  arduboy.setTextSize(2);
  if (wave == 4) { arduboy.setCursor(11, 23); arduboy.print(F("CROW KING")); drawCrow(53, 45, true); }
  else { arduboy.setCursor(4, 23); arduboy.print(F("MUDZILLA!")); drawMudMonster(52, 44); }
  arduboy.setTextSize(1);
  if (arduboy.frameCount & 8) printCentered(F("A: FIGHT!"), 52);
}

void drawHurtCard() {
  printCentered(F("OINK! BONKED!"), 10);
  drawPig(53, 23, true);
  arduboy.drawCircle(82, 25, 5, WHITE);
  arduboy.drawLine(79, 22, 85, 28, WHITE);
  arduboy.drawLine(85, 22, 79, 28, WHITE);
  printCentered(F("TRY THAT PEN AGAIN!"), 47);
  if (arduboy.frameCount & 16) printCentered(F("PRESS A"), 56);
}

void drawVictory() {
  for (uint8_t i = 0; i < 24; ++i) {
    uint8_t x = (i * 23 + arduboy.frameCount) & 127;
    uint8_t y = (i * 13 + arduboy.frameCount / 2) % 64;
    arduboy.drawPixel(x, y, WHITE);
    if (!(i % 4)) arduboy.drawPixel(x + 1, y, WHITE);
  }
  arduboy.setTextSize(2);
  arduboy.setCursor(5, 4); arduboy.print(F("PIGPEN"));
  arduboy.setCursor(17, 20); arduboy.print(F("LEGEND!"));
  arduboy.setTextSize(1);
  drawPig(51, 38, true);
  arduboy.setCursor(4, 55); arduboy.print(F("HUNTER"));
  arduboy.setCursor(86, 55); arduboy.print(F("A:AGAIN"));
}

void drawPause() {
  drawPlay();
  arduboy.fillRect(27, 19, 74, 28, BLACK);
  arduboy.drawRoundRect(27, 19, 74, 28, 4, WHITE);
  printCentered(F("PIGGY PAUSE"), 25);
  printCentered(F("A:PLAY  B:QUIT"), 37);
}

void setup() {
  arduboy.begin();
  arduboy.setFrameRate(FPS);
  arduboy.initRandomSeed();
  sound.begin();
  loadProgress();
}

void loop() {
  if (!arduboy.nextFrame()) return;
  sound.timer();
  arduboy.pollButtons();

  if (mode == TITLE && arduboy.justPressed(A_BUTTON)) { mode = STORY; playTone(1275, 5); }
  else if (mode == STORY && arduboy.justPressed(A_BUTTON)) newAdventure();
  else if (mode == PLAY) updatePlay();
  else if (mode == BOSS_CARD) {
    if (cardClock < 21) ++cardClock;
    if (cardClock > 20 && arduboy.justPressed(A_BUTTON)) { mode = PLAY; playTone(638, 8); }
  } else if (mode == REWARD) {
    if (arduboy.justPressed(LEFT_BUTTON)) rewardPick = 0;
    if (arduboy.justPressed(RIGHT_BUTTON)) rewardPick = 1;
    if (arduboy.justPressed(A_BUTTON)) chooseGift();
  } else if (mode == HURT_CARD) {
    if (cardClock < 21) ++cardClock;
    if (cardClock > 20 && arduboy.justPressed(A_BUTTON)) { hp = MAX_HP; beginWave(false); }
  } else if (mode == VICTORY && arduboy.justPressed(A_BUTTON)) { mode = TITLE; playTone(1275, 6); }
  else if (mode == PAUSED) {
    if (arduboy.justPressed(A_BUTTON)) mode = modeBeforePause;
    if (arduboy.justPressed(B_BUTTON)) mode = TITLE;
  }

  updateMusic();
  arduboy.clear();
  if (mode == TITLE) drawTitle();
  else if (mode == STORY) drawStory();
  else if (mode == PLAY) drawPlay();
  else if (mode == REWARD) drawReward();
  else if (mode == BOSS_CARD) drawBossCard();
  else if (mode == HURT_CARD) drawHurtCard();
  else if (mode == VICTORY) drawVictory();
  else drawPause();
  arduboy.display();
}
