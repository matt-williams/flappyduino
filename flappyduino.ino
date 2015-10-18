#include <avr/pgmspace.h>
#include <SPI.h>

/*const byte font[] = {
#include "font.h"
};*/

const byte sprites[] = {
#include "sprites.h"
};

const byte spriteMasks[] = {
#include "sprites.mask.h"
};

const byte tiles[] = {
#include "tiles.h"
};

const byte collisionTile[] = {
  0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0
};

const byte map1[] = {
#include "map1.h"
};

const byte title[] = {
#include "title.h"
};

const byte titleMasks[] = {
#include "title.mask.h"
};

const byte PROGMEM titleMap[] = {
#include "title-map.h"
};

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
    _trigger = !digitalRead(TRIGGER);
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
  SpriteLayer() {}

  void reset(const byte* sprites, const byte* spriteMasks) {
    _sprites = sprites;
    _spriteMasks = spriteMasks;
    memset(_spriteMap, 0xff, sizeof(_spriteMap));
  }
  void addSprite(byte sprite, uint8_t x, uint8_t y) {
    if ((x >= 0) && (x < 21) && (y >= 0) && (y < 12)) {
      _spriteMap[x * 12 + y] = sprite;
    }
  }
  void add2x2Sprite(byte sprite, uint8_t x, uint8_t y) {
    addSprite(sprite, x, y);
    addSprite(sprite + 1, x, y + 1);
    addSprite(sprite + 2, x + 1, y);
    addSprite(sprite + 3, x + 1, y + 1);
  }
  void setSpriteMap(const byte PROGMEM* spriteMap) {
    for (int ii = 0; ii < sizeof(_spriteMap); ii++) {
      _spriteMap[ii] = pgm_read_byte_near(&spriteMap[ii]);
    }
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
  const byte* _sprites;
  const byte* _spriteMasks;
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

Map maps[] = {Map(tiles, map1, sizeof(map1) / 6)};
Joystick joystick = Joystick();
Display display = Display();
SpriteLayer spriteLayer = SpriteLayer();

class Mode {
public:
  virtual void reset();
  virtual void loop();

  static Mode* getCurrent() { return _current; }
  static void setCurrent(Mode* mode) {
    _current = mode;
    _current->reset();
  }

  static Mode* const TITLE;
  static Mode* const GAME;

private:
  static Mode* _current;
};
Mode* Mode::_current = NULL;

class TitleMode : public Mode {
public:
  TitleMode() { reset(); }

  void reset() {
    _currentMap = &maps[0];
    _x = 0;
  }
  
  void loop() {
    joystick.update();
    if ((joystick.getX() != 0) || (joystick.getY() != 0) || (joystick.getTrigger() != 0)) {
      Mode::setCurrent(Mode::GAME);
    }
    _x++;
    spriteLayer.reset(title, titleMasks);
    spriteLayer.setSpriteMap(titleMap);
    display.blit(_x, _currentMap, spriteLayer);
    _x = (_x > _currentMap->getWidth() * 8 - 88) ? 0 : _x;
  }

private:
  Map* _currentMap;
  uint16_t _x;
};
TitleMode titleMode = TitleMode();
Mode* const Mode::TITLE = &titleMode;

class GameMode : public Mode {
public:
  GameMode() { reset(); }

  void reset() {
    _startCount = 12;
    _dieCount = 0;
    _currentMap = &maps[0];
    _x = 0;
    _birdX = -8;
    _birdY = 20;
    for (uint8_t ii = 0; ii < MAX_BIRDS; ii++) {
      _enemyBirdX[ii] = 248;
      _enemyBirdY[ii] = 248;
    }
    _score = 0;
  }
  
  void loop() {
    if (_startCount > 0) {
      if (_startCount > 4) {
        _birdX += 4;
      } else {
        _birdX -= 4;
      }
      _startCount--;
    } else if (_dieCount > 0) {
      _birdY += 4;
      _dieCount--;
      display.setNegated((_dieCount + 3) >> 2);
      if (_dieCount == 0) {
        Mode::setCurrent(Mode::TITLE);
      }
    } else {
      joystick.update();
      if ((joystick.getX() < 0) && (_birdX > 0)) {
        _birdX -= 2;
      } else if ((joystick.getX() > 0) && (_birdX < 76)) {
        _birdX += 2;
      }
      if ((joystick.getY() < 0) && (_birdY > 0)) {
        _birdY -= 2;
      } else if ((joystick.getY() > 0) && (_birdY < 40)) {
        _birdY += 2;
      }
      _x++;
      if (_x % 32 == 0) {
        for (uint8_t ii = 0; ii < MAX_BIRDS; ii++) {
          if ((_enemyBirdX[ii] == 248) && (_enemyBirdY[ii] == 248)) {
            _enemyBirdX[ii] = 84;
            _enemyBirdY[ii] = (_x / 64 * 7) % 48;
            break;
          }
        }
      }
      if (_x % 64 == 0) {
        _score++;
      }
      _x = (_x > _currentMap->getWidth() * 8 - 88) ? 0 : _x;
      byte tile = _currentMap->getTile((_x + _birdX + 4) >> 3, (_birdY + 4) >> 3);
      if (collisionTile[tile]) {
        _dieCount = 12;
      }
    }
    spriteLayer.reset(sprites, spriteMasks);
    byte flap = ((_x >> 1) & 4);
    spriteLayer.add2x2Sprite(10 + flap, _birdX / 4, _birdY / 4);
    for (uint8_t ii = 0; ii < MAX_BIRDS; ii++) {
      if ((_enemyBirdX[ii] != 248) && (_enemyBirdY[ii] != 248)) {
        _enemyBirdX[ii]-= 2;
        if (_x % 4 == 0) {
          if (_enemyBirdY[ii] > _birdY) {
            _enemyBirdY[ii]--;
          }
          if (_enemyBirdY[ii] < _birdY) {
            _enemyBirdY[ii]++;
          }
        }
        if (_enemyBirdX[ii] == 248) {
          _enemyBirdY[ii] = 248;
        }
        if (_dieCount == 0) {
          int16_t deltaX = _birdX - _enemyBirdX[ii];
          int16_t deltaY = _birdY - _enemyBirdY[ii];
          if ((deltaX > -4) && (deltaX < 4) && (deltaY > -4) && (deltaY < 4)) {
            _dieCount = 12;
          }
        }
        spriteLayer.add2x2Sprite(18 + flap, _enemyBirdX[ii] / 4, _enemyBirdY[ii] / 4);
      }
    }
    drawScore();
    display.blit(_x, _currentMap, spriteLayer);
  }

private:
  static const uint8_t MAX_BIRDS = 3;

  Map* _currentMap;
  uint16_t _startCount;
  uint16_t _dieCount;
  uint16_t _x;
  uint8_t _birdX;
  uint8_t _birdY;
  uint8_t _enemyBirdX[MAX_BIRDS];
  uint8_t _enemyBirdY[MAX_BIRDS];
  uint16_t _score;

  void drawScore() {
    uint8_t scoreX = 20;
    uint16_t score = _score;
    do {
      spriteLayer.addSprite(score % 10, scoreX, 11);
      score = score / 10;
      scoreX--;
    } while (score > 0);
  }
};
GameMode gameMode = GameMode();
Mode* const Mode::GAME = &gameMode;

void setup()
{
  // TODO: Remove when debugging no longer required
  //Serial.begin(9600);

  joystick.begin();
  display.begin();
  Mode::setCurrent(Mode::TITLE);
}

void loop()
{
  Mode::getCurrent()->loop();
  delay(50);
}
