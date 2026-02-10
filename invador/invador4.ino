#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const int LEFT_BUTTON_PIN = 5;
const int RIGHT_BUTTON_PIN = 6;
const int BUZZER_PIN = 9;


/* 効果音（共通） */
#define S_R 1
#define S_A0 2
#define S_B0 3
#define S_C1 4
#define S_D1 5

const char melody[] = { S_D1, S_C1, S_B0, S_A0};

// プレイヤー
int playerX = 54;
const int playerY = 56;
const int playerWidth = 8;
const int playerHeight = 5;

// 弾
struct Bullet {
  float x;
  float y;
  bool active;
};

Bullet playerBullet = {0, 0, false};
const int bulletSpeed = 3;
unsigned long lastPlayerShot = 0;
const int playerShootDelay = 50;  // 0.5秒ごとに自動発射

// 敵
const int enemyRows = 3;
const int enemyCols = 8;
const int enemyWidth = 6;
const int enemyHeight = 5;
const int enemySpacing = 4;
struct Enemy {
  float x;
  float y;
  bool alive;
};
Enemy enemies[enemyRows][enemyCols];
float enemyDirection = 1;
float enemySpeed = 0.5;
unsigned long lastEnemyMove = 0;
//const int enemyMoveDelay = 200;
int enemyMoveDelay = 220;

// 敵の弾
const int maxEnemyBullets = 3;
Bullet enemyBullets[maxEnemyBullets];
unsigned long lastEnemyShot = 0;
const int enemyShootDelay = 1500;

int score = 0;
bool gameOver = false;
bool gameWin = false;
unsigned long lastButtonPress = 0;
const int buttonDelay = 150;
int alo=80;
unsigned long lastmyMove = 0;
int dflag=0;
int msf=0;

/* =========================
 *  ブザー音（D9=OC1A / Timer1）
 * ========================= */
struct Beep { uint16_t freq; uint16_t ms; };
bool     BUZZER_ENABLED   = true;
int8_t   BUZZER_SEMITONES = 0;

static volatile uint32_t g_matches_left = 0;
static volatile bool     g_running      = false;

Beep inv_sound;

ISR(TIMER1_COMPA_vect){
  if (!g_running) return;
  if (g_matches_left > 0) {
    if (--g_matches_left == 0) {
      TCCR1A = 0; TCCR1B = 0; TIMSK1 = 0;
      digitalWrite(9, LOW);
      g_running = false;
    }
  }
}



void buzzerPlayRaw(uint32_t freq, uint32_t dur_ms){
  if (freq == 0 || dur_ms == 0) return;
  pinMode(9, OUTPUT);
  TCCR1A = 0; TCCR1B = 0; TIMSK1 = 0;
  uint32_t ocr; uint16_t prescBits; uint32_t presc;
  if ((ocr = (F_CPU/(2UL*1*freq))-1)      <= 65535) { prescBits=_BV(CS10);            presc=1; }
  else if ((ocr=(F_CPU/(2UL*8*freq))-1)   <= 65535) { prescBits=_BV(CS11);            presc=8; }
  else if ((ocr=(F_CPU/(2UL*64*freq))-1)  <= 65535) { prescBits=_BV(CS11)|_BV(CS10);  presc=64; }
  else if ((ocr=(F_CPU/(2UL*256*freq))-1) <= 65535) { prescBits=_BV(CS12);            presc=256; }
  else                                            { ocr=(F_CPU/(2UL*1024*freq))-1; prescBits=_BV(CS12)|_BV(CS10); presc=1024; }
  OCR1A = (uint16_t)ocr;
  TCCR1A = _BV(COM1A0);
  TCCR1B = _BV(WGM12)|prescBits;
  uint64_t match_rate = (uint64_t)F_CPU / ((uint64_t)presc * ((uint64_t)OCR1A + 1));
  uint64_t matches = (match_rate * dur_ms) / 1000ULL;
  if (matches == 0) matches = 1;
  noInterrupts();
  g_matches_left = (uint32_t)matches;
  g_running = true;
  TIMSK1 = _BV(OCIE1A);
  interrupts();
}
static inline uint16_t transpose(uint16_t baseHz, int8_t semis){
  if (semis == 0) return baseHz;
  float f = (float)baseHz * powf(2.0f, semis / 12.0f);
  if (f < 1.0f) f = 1.0f; if (f > 20000.0f) f = 20000.0f;
  return (uint16_t)(f + 0.5f);
}
void playBeep(const Beep& s){
  if (!BUZZER_ENABLED) return;
  buzzerPlayRaw(transpose(s.freq, BUZZER_SEMITONES), s.ms);
}


void updateSound(int f) {
  Beep tetris_sound;
  static int i;
  static unsigned long ti;
  static int rep;
  if (f==0){i=0; return;}
  int t=0;
  int l=0;
  if (millis()<ti){return;}
  t=melody[i];  
  //t=pgm_read_byte(melody+i);  
  if (t==1){l=1;}
  if (t==2){l=28;}
  if (t==3){l=30;}
  if (t==4){l=32;}
  if (t==5){l=37;}

  inv_sound.freq=l;
  //t=noteD[i];
  //t=pgm_read_byte(noteD+i);  
  //tetris_sound.ms=1200/t;
  inv_sound.ms=200;
  if (millis()>ti){ti=millis()+enemyMoveDelay*5;}else{return;} 
  if (l!=1){
    playBeep(inv_sound);
  }
  i++;
  //if (i==40 && rep==0){i=0; rep=1;}
  if (i>=sizeof(melody)){i=0; rep=0;}
}




void setup() {

  Wire.setClock(100000);	

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON_PIN, INPUT_PULLUP);
  initGame();
}

void initGame() {
  // 敵の初期化
  for (int i = 0; i < enemyRows; i++) {
    for (int j = 0; j < enemyCols; j++) {
      enemies[i][j].x = j * (enemyWidth + enemySpacing) + 8;
      enemies[i][j].y = i * (enemyHeight + enemySpacing) + 5;
      enemies[i][j].alive = true;
    }
  }

  // 敵の弾の初期化
  for (int i = 0; i < maxEnemyBullets; i++) {
    enemyBullets[i].active = false;
  }

  playerX = 54;
  playerBullet.active = false;
  //score = 0;
  gameOver = false;
  gameWin = false;
  enemyDirection = 1;
  enemySpeed = 0.5;
  enemyMoveDelay = 240;
  dflag=0;
}

void loop() {
  //if (gameOver || gameWin) {
  if (gameOver ) {
    displayEndScreen();
    // 左ボタンでリスタート
    if (digitalRead(LEFT_BUTTON_PIN) == LOW) {
      delay(50);
      while (digitalRead(LEFT_BUTTON_PIN) == LOW) {
        delay(10);
      }
      initGame();
      score = 0;
    }
    return;
  }
  if (gameWin) {
    displayEndScreen();
    initGame();
  }

  unsigned long currentTime = millis();


if (currentTime - lastmyMove > alo) {
  lastmyMove=currentTime;
  //dflag=1;
  // プレイヤー操作
  //if (digitalRead(LEFT_BUTTON_PIN) == LOW && playerX > 0) {
  if (digitalRead(LEFT_BUTTON_PIN) == LOW && digitalRead(RIGHT_BUTTON_PIN) == LOW) {
    if (!playerBullet.active) {msf=1;}   //playerBullet.active = true;
  }else{ 
    if (digitalRead(LEFT_BUTTON_PIN) == LOW ) {
      if (currentTime - lastButtonPress > 30) {
        if (playerX > 0){
          playerX -= 2;
        }
        lastButtonPress = currentTime;
      }
    }
    if (digitalRead(RIGHT_BUTTON_PIN) == LOW) {
      if (currentTime - lastButtonPress > 30) {
        if (playerX < SCREEN_WIDTH - playerWidth) {
          playerX += 2;
        }
        lastButtonPress = currentTime;
      }
    }
  }

  // 自動発射
  if (msf==1 &&  !playerBullet.active && (currentTime - lastPlayerShot > playerShootDelay) ) {
    playerBullet.x = playerX + playerWidth / 2;
    playerBullet.y = playerY - 2;
    playerBullet.active = true;
    lastPlayerShot = currentTime;
    inv_sound.freq=4000;
    inv_sound.ms=5;
    playBeep(inv_sound);
    msf=0;

  }

  // プレイヤーの弾の移動
  if (playerBullet.active) {
    playerBullet.y -= bulletSpeed;
    if (playerBullet.y < 0) {
      playerBullet.active = false;
    }
  }
}

  // 敵の移動
  if (currentTime - lastEnemyMove > enemyMoveDelay) {
    bool needMoveDown = false;
    if (dflag++>3){dflag=0;}
    // 端に到達したかチェック
    for (int i = 0; i < enemyRows; i++) {
      for (int j = 0; j < enemyCols; j++) {
        if (enemies[i][j].alive) {
          if ((enemies[i][j].x <= 0 && enemyDirection < 0) ||
              (enemies[i][j].x >= SCREEN_WIDTH - enemyWidth && enemyDirection > 0)) {
            needMoveDown = true;
            enemyDirection = -enemyDirection;
            break;
          }
        }
      }
      if (needMoveDown) break;
    }

    // 敵を移動
    for (int i = 0; i < enemyRows; i++) {
      for (int j = 0; j < enemyCols; j++) {
        if (enemies[i][j].alive) {
          enemies[i][j].x += enemyDirection * enemySpeed;
          if (needMoveDown) {
            enemies[i][j].y += 3;
            // 敵がプレイヤーまで到達したらゲームオーバー
            if (enemies[i][j].y >= playerY - enemyHeight) {
              gameOver = true;
            }
          }
        }
      }
    }

    lastEnemyMove = currentTime;
  }

  // 敵の発射
  if (currentTime - lastEnemyShot > enemyShootDelay) {
    // ランダムな生きている敵から発射
    int attempts = 0;
    while (attempts < 20) {
      int i = random(enemyRows);
      int j = random(enemyCols);
      if (enemies[i][j].alive) {
        // 空いている弾スロットを探す
        for (int b = 0; b < maxEnemyBullets; b++) {
          if (!enemyBullets[b].active) {
            enemyBullets[b].x = enemies[i][j].x + enemyWidth / 2;
            enemyBullets[b].y = enemies[i][j].y + enemyHeight;
            enemyBullets[b].active = true;
            break;
          }
        }
        break;
      }
      attempts++;
    }
    lastEnemyShot = currentTime;
  }

  // 敵の弾の移動
  for (int i = 0; i < maxEnemyBullets; i++) {
    if (enemyBullets[i].active) {
      enemyBullets[i].y += 2;
      if (enemyBullets[i].y > SCREEN_HEIGHT) {
        enemyBullets[i].active = false;
      }

      // プレイヤーとの衝突判定
      if (enemyBullets[i].y >= playerY && enemyBullets[i].y <= playerY + playerHeight &&
          enemyBullets[i].x >= playerX && enemyBullets[i].x <= playerX + playerWidth) {
        gameOver = true;
          tone(BUZZER_PIN, 200, 200);  // 低い音
          delay(250);
          tone(BUZZER_PIN, 150, 200);  // さらに低い音
          delay(250);
          tone(BUZZER_PIN, 100, 500);  // とても低い音
          delay(600);
          noTone(BUZZER_PIN);
      }
    }
  }


  // プレイヤーの弾と敵の衝突判定
  if (playerBullet.active) {
    for (int i = 0; i < enemyRows; i++) {
      for (int j = 0; j < enemyCols; j++) {
        if (enemies[i][j].alive) {
          if (playerBullet.x >= enemies[i][j].x &&
              playerBullet.x <= enemies[i][j].x + enemyWidth &&
              playerBullet.y >= enemies[i][j].y &&
              playerBullet.y <= enemies[i][j].y + enemyHeight) {
            enemies[i][j].alive = false;
            playerBullet.active = false;
            score += 10;
            inv_sound.freq=100;
            inv_sound.ms=50;
            playBeep(inv_sound);
            enemyMoveDelay+= -8;

          }
        }
      }
    }
  }

  // 全滅チェック
  bool anyEnemyAlive = false;
  for (int i = 0; i < enemyRows; i++) {
    for (int j = 0; j < enemyCols; j++) {
      if (enemies[i][j].alive) {
        anyEnemyAlive = true;
        break;
      }
    }
    if (anyEnemyAlive) break;
  }
  if (!anyEnemyAlive) {
    gameWin = true;
  }


  // 描画
  display.clearDisplay();
  
   // 敵の描画
  for (int i = 0; i < enemyRows; i++) {
    for (int j = 0; j < enemyCols; j++) {
      if (enemies[i][j].alive) {
        drawEnemy((int)enemies[i][j].x, (int)enemies[i][j].y,dflag);
      }
    }
  }

  // プレイヤーの描画
  drawPlayer(playerX, playerY);

  // プレイヤーの弾の描画
  if (playerBullet.active) {
    display.fillRect((int)playerBullet.x, (int)playerBullet.y, 1, 3, SSD1306_WHITE);
  }

  // 敵の弾の描画
  for (int i = 0; i < maxEnemyBullets; i++) {
    if (enemyBullets[i].active) {
      display.fillRect((int)enemyBullets[i].x, (int)enemyBullets[i].y, 1, 3, SSD1306_WHITE);
    }
  }

  // スコア表示
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  //display.print("S:");
  display.print(score);

  display.display();

  updateSound(1);

  //delay(1);
}

void drawPlayer(int x, int y) {
  // 三角形の自機
  display.fillTriangle(
    x + playerWidth/2, y,
    x, y + playerHeight,
    x + playerWidth, y + playerHeight,
    SSD1306_WHITE
  );
}

void drawEnemy(int x, int y,int ty) {
  // シンプルな敵の形
  display.fillRect(x + 1, y, 4, 2, SSD1306_WHITE);
  display.fillRect(x+1, y + 2, 4, 2, SSD1306_WHITE);
  if (ty<2){
    display.drawPixel(x, y + 4, SSD1306_WHITE);
    display.drawPixel(x + 5, y + 4, SSD1306_WHITE);
  }else{
    display.drawPixel(x, y + 4, SSD1306_WHITE);
    display.drawPixel(x + 5, y + 4, SSD1306_WHITE);
    display.drawPixel(x, y+1 , SSD1306_WHITE);
    display.drawPixel(x+5 , y+1 , SSD1306_WHITE);
  }
}

void displayEndScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);

  if (gameWin) {
    display.setCursor(20, 15);
    display.print("YOU WIN!");
    display.display();
    tone(BUZZER_PIN, 1100, 200);  // 低い音
    delay(250);
    tone(BUZZER_PIN, 1300, 200);  // さらに低い音
    delay(250);
    tone(BUZZER_PIN, 1500, 500);  // とても低い音
    delay(600);
    noTone(BUZZER_PIN);

  } else {
    display.setCursor(10, 15);
    display.print("GAME OVER");
    display.setCursor(10, 50);
    display.setTextSize(1);
    display.print("LEFT to restart");
  }

  display.setTextSize(1);
  display.setCursor(30, 35);
  display.print("Score: ");
  display.print(score);

  display.display();
}
