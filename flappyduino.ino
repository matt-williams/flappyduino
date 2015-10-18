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
    _x = (analogRead(X) / 64 - 8);
    _x = ((_x > -3) && (_x < 3)) ? 0 : _x;
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
  Map(const byte *tiles, const byte* tileMap, uint16_t width) : _tiles(tiles), _tileMap(tileMap), _width(width) {}

  byte getTile(uint16_t x, uint8_t y) const { return _tileMap[(x % _width) * 6 + y]; }
  byte getPixels(byte tile, uint8_t xOff) const { return _tiles[tile * 8 + xOff]; }
  uint16_t getWidth() { return _width; }

private:
  const byte* _tiles;
  const byte* _tileMap;
  const uint16_t _width;
};

class SpriteLayer {
public:
  SpriteLayer(byte* sprites, byte* spriteMasks) : _sprites(sprites), _spriteMasks(spriteMasks) {}

  void reset() { memset(_spriteMap, 0xff, sizeof(_spriteMap)); }
  void addSprite(byte sprite, uint8_t x, uint8_t y) { _spriteMap[x * 12 + y] = sprite; }
  void add2x2Sprite(byte sprite, uint8_t x, uint8_t y) {
    addSprite(sprite, x, y);
    addSprite(sprite + 1, x, y + 1);
    addSprite(sprite + 2, x + 1, y);
    addSprite(sprite + 3, x + 1, y + 1);
  }
  byte getSprite(uint8_t x, uint8_t y) const { return _spriteMap[x * 12 + y]; }
  byte maskPixels(byte inputPixels, uint8_t x, uint8_t y) const {
    uint8_t spriteX = x >> 2;
    uint8_t spriteY = y << 1;
    uint8_t xOff = x & 3;
    return maskPixels(inputPixels, getSprite(spriteX, spriteY), getSprite(spriteX, spriteY + 1), xOff);
  }
  byte maskPixels(byte inputPixels, byte top, byte bottom, uint8_t xOff) const {
    if ((top != 0xff) || (bottom != 0xff)) {
      return maskPixelsSprite(inputPixels & 0xf, top, xOff) | (maskPixelsSprite(inputPixels >> 4, bottom, xOff) << 4);
    } else {
      return inputPixels;
    }
  }
  byte maskPixelsSprite(byte inputPixels, byte sprite, uint8_t xOff) const {
    if (sprite != 0xff) {
      uint16_t off = (sprite >> 1) * 4 + xOff;
      uint8_t shift = (sprite & 1) << 2;
      return (inputPixels & ((~_spriteMasks[off]) >> shift) | (_sprites[off] >> shift)) & 0x0f;
    } else {
      return inputPixels;
    }
  }

private:
  byte _spriteMap[21 * 12];
  byte* _sprites;
  byte* _spriteMasks;
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
    delay(10);
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

  void blit(uint16_t x, const Map* map, const SpriteLayer& spriteLayer)
  {
    digitalWrite(DATA_NOT_CONTROL, LOW);
    SPI.transfer(0x80); // x = 0
    SPI.transfer(0x40); // y = 0
    digitalWrite(DATA_NOT_CONTROL, HIGH);
    for (uint8_t tileY = 0; tileY < 6; tileY++) {
      for (uint8_t xi = 0; xi < 84; ) {
        uint16_t tileX = (x + xi) >> 3;
        uint8_t xOff = ((x + xi) & 7);
        byte tile = map->getTile(tileX, tileY);
        while ((xOff < 8) && (xi < 84)) {
          byte pixels = map->getPixels(tile, xOff);
          pixels = spriteLayer.maskPixels(pixels, xi, tileY);
          SPI.transfer(pixels);
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
#include "font.h"
};

byte sprites[] = {
#include "sprites.h"
};

byte spriteMasks[] = {
#include "sprites.mask.h"
};

byte tiles[] = {
#include "tiles.h"
};

const byte map1[] = {
#include "map1.h"
};

Map maps[] = {Map(tiles, map1, sizeof(map1) / 6)};
Map* currentMap = &maps[0];
SpriteLayer spriteLayer = SpriteLayer(sprites, spriteMasks);

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
    y -= 4;
  } else if ((joystick.getY() > 0) && (y < 40)) {
    y += 4;
  }
  delay(50);
  x++;
  spriteLayer.reset();
  spriteLayer.add2x2Sprite(/* sprite = */ (x >> 1) & 4, 2, y / 4);
  display.blit(x, currentMap, spriteLayer);
  x = (x > currentMap->getWidth() * 8 - 88) ? 0 : x;
}
