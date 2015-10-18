#include <SPI.h>

class Joystick {
public:
  Joystick() {};

  void begin() {
    pinMode(TRIGGER, INPUT_PULLUP);
    // TODO: Don't apply power through an output pin!
    pinMode(POWER, OUTPUT);
    digitalWrite(POWER, HIGH);
  }

  void update() {
    delay(10);
    _x = (analogRead(X) / 64 - 8);
    _x = ((_x > -3) && (_x < 3)) ? 0 : _x;
    delay(10);
    _y = (analogRead(Y) / 64 - 8);
    _y = ((_y > -3) && (_y < 3)) ? 0 : _y;
    _trigger = digitalRead(TRIGGER);
  }

  int8_t getX() { return _x; }
  int8_t getY() { return _y; }
  uint8_t getTrigger() { return _trigger; }

private:
  const int POWER = A3;
  const int X = A2;
  const int Y = A1;
  const int TRIGGER = A0;

  int8_t _x;
  int8_t _y;
  uint8_t _trigger;
};

class Map {
public:
  Map(const byte* tileMap, uint16_t width) : _tileMap(tileMap), _width(width) {}

  byte getTile(uint16_t x, uint8_t y) const { return _tileMap[(x % _width) * 6 + y]; }
  uint16_t getWidth() { return _width; }

private:
  const byte* _tileMap;
  const uint16_t _width;
};

class Display {
public:
  Display() {};

  void begin()
  {
    SPI.begin();
    SPI.setClockDivider(SPI_CLOCK_DIV4);
    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);
    
    pinMode(CHIP_ENABLE, OUTPUT);
    pinMode(DATA_NOT_CONTROL, OUTPUT);
    pinMode(RESET, OUTPUT);
  
    digitalWrite(RESET, LOW);
    delay(500);
    digitalWrite(RESET, HIGH);
    digitalWrite(CHIP_ENABLE, LOW);
    
    digitalWrite(DATA_NOT_CONTROL, LOW);
    SPI.transfer(0x21); // extended commands
    SPI.transfer(0xc0); // Vop
    SPI.transfer(0x04); // temp coefficient
    SPI.transfer(0x14); // bias mode
    SPI.transfer(0x20); // basic commands

    setNegated(0);
    clear();
  }

  void clear()
  {
    digitalWrite(DATA_NOT_CONTROL, LOW);
    SPI.transfer(0x80); // x = 0
    SPI.transfer(0x40); // y = 0
    digitalWrite(DATA_NOT_CONTROL, HIGH);
    for (uint16_t ii = 0; ii < 84 * 48; ii ++) {
      SPI.transfer(0x00);
    }
  }

  void setNegated(byte negated) {
    digitalWrite(DATA_NOT_CONTROL, LOW);
    SPI.transfer(0x0c | negated);
  }

  void blit(uint8_t x, uint8_t y, uint8_t w, uint8_t h, byte* img)
  {
    for (uint8_t yi = y; yi < y + h; yi += 8) {
      digitalWrite(DATA_NOT_CONTROL, LOW);
      SPI.transfer(0x80 | x); // x
      SPI.transfer(0x40 | (yi >> 3)); // y
      digitalWrite(DATA_NOT_CONTROL, HIGH);
      for (uint8_t xi = 0; xi < w; xi++) {
        SPI.transfer(*(img++));  
      }
    }
  }

  void blit(uint16_t x, Map* map, byte* tiles)
  {
    digitalWrite(DATA_NOT_CONTROL, LOW);
    SPI.transfer(0x80); // x = 0
    SPI.transfer(0x40); // y = 0
    digitalWrite(DATA_NOT_CONTROL, HIGH);
    for (uint8_t tileY = 0; tileY < 6; tileY++) {
      for (uint8_t xi = 0; xi < 84; ) {
        uint16_t tileX = (x + xi) >> 3;
        uint8_t xOff = ((x + xi) & 7);
        byte* tile = &tiles[map->getTile(tileX, tileY) * 8];
        while ((xOff < 8) && (xi < 84)) {
          SPI.transfer(tile[xOff]);
          xOff++;
          xi++;
        }
      }
    }
  }

private:
  const int CHIP_ENABLE = 10;
  const int RESET = 9;
  const int DATA_NOT_CONTROL = 8;  
};

byte font[] = {
  0x3e, 0x41, 0x41, 0x41, 0x41, 0x41, 0x3e, 0x00, 0x40, 0x44, 0x42, 0x7f,
  0x40, 0x40, 0x40, 0x00, 0x42, 0x61, 0x51, 0x51, 0x49, 0x49, 0x46, 0x00,
  0x22, 0x41, 0x41, 0x49, 0x49, 0x49, 0x36, 0x00, 0x10, 0x18, 0x14, 0x12,
  0x79, 0x10, 0x10, 0x00, 0x27, 0x45, 0x45, 0x45, 0x45, 0x45, 0x39, 0x00,
  0x3e, 0x49, 0x49, 0x49, 0x49, 0x49, 0x31, 0x00, 0x01, 0x01, 0x01, 0x71,
  0x09, 0x05, 0x03, 0x00, 0x36, 0x49, 0x49, 0x49, 0x49, 0x49, 0x36, 0x00,
  0x46, 0x49, 0x49, 0x49, 0x49, 0x49, 0x3e, 0x00
};

byte sprites[] = {
  0x3c, 0x62, 0x52, 0x57, 0x69, 0x59, 0x2e, 0x30, 0x3c, 0x62, 0x82, 0x87,
  0x69, 0x59, 0x2e, 0x30
};

byte tiles[] = {
// TODO: Figure out why this doesn't work:
//#include "tiles.h"
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x02, 0x01, 0xa1,
  0x01, 0xa9, 0x01, 0xa9, 0x01, 0xa9, 0x01, 0xa9, 0x01, 0xa9, 0x01, 0xa9,
  0x01, 0xa9, 0x01, 0xa9, 0x01, 0xa9, 0x52, 0xfc, 0xff, 0x00, 0x00, 0xaa,
  0x00, 0xaa, 0x00, 0xaa, 0x00, 0xaa, 0x00, 0xaa, 0x00, 0xaa, 0x00, 0xaa,
  0x00, 0xaa, 0x00, 0xaa, 0x00, 0xaa, 0x55, 0xff, 0x3f, 0x40, 0x80, 0xaa,
  0xc0, 0xaa, 0xc0, 0xaa, 0xc0, 0xaa, 0xc0, 0xaa, 0xc0, 0xaa, 0xc0, 0xaa,
  0xc0, 0xaa, 0xc0, 0xaa, 0xc0, 0xaa, 0x55, 0x3f
};

const byte map1[] = {
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 7, 0, 0, 0, 0, 2, 5, 4, 0, 0, 0, 2, 5, 5, 0, 1, 4, 5, 5, 5, 0, 3, 5, 5, 6, 6, 0, 0, 3, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 4, 4, 7, 0, 0, 0, 5, 5, 8, 0, 0, 0, 5, 5, 9, 0, 0, 0, 5, 8, 0, 0, 0, 0, 6, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 5, 0, 0, 0, 0, 2, 5, 0, 0, 1, 4, 5, 5, 0, 0, 2, 
5, 5, 6, 0, 0, 3, 6, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 5, 7, 0, 0, 0, 0, 5, 8, 0, 0, 0, 0, 5, 5, 4, 7, 0, 0, 5, 5, 
5, 8, 0, 0, 5, 5, 5, 8, 0, 0, 5, 5, 5, 9, 0, 0, 5, 5, 8, 0, 0, 0, 5, 5, 9, 0, 0, 0, 5, 8, 0, 0, 0, 0, 5, 8, 0, 0, 0, 1, 5, 9, 0, 0, 0, 2, 8, 0, 0, 0, 1, 5, 8, 0, 0, 0, 2, 5, 8, 
0, 0, 1, 5, 5, 8, 0, 0, 2, 5, 5, 8, 0, 0, 3, 5, 5, 8, 0, 0, 0, 2, 5, 8, 0, 0, 0, 2, 5, 5, 7, 0, 0, 2, 5, 5, 8, 0, 0, 3, 5, 5, 8, 0, 0, 0, 2, 5, 8, 0, 0, 0, 2, 5, 9, 0, 0, 0, 2, 
8, 0, 0, 0, 0, 2, 8, 0, 0, 0, 0, 3, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0
};

Map maps[] = {Map(map1, sizeof(map1) / 6)};
Map* currentMap = &maps[0];

Joystick joystick = Joystick();
Display display = Display();
uint16_t x = 0;
uint8_t y = 24;

void setup()
{
  // start serial port at 9600 bps:
  Serial.begin(9600);

  joystick.begin();
  display.begin();
}

void loop()
{
  joystick.update();
  if ((joystick.getY() < 0) && (y > 0)) {
    y -= 8;
  } else if ((joystick.getY() > 0) && (y < 40)) {
    y += 8;
  }
  delay(50);
  x++;
  display.blit(x, currentMap, tiles);
  display.blit(8, y, 8, 8, &sprites[(x << 1) & 8]);
//  display.blit(0, tileMap, tiles);
  x = (x > currentMap->getWidth() * 8 - 88) ? 0 : x;
}
