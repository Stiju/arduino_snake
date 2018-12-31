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
};

struct Position {
  char x, y;  
  bool operator==(const Position& other) const {
    return x == other.x && y == other.y;
  }
};

void draw_square(int px, int py) {
  lcd.drawPixel(px, py, 1);
  lcd.drawPixel(px+1, py, 1);
  lcd.drawPixel(px, py+1, 1);
  lcd.drawPixel(px+1, py+1, 1);
}

struct Player {
  Player() { reset(); }
  Position pos;
  Position tail[64];
  char direction;
  int size;
  void reset() {
    pos = {32,16};
    direction = 1;
    size = 6;
    memset(tail, 0, sizeof(tail));
  }
  void turn_left() {
    direction = (direction + 3) % 4;
  }
  void turn_right() {
    direction = (direction + 1) % 4;
  }
  void update() {
    for(int i = size; i > 0; --i) {
      tail[i] = tail[i - 1];
    }
    tail[0] = pos;
    switch(direction) {
      case 0: --pos.y; break;
      case 1: ++pos.x; break;
      case 2: ++pos.y; break;
      case 3: --pos.x; break;
    }
  }
  void render() const {
    int px = pos.x * 2, py = pos.y * 2;
    draw_square(px, py);
  }
};
Player player;
struct Item {
  Position pos;
  Item() : pos{4, 4} {}
  void render() const {
    int px = pos.x * 2, py = pos.y * 2;
    draw_square(px, py);
  }
};

Item item;

PushButton left_button{3}, right_button{2};
struct Screen {
  char data[256];
  void clear() {
    for(int i = 0; i < 256; ++i) {
      data[i] = 0;
    }
  }
  void set(int x, int y, bool val) {
    if(val) {
      data[x + (y >> 3) * kGameWidth] |= (1 << (y & 7));
    } else {
      data[x + (y >> 3) * kGameWidth] &= ~(1 << (y & 7));
    }
  }
  bool test(int x, int y) const {
    return (data[x + (y >> 3) * kGameWidth] & (1 << (y & 7))) != 0;
  }
  void render() {
    for(int y = 0; y < 32; ++y) {
      for(int x = 0; x < 64; ++x) {
        if(test(x, y)) {
          int px = x * 2, py = y * 2;
          draw_square(px, py);
        }
      }
    }
  }
};

Screen screen;
void reset_game() {
  screen.clear();
  for(int x = 0; x < kGameWidth; ++x) {
    screen.set(x, 0, 1);
    screen.set(x, 31, 1);
  }
  for(int y = 0; y < kGameHeight; ++y) {
    screen.set(0, y, 1);
    screen.set(63, y, 1);
  }
  player.reset();
  item.pos.x = random(1, 63);
  item.pos.y = random(1, 31);
}
void setup() {
  lcd.begin(SSD1306_SWITCHCAPVCC);
  reset_game();
}

void update() {
  screen.set(player.pos.x, player.pos.y, 1);
  player.update();
  Position tail = player.tail[(int)player.size];
  if(tail.x != 0 && tail.y != 0) {
    screen.set(tail.x, tail.y, 0);
  }
  if(player.pos.x < 0) player.pos.x += kGameWidth;
  if(player.pos.y < 0) player.pos.y += kGameHeight;
  if(player.pos.x >= kGameWidth) player.pos.x -= kGameWidth;
  if(player.pos.y >= kGameHeight) player.pos.y -= kGameHeight;

  if(player.pos == item.pos) {
    player.size++;
    item.pos.x = random(1, 63);
    item.pos.y = random(1, 31);
  }
  if(screen.test(player.pos.x, player.pos.y)) {
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
  lcd.clearDisplay();
  screen.render();
  player.render();
  item.render();
  lcd.display();
}

void loop() {
  input();
  update();
  render();
}

