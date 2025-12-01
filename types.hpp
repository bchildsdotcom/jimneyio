#pragma once

#include "libraries/pico_graphics/pico_graphics.hpp"

using namespace pimoroni;

static const int WIDTH = 240;
static const int HEIGHT = 240;

static const uint A = 12;
static const uint B = 13;
static const uint X = 14;
static const uint Y = 15;

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

enum STATE {
  SPLASH = 0,
  ENVIRONMENT = 1,
  INCLINOMETER = 2,
};

enum UNIT {
  CELSIUS = 0,
  FAHRENHEIT = 1
};