#include <Adafruit_SSD1306.h>

const int kScreenWidth = 128, kScreenHeight = 64, kGameWidth = 64, kGameHeight = 32;
const int OLED_MOSI = 9, OLED_CLK = 10, OLED_DC = 11, OLED_CS = 12, OLED_RESET = 13;

Adafruit_SSD1306 lcd(kScreenWidth, kScreenHeight, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);


class PushButton {
  char last_state, is_down, pin;
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
  char x, y;  
  bool operator==(const Position& other) const {
    return x == other.x && y == other.y;
  }
};

void draw_square(Position pos, int color = WHITE) {
  pos.x *= 2;
  pos.y *= 2;
  lcd.drawPixel(pos.x, pos.y, color);
  lcd.drawPixel(pos.x+1, pos.y, color);
  lcd.drawPixel(pos.x, pos.y+1, color);
  lcd.drawPixel(pos.x+1, pos.y+1, color);
}

bool test_position(Position pos) {
  return lcd.getPixel(pos.x * 2, pos.y * 2);
}

const Position kDirPos[4] = {
  {0,-1}, {1, 0}, {0, 1}, {-1, 0}
};

const int kTailSize = 464;
struct Player {
  Player() { reset(); }
  Position pos;
  char tail[kTailSize];
  char direction;
  int size, moved;
  void reset() {
    pos = {32,16};
    direction = 1;
    size = 6;
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
    for(int i = kTailSize - 1; i > 0; --i) {
      tail[i] = tail[i] << 2 | ((tail[i - 1] >> 6) & 3);
    }
    tail[0] = tail[0] << 2 | ((direction + 2) % 4);
    switch(direction) {
      case 0: --pos.y; break;
      case 1: ++pos.x; break;
      case 2: ++pos.y; break;
      case 3: --pos.x; break;
    }
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
      Position dir = kDirPos[(tail[(i >> 2)] >> ((i & 3) * 2)) & 3];
      tailpos.x += dir.x;
      tailpos.y += dir.y;
    }
    draw_square(tailpos, BLACK);
  }
} player;

struct Item {
  Position pos;
  Item() : pos{4, 4} {}
  void render() const {
    draw_square(pos);
  }
} item;

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
  item.pos.x = random(1, 63);
  item.pos.y = random(1, 31);
}

void update_game() {
  player.update();
  
  if(player.pos == item.pos) {
    player.size++;
    item.pos.x = random(1, 63);
    item.pos.y = random(1, 31);
  } else if(test_position(player.pos)) {
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
  lcd.begin(SSD1306_SWITCHCAPVCC);
  reset_game();
}

void loop() {
  input();
  update_game();
  render();
}
