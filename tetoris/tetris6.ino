#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <avr/pgmspace.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const int LEFT_BUTTON_PIN = 17;
const int RIGHT_BUTTON_PIN = 5;
const int O_BUTTON_PIN = 6;

const int BUZZER_PIN = 9;

/* 効果音（共通） */
#define S_R 1
#define S_GS2 2
#define S_A2 3
#define S_B2 4
#define S_C3 5
#define S_D3 6
#define S_E3 7
#define S_F3 8
#define S_G3 9
#define S_GS3 10
#define S_A3 11
#define S_B3 12
#define S_C4 13
#define S_D4 14
#define S_E4 15
#define S_F4 16
#define S_G4 17
#define S_A4 18

const char melody[] PROGMEM  = { S_E4, S_B3, S_C4, S_D4, S_C4, S_B3, S_A3, S_A3, S_C4 , S_E4, S_D4, S_C4, 
S_B3, S_B3 , S_C4, S_D4, S_E4, S_C4, S_A3, S_A3, S_R,
S_D4, S_F4, S_A4, S_G4, S_F4, S_E4, S_C4 ,S_E4,S_D4 ,S_C4,
S_B3,S_B3,S_C4,S_D4,S_E4,S_C4,S_A3,S_A3,S_R,
S_E3,S_C3,S_D3,S_B2,S_C3,S_A2,S_GS2,S_B2,S_E3,S_C3,
S_D3,S_B2,S_C3,S_E3,S_A3,S_A3,S_GS3
};

const char noteD[] PROGMEM  = { 4,8,8,4,8,8,4,8,8,4,8,8,4,8,8,4,4,4,4,4,8,4,8,4,8,8,3,8,4,8,8,4,8,8,4,4,4,4,4,4,2,2,2,2,2,2,2,2,2,2,2,2,4,4,4,4,1 };



// ゲームフィールド設定
const int FIELD_WIDTH = 10;
const int FIELD_HEIGHT = 20;
const int BLOCK_SIZE = 6;
const int FIELD_OFFSET_X = 2;
const int FIELD_OFFSET_Y = 2;

// フィールド (0=空, 1=ブロック)
byte field[FIELD_HEIGHT][FIELD_WIDTH];

// テトロミノの形状 (4x4の配列)
const byte TETROMINOS[7][4][4] = {
  // I
  {
    {0,0,0,0},
    {1,1,1,1},
    {0,0,0,0},
    {0,0,0,0}
  },
  // O
  {
    {0,0,0,0},
    {0,1,1,0},
    {0,1,1,0},
    {0,0,0,0}
  },
  // T
  {
    {0,0,0,0},
    {0,1,0,0},
    {1,1,1,0},
    {0,0,0,0}
  },
  // S
  {
    {0,0,0,0},
    {0,1,1,0},
    {1,1,0,0},
    {0,0,0,0}
  },
  // Z
  {
    {0,0,0,0},
    {1,1,0,0},
    {0,1,1,0},
    {0,0,0,0}
  },
  // J
  {
    {0,0,0,0},
    {1,0,0,0},
    {1,1,1,0},
    {0,0,0,0}
  },
  // L
  {
    {0,0,0,0},
    {0,0,1,0},
    {1,1,1,0},
    {0,0,0,0}
  }
};

// 現在のテトロミノ
int currentType = 0;
int currentX = 3;
int currentY = 0;
int currentRotation = 0;
byte currentShape[4][4];

// 次のテトロミノ
int nextType = 0;

// ゲーム状態
bool gameStarted = false;
bool gameOver = false;
bool soundPlayed = false;
int score = 0;
unsigned long lastFallTime = 0;
int fallDelay = 500;
unsigned long lastButtonTime = 0;
const int buttonDelay = 150;
int ky=0;


/* =========================
 *  ブザー音（D9=OC1A / Timer1）
 * ========================= */
struct Beep { uint16_t freq; uint16_t ms; };
bool     BUZZER_ENABLED   = true;
int8_t   BUZZER_SEMITONES = 0;

static volatile uint32_t g_matches_left = 0;
static volatile bool     g_running      = false;

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
/*

#define S_R 1
#define S_GS2 415
#define S_A2 440
#define S_B2 493
#define S_C3 523
#define S_D3 587
#define S_E3 659
#define S_F3 698
#define S_G3 783
#define S_GS3 830
#define S_A3 880
#define S_B3 987
#define S_C4 1046
#define S_D4 1174
#define S_E4 1318
#define S_F4 1396
#define S_G4 1568
#define S_A4 1760
*/
  Beep tetris_sound;
  static int i;
  static unsigned long ti;
  static int rep;
  if (f==0){i=0; return;}
  int t=0;
  int l=0;
  //if (g_running){return;}
  if (millis()<ti){return;}
  //t=melody[i];  
  t=pgm_read_byte(melody+i);  
  if (t==1){l=1;}
  if (t==2){l=415;}
  if (t==3){l=440;}
  if (t==4){l=493;}
  if (t==5){l=523;}
  if (t==6){l=587;}
  if (t==7){l=659;}
  if (t==8){l=698;}
  if (t==9){l=783;}
  if (t==10){l=830;}
  if (t==11){l=880;}
  if (t==12){l=987;}
  if (t==13){l=1046;}
  if (t==14){l=1174;}
  if (t==15){l=1318;}
  if (t==16){l=1396;}
  if (t==17){l=1568;}
  if (t==18){l=1760;}

  tetris_sound.freq=l;
  //t=noteD[i/2];
  t=pgm_read_byte(noteD+i);  
  //tetris_sound.ms=1200/t;
  tetris_sound.ms=1500/t;
  if (millis()>ti){ti=millis()+1600/t;}else{return;} 
  if (l!=1){
    playBeep(tetris_sound);
  }
  i++;
  if (i==40 && rep==0){i=0; rep=1;}
  if (i>=sizeof(melody)){i=0; rep=0;}
}


Beep SND_JUMP     = {1200, 60};
Beep SND_AIRJUMP  = {1050, 60};
Beep SND_OK       = {1100, 60};
Beep SND_CANCEL   = { 400,120};

void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setRotation(1);  // 画面ぢ90度回転（縦使用）
  pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(O_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  randomSeed(analogRead(0));
  nextType = random(7);  // 最初の次のブロックを決定
  //Serial.begin(9600);
  displayTitle();
  playBeep(SND_JUMP );
}

void displayTitle() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(15, 15);
  display.print("Tetris");
  display.setTextSize(1);
  display.setCursor(5, 40);
  display.print("Press L to start");
  display.display();
}

void initGame() {
  // フィールドをクリア
  for (int y = 0; y < FIELD_HEIGHT; y++) {
    for (int x = 0; x < FIELD_WIDTH; x++) {
      field[y][x] = 0;
    }
  }

  score = 0;
  gameOver = false;
  soundPlayed = false;
  spawnNewTetromino();
  ky=0;
}


void spawnNewTetromino() {
  currentType = nextType;  // 次のブロックを現在のブロックにする
  nextType = random(7);    // 新しい次のブロックを決定
  currentX = 3;
  currentY = 0;
  currentRotation = 0;

  // 形状をコピー
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++) {
      currentShape[y][x] = TETROMINOS[currentType][y][x];
    }
  }

  // スポーン位置に既にブロックがある場合はゲームオーバー
  if (checkCollision(currentX, currentY, currentShape)) {
    gameOver = true;
  }
}

void rotateTetromino() {
  byte newShape[4][4];

  // 90度回転
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++) {
      newShape[x][3-y] = currentShape[y][x];
    }
  }

  // 回転後の衝突チェック
  if (!checkCollision(currentX, currentY, newShape)) {
    for (int y = 0; y < 4; y++) {
      for (int x = 0; x < 4; x++) {
        currentShape[y][x] = newShape[y][x];
      }
    }
  }
}

bool checkCollision(int x, int y, byte shape[4][4]) {
  for (int py = 0; py < 4; py++) {
    for (int px = 0; px < 4; px++) {
      if (shape[py][px]) {
        int fieldX = x + px;
        int fieldY = y + py;

        // 壁のチェック
        if (fieldX < 0 || fieldX >= FIELD_WIDTH || fieldY >= FIELD_HEIGHT) {
          return true;
        }

        // 既存のブロックとの衝突チェック
        if (fieldY >= 0 && field[fieldY][fieldX]) {
          return true;
        }
      }
    }
  }
  return false;
}

void lockTetromino() {
  // テトロミノをフィールドに固定
  for (int py = 0; py < 4; py++) {
    for (int px = 0; px < 4; px++) {
      if (currentShape[py][px]) {
        int fieldY = currentY + py;
        int fieldX = currentX + px;
        if (fieldY >= 0 && fieldY < FIELD_HEIGHT && fieldX >= 0 && fieldX < FIELD_WIDTH) {
          field[fieldY][fieldX] = 1;
        }
      }
    }
  }

  // ライン消去チェック
  clearLines();

  // 新しいテトロミノを生成
  spawnNewTetromino();
}

void clearLines() {
  int linesCleared = 0;

  for (int y = FIELD_HEIGHT - 1; y >= 0; y--) {
    bool lineFull = true;
    for (int x = 0; x < FIELD_WIDTH; x++) {
      if (field[y][x] == 0) {
        lineFull = false;
        break;
      }
    }

    if (lineFull) {
      linesCleared++;

      // ラインを消して上のブロックを落とす
      for (int yy = y; yy > 0; yy--) {
        for (int x = 0; x < FIELD_WIDTH; x++) {
          field[yy][x] = field[yy - 1][x];
        }
      }

      // 一番上の行をクリア
      for (int x = 0; x < FIELD_WIDTH; x++) {
        field[0][x] = 0;
        playBeep(SND_AIRJUMP );
        delay(10);
        playBeep(SND_CANCEL);
        playBeep(SND_OK);
        delay(10);
        playBeep(SND_CANCEL);

      }
      //tone(BUZZER_PIN, 1400, 80);
      
 
      y++; // 同じ行を再チェック
    }
  }

  if (linesCleared > 0) {
    score += linesCleared * 10;
  }
}

void drawField() {
  // 枠線を描画
  display.drawRect(FIELD_OFFSET_X - 1, FIELD_OFFSET_Y - 1,
                   FIELD_WIDTH * BLOCK_SIZE + 2, FIELD_HEIGHT * BLOCK_SIZE + 2,
                   SSD1306_WHITE);

  // フィールドのブロックを描画
  for (int y = 0; y < FIELD_HEIGHT; y++) {
    for (int x = 0; x < FIELD_WIDTH; x++) {
      if (field[y][x]) {
        display.fillRect(FIELD_OFFSET_X + x * BLOCK_SIZE,
                        FIELD_OFFSET_Y + y * BLOCK_SIZE,
                        BLOCK_SIZE - 1, BLOCK_SIZE - 1, SSD1306_WHITE);
      }
    }
  }

  // 現在のテトロミノを描画
  for (int py = 0; py < 4; py++) {
    for (int px = 0; px < 4; px++) {
      if (currentShape[py][px]) {
        int drawY = currentY + py;
        if (drawY >= 0) {
          display.fillRect(FIELD_OFFSET_X + (currentX + px) * BLOCK_SIZE,
                          FIELD_OFFSET_Y + drawY * BLOCK_SIZE,
                          BLOCK_SIZE - 1, BLOCK_SIZE - 1, SSD1306_WHITE);
        }
      }
    }
  }
}

void drawNextTetromino() {
  // 右上に次のテトロミノを表示（回転後の座標系で）
  int previewX = 50;  // 少し右に移動
  int previewY = 6;   // 少し下に移動
  int previewSize = 2;

  // 次のテトロミノを描画
  for (int py = 0; py < 4; py++) {
    for (int px = 0; px < 4; px++) {
      if (TETROMINOS[nextType][py][px]) {
        display.fillRect(previewX + px * previewSize,
                        previewY + py * previewSize,
                        previewSize - 1, previewSize - 1, SSD1306_WHITE);
      }
    }
  }
}

void playGameOverSound() {
  // ゲームオーバー音を鳴らす（1回だけ）
  tone(BUZZER_PIN, 200, 200);  // 低い音
  delay(250);
  tone(BUZZER_PIN, 150, 200);  // さらに低い音
  delay(250);
  tone(BUZZER_PIN, 100, 400);  // とても低い音
  delay(450);
  noTone(BUZZER_PIN);
}

void displayGameOver() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(15, 15);
  display.print("GAME");
  display.setCursor(15, 35);
  display.print("OVER");
  display.setTextSize(1);
  display.setCursor(30, 55);
  display.print("Score:");
  display.print(score);
  display.display();
}




void loop() {
  // ゲーム開始前
  if (!gameStarted) {
    if (digitalRead(LEFT_BUTTON_PIN) == LOW) {
      delay(50);
      while (digitalRead(LEFT_BUTTON_PIN) == LOW) {
        delay(10);
      }
      gameStarted = true;
      initGame();
    }
    return;
  }

  // ゲームオーバー
  if (gameOver) {
    // BGMを止める
    noTone(BUZZER_PIN);
    updateSound(0);
    // 音を1回だけ鳴らす
    if (!soundPlayed) {
      playGameOverSound();
      soundPlayed = true;
      delay(1000);  // スコア表示時間
    }

    // リトライ待ち画面を表示
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(5, 20);
    display.print("Score:");
    display.print(score);
    display.setCursor(5, 35);
    display.print("Press L to retry");
    display.display();

    if (digitalRead(LEFT_BUTTON_PIN) == LOW) {
      delay(50);
      while (digitalRead(LEFT_BUTTON_PIN) == LOW) {
        delay(10);
      }
      initGame();
    }
    return;
  }

  unsigned long currentTime = millis();


  // ボタン入力
  if (currentTime - lastButtonTime > buttonDelay) {
    bool leftPressed = digitalRead(LEFT_BUTTON_PIN) == LOW;
    bool rightPressed = digitalRead(RIGHT_BUTTON_PIN) == LOW;
   bool oPressed = digitalRead(O_BUTTON_PIN) == LOW;

    if ( (leftPressed && rightPressed) || (oPressed)) {
      // 両方押されたら回転
      rotateTetromino();
      lastButtonTime = currentTime;
      ky=0;
      //tone(BUZZER_PIN, 1000, 20);
    } else if (leftPressed) {
      // 左移動
      if (!checkCollision(currentX - 1, currentY, currentShape)) {
        currentX--;
        //tone(BUZZER_PIN, 600, 20);
        playBeep(SND_JUMP );
        ky=0;

      }
      lastButtonTime = currentTime;
    } else if (rightPressed) {
      // 右移動
      if (!checkCollision(currentX + 1, currentY, currentShape)) {
        currentX++;
        //tone(BUZZER_PIN, 800, 20);
        playBeep(SND_AIRJUMP );
        ky=0;
      }
      lastButtonTime = currentTime;
    }
  }

  // 落下
  if (currentTime - lastFallTime > fallDelay) {
    if (!checkCollision(currentX, currentY + 1, currentShape)) {
      currentY++;
      ky++;
    } else {
      lockTetromino();
      fallDelay=500;
      ky=0;
    }
    if (ky>3){fallDelay=80;}else{fallDelay=500;}
    lastFallTime = currentTime;
  }


  // 描画
  display.clearDisplay();

  // 左上にスコアを表示（少し下、少し右に）
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(5, 8);
  display.print(score);

  // フィールドとテトロミノを描画
  drawField();

  // 次のブロックを表示
  drawNextTetromino();

  display.display();
  //playBeep(SND_JUMP );
  updateSound(1);
  delay(10);
}

