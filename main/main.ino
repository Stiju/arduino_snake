#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
#include "bitmaps.h"

//#define USEISP
#define USEI2C

const int kScreenWidth = 128, kScreenHeight = 64, kGameWidth = 64, kGameHeight = 32, kMaxLength = 464, kStartLength = 6;

    

const int OLED_MOSI = 9, OLED_CLK = 10, OLED_DC = 11, OLED_CS = 12, OLED_RESET = 13;

#ifdef USEISP
  Adafruit_SSD1306 lcd(kScreenWidth, kScreenHeight, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
#endif
#ifdef USEI2C
  Adafruit_SSD1306 lcd(kScreenWidth, kScreenHeight, &Wire, -1);
#endif

class PushButton {
  unsigned char last_state, is_down, pin;
public:
  PushButton(int pin) : last_state(0), is_down(0), pin(pin) {
    pinMode(pin, INPUT);
  }
  void update() {
    int state = digitalRead(pin);
    if(state != last_state) {
      if(state == HIGH) {
        is_down = true;
      }
    }
    last_state = state;
  }
  bool get_state() {
    bool down = is_down;
    is_down = false;
    return down;
  }
} left_button{3}, right_button{2};

struct Position {
  signed char x, y;  
  bool operator==(const Position& other) const {
    return x == other.x && y == other.y;
  }
  Position& operator+=(const Position& other) {
    x += other.x;
    y += other.y;
    return *this;
  }
};

void draw_square(Position pos, int color = WHITE) {
  lcd.fillRect(pos.x * 2, pos.y * 2, 2, 2, color);
}

bool test_position(Position pos) {
  return lcd.getPixel(pos.x * 2, pos.y * 2);
}

const Position kDirPos[4] = {
  {0,-1}, {1, 0}, {0, 1}, {-1, 0}
};

struct Player {
  Player() { reset(); }
  Position pos;
  unsigned char tail[kMaxLength];
  unsigned char direction;
  int size, moved;
  void reset() {
    pos = {32,16};
    direction = 1;
    size = kStartLength;
    memset(tail, 0, sizeof(tail));
    moved = 0;
  }
  void turn_left() {
    direction = (direction + 3) % 4;
  }
  void turn_right() {
    direction = (direction + 1) % 4;
  }
  void update() {
    for(int i = kMaxLength - 1; i > 0; --i) {
      tail[i] = tail[i] << 2 | ((tail[i - 1] >> 6) & 3);
    }
    tail[0] = tail[0] << 2 | ((direction + 2) % 4);
    pos += kDirPos[direction];
    if(moved < size) {
      moved++;
    }
  }
  void render() const {
    draw_square(pos);
    if(moved < size) {
      return;
    }
    Position tailpos = pos;
    for(int i = 0; i < size; ++i) {
      tailpos += kDirPos[(tail[(i >> 2)] >> ((i & 3) * 2)) & 3];
    }
    draw_square(tailpos, BLACK);
  }
} player;

struct Item {
  Position pos;
  void spawn() {
    pos.x = random(1, 63);
    pos.y = random(1, 31);
  }
  void render() const {
    draw_square(pos);
  }
} item;

void wait_for_input() {
  do {
    right_button.update();
    left_button.update();
  } while(!right_button.get_state() && !left_button.get_state());
}

void push_to_start() {
  lcd.setCursor(26,57);
  lcd.print(F("Push to start"));
}

void flash_screen() {
  lcd.invertDisplay(true);
  delay(100);
  lcd.invertDisplay(false);
  delay(200);
}

void play_intro() {
  lcd.clearDisplay();
  lcd.drawBitmap(18, 0, kSplashScreen, 92, 56, WHITE);
  push_to_start();
  lcd.display();
  wait_for_input();
  flash_screen();
}

void play_gameover() {
  flash_screen();
  lcd.clearDisplay();
  lcd.drawBitmap(4, 0, kGameOver, 124, 38, WHITE);
  int score = player.size - kStartLength;
  lcd.setCursor(26, 34);
  lcd.print(F("Score: "));
  lcd.print(score);
  int hiscore;
  EEPROM.get(0, hiscore);
  if(score > hiscore) {
    EEPROM.put(0, score);
    hiscore = score;
    lcd.setCursor(4, 44);
    lcd.print(F("NEW"));
  }
  lcd.setCursor(26, 44);
  lcd.print(F("Hi-Score: "));
  lcd.print(hiscore);
  push_to_start();
  lcd.display();
  wait_for_input();
}

void reset_game() {
  lcd.clearDisplay();
  for(char x = 0; x < kGameWidth; ++x) {
    draw_square({x, 0});
    draw_square({x, 31});
  }
  for(char y = 0; y < kGameHeight; ++y) {
    draw_square({0, y});
    draw_square({63, y});
  }
  player.reset();
  item.spawn();
}

void update_game() {
  player.update();
  
  if(player.pos == item.pos) {
    player.size++;
    item.spawn();
  } else if(test_position(player.pos)) {
    play_gameover();
    reset_game();
  }
}

void input() {
  right_button.update();
  if(right_button.get_state()) {
    player.turn_right();
  }
  
  left_button.update();
  if(left_button.get_state()) {
    player.turn_left();
  }
}

void render() {
  player.render();
  item.render();
  lcd.display();
}

void setup() {
#ifdef USEISP
  lcd.begin(SSD1306_SWITCHCAPVCC);
#endif
#ifdef USEI2C
  lcd.begin(SSD1306_SWITCHCAPVCC, 0x3C);
#endif

  lcd.setTextColor(WHITE);
  play_intro();
  reset_game();
}

void loop() {
  input();
  update_game();
  render();
}
