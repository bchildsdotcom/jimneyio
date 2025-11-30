#pragma once

#include "libraries/pico_graphics/pico_graphics.hpp"

using namespace pimoroni;

const uint16_t HEIGHT = 240;
const uint16_t WIDTH = 240;

struct Pens {
  Pen BLACK;
  Pen WHITE;
  Pen YELLOW;
  Pen LIGHT_BLUE;
  Pen SPRITE_TRANSPARENCY_LIGHT;
  Pen SPRITE_TRANSPARENCY_DARK;

  Pen SKY_BLUE_DAY;
  Pen GRASS_GREEN_DAY;

  
  Pen SKY_BLUE_NIGHT;
  Pen GRASS_GREEN_NIGHT;
};

struct Orientation {
  int pitch;
  int roll;

  Orientation(int pitch, int roll) : pitch(pitch), roll(roll) {} 
};

struct Line {
  Point p1;
  Point p2;
  
  Line(Point p1, Point p2) : p1(p1), p2(p2) {}
};

enum STATES {
  SPLASH = 0,
  ENVIRONMENT = 1,
  INCLINOMETER = 2,
};

enum UNITS {
  CELSIUS = 0,
  FARENHEIT = 1
};