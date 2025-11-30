#include <string.h>
#include <math.h>
#include <vector>
#include <cstdlib>
#include <malloc.h>

#include "pico/multicore.h"
#include "drivers/st7789/st7789.hpp"
#include "libraries/pico_graphics/pico_graphics.hpp"
#include "rgbled.hpp"

#include "drivers/bme68x/bme68x.hpp"
#include "common/pimoroni_i2c.hpp"
#include "image_data.hpp"

uint32_t getTotalHeap(void) {
   extern char __StackLimit, __bss_end__;
   
   return &__StackLimit  - &__bss_end__;
}

uint32_t getFreeHeap(void) {
   struct mallinfo m = mallinfo();

   return getTotalHeap() - m.uordblks;
}

using namespace pimoroni;

const uint16_t HEIGHT = 240;
const uint16_t WIDTH = 240;

ST7789 st7789(240, 240, ROTATE_90, false, get_spi_pins(BG_SPI_FRONT));
PicoGraphics_PenRGB332 graphicsA(st7789.width, st7789.height, nullptr);
PicoGraphics_PenRGB332 graphicsB(st7789.width, st7789.height, nullptr);

I2C i2c(BOARD::BREAKOUT_GARDEN);
BME68X bme68x(&i2c, BME68X::ALTERNATE_I2C_ADDRESS);

RGBLED led(6, 7, 8);
uint32_t GRAPHICS_A = 1;
uint32_t GRAPHICS_B = 2;

int loopTime = 0;
int frameTime = 0;
int renderTime = 0;

void core1_entry() {
    st7789.set_backlight(255);
    while (true) {
      uint32_t g = multicore_fifo_pop_blocking();

      auto updateStart = get_absolute_time();
      // update screen
      st7789.update(g == GRAPHICS_A ? &graphicsA : &graphicsB);
      auto updateEnd = get_absolute_time();
      frameTime = absolute_time_diff_us(updateStart, updateEnd);

      multicore_fifo_push_blocking(g);
    }
}

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

Pens graphicsAPens;
Pens graphicsBPens;

Pens initGraphics(PicoGraphics& graphics) {
  Pens pens;
  pens.BLACK = graphics.create_pen(0, 0, 0);
  pens.WHITE = graphics.create_pen(255,255,255);
  pens.YELLOW = graphics.create_pen(242,203,0);
  pens.LIGHT_BLUE = graphics.create_pen(93, 177, 247);

  pens.SPRITE_TRANSPARENCY_LIGHT = graphics.create_pen(146, 146, 146);
  pens.SPRITE_TRANSPARENCY_DARK = graphics.create_pen(213,213,213);

  
  pens.SKY_BLUE_DAY = graphics.create_pen(139, 214, 245);
  pens.GRASS_GREEN_DAY = graphics.create_pen(186,234,147);

  pens.SKY_BLUE_NIGHT = graphics.create_pen(30, 50, 117);
  pens.GRASS_GREEN_NIGHT = graphics.create_pen(0,145,104);

  graphics.set_font("bitmap8");
  graphics.set_pen(pens.BLACK);
  graphics.clear();
  return pens;
}

enum states {
  SPLASH = 0,
  ENVIRONMENT = 1,
  INCLINOMETER = 2,
};

uint8_t state = SPLASH;
char stringBuffer[256];

const float MIN_GAS = 100000.0;
const float MAX_GAS = 0.0;
const int ALTITUDE = 0;

enum UNITS {
  CELSIUS = 0,
  FARENHEIT = 1
};

int units = CELSIUS;

struct Orientation {
  int pitch;
  int roll;

  Orientation(int pitch, int roll) : pitch(pitch), roll(roll) {} 
};


int testPitch = 0;
int testRoll = 0;
const int MAX_TEST_PITCH = 16;
const int MAX_TEST_ROLL = 5;
bool isPitchReversing = false;
bool isRollReversing = false;

// allows for exagerating changes in pitch and roll for ease of reading
const int ROLL_SCALING = 2;
const int PITCH_SCALING = 2;

Orientation generateTestOrientationData() {
  if(testPitch < MAX_TEST_PITCH && !isPitchReversing) {
    testPitch += 2;
    if(testPitch == MAX_TEST_PITCH) isPitchReversing = true;
  }
  else if(testPitch > -1*MAX_TEST_PITCH && isPitchReversing) 
  {
    testPitch -= 2;
    if(testPitch == -1*MAX_TEST_PITCH) isPitchReversing = false;
  }

  if(testRoll < MAX_TEST_ROLL && !isRollReversing) {
    testRoll += 1;
    if(testRoll == MAX_TEST_ROLL) isRollReversing = true;
  }
  else if(testRoll > -1*MAX_TEST_ROLL && isRollReversing) 
  {
    testRoll -= 1;
    if(testRoll == -1*MAX_TEST_ROLL) isRollReversing = false;
  }

  return Orientation(testPitch, testRoll);  
}


int orientationSamplesCount = 0;
float averageRoll = 0;
float averagePitch = 0; 
Orientation calculateOrientation() {
  auto current = generateTestOrientationData();

  if(orientationSamplesCount < 5) {
    orientationSamplesCount++;
    averageRoll += current.roll;
    averageRoll /= orientationSamplesCount;

    averagePitch += current.pitch;
    averagePitch /= orientationSamplesCount;

    return Orientation(0,0);
  } 

  averageRoll = averageRoll * 0.8 + current.roll * 0.2;
  averagePitch = averagePitch * 0.8 + current.pitch * 0.2;
  return Orientation(
    averagePitch * PITCH_SCALING,
    averageRoll * ROLL_SCALING);
}

struct Line {
  Point p1;
  Point p2;
  
  Line(Point p1, Point p2) : p1(p1), p2(p2) {}
};

Line rotateLine(Line line, float deg) {
   float theta = deg * M_PI / 180;
   float cosang = cos(theta);
   float sinang = sin(theta);
   
   auto cx = (line.p1.x + line.p2.x) / 2;
   auto cy = (line.p1.y + line.p2.y);
  
   auto tx1 = line.p1.x - cx;
   auto ty1 = line.p1.y - cy;

   auto p1 = Point(tx1*cosang + ty1*sinang + cx, -tx1*sinang+ty1*cosang + cy);

   auto tx2 = line.p2.x - cx;
   auto ty2 = line.p2.y - cy;

   auto p2 = Point(tx2*cosang + ty2*sinang + cx, -tx2*sinang+ty2*cosang + cy);

   return Line(p1, p2);
}

int det(Point a, Point b)
{
  return a.x * b.y - a.y*b.x;
}

Point lineIntersection(Line l1, Line l2) 
{
  auto xDiff = Point(l1.p1.x - l1.p2.x,l2.p1.x - l2.p2.x);
  auto yDiff = Point(l1.p1.y - l1.p2.y,l2.p1.y - l2.p2.y);
  
  auto div = det(xDiff, yDiff);
  if (div == 0)
    return Point(INT_MAX, INT_MAX);

  auto d = Point(det(l1.p1, l1.p2), det(l2.p1, l2.p2));
  return Point(det(d, xDiff) / div, det(d, yDiff) / div);
}

float adjustToSeaPressure(float pressureHpa, float temperature, float altitude) {
    
    // Adjust pressure based on your altitude.
    // credits to @cubapp https://gist.github.com/cubapp/23dd4e91814a995b8ff06f406679abcf
  
    // Adjusted-to-the-sea barometric pressure
    return pressureHpa + ((pressureHpa * 9.80665 * altitude) / (287 * (273 + temperature + (altitude / 400))));
}


void renderEnvironmentFrame(PicoGraphics& graphics, Pens& pens) {
    const int TEMPERATURE_OFFSET = 9;

    bme68x_data data;
    auto result = bme68x.read_forced(&data, 300, 100);
    (void)result;
    
    graphics.set_pen(pens.BLACK);
    graphics.clear();

    // sprintf(stringBuffer, "%dus, %dus", frameTime, loopTime);
    // Point text_location(0, 0);
    // graphics.set_pen(pens.WHITE);

    // sprintf(stringBuffer, "%.2f, %.2f, %.2f, %.2f, 0x%x",
    //     data.temperature,
    //     data.pressure,
    //     data.humidity,
    //     data.gas_resistance,
    //     data.status);
        
    // graphics.text(stringBuffer, text_location, 240);

    auto correctedTemperature = data.temperature - TEMPERATURE_OFFSET;
    auto dewpoint = data.temperature - ((100 - data.humidity) / 5);
    auto correctedHumidity = 100 - (5 * (correctedTemperature - dewpoint));
    auto correctedTemperatureF = (correctedTemperature * 9/5)+32;

    // auto gas = MAX(MIN(MAX_GAS, data.gas_resistance), MIN_GAS);
    // auto pressureHpa = adjustToSeaPressure(data.pressure / 100, data.temperature, ALTITUDE);

    if(data.status & BME68X_HEAT_STAB_MSK) 
    {        
        char primaryBuffer[16], secondaryBuffer[16];

        if(units == CELSIUS) {
          sprintf(primaryBuffer, "%.0f°C", correctedTemperature);
          sprintf(secondaryBuffer, "%.0f°F", correctedTemperatureF);
        }
        else {
          sprintf(primaryBuffer, "%.0f°F", correctedTemperatureF);
          sprintf(secondaryBuffer, "%.0f°C", correctedTemperature);
        }

        uint16_t primaryTextWidth = graphics.measure_text(primaryBuffer, 8) - 8;
        uint16_t secondaryTextWidth = graphics.measure_text(secondaryBuffer, 3) - 3;
        uint16_t centeredPrimaryTextX = (WIDTH-primaryTextWidth) / 2;
        uint16_t centeredSecondaryTextX = WIDTH-secondaryTextWidth - 12;

        graphics.set_pen(pens.YELLOW);
        graphics.text(primaryBuffer, Point(centeredPrimaryTextX, 90), false, 8);

        graphics.set_pen(pens.WHITE);
        graphics.text(secondaryBuffer, Point(centeredSecondaryTextX, 205), false, 3);

        sprintf(primaryBuffer, "%.0f%%", correctedHumidity);
        graphics.text(primaryBuffer, Point(42, 205), false, 3);

        
        graphics.set_pen(pens.LIGHT_BLUE);
        graphics.circle(Point(25, 219), 8);

        std::vector<Point> poly;
        poly.push_back(Point(19, 215));
        poly.push_back(Point(25, 205));
        poly.push_back(Point(31, 215));
        graphics.polygon(poly);
    }

}


void drawJimny(PicoGraphics& graphics, uint8_t offset_x, uint8_t offset_y, void* data, Pen transparency) {
  const uint8_t SPRITE_XMAX = 15;
  const uint8_t SPRITE_YMAX = 15;

  for(uint8_t sprite_y = 0; sprite_y < SPRITE_YMAX; sprite_y++) {
    for(uint8_t sprite_x = 0; sprite_x < SPRITE_XMAX; sprite_x++) {
      graphics.sprite(data, Point(sprite_x, sprite_y), Point((sprite_x*8)+offset_x, (sprite_y*8)+offset_y), 1, transparency);
    }
  }  
}

void renderInclinometerFrame(PicoGraphics& graphics, Pens& pens) {

  auto orientation = calculateOrientation();

  uint8_t yOffset = 120+orientation.pitch;

  Line line = rotateLine(Line(Point(0, yOffset),Point(240, yOffset)), orientation.roll);
  Point leftEdgePoint = line.p1;
  Point rightEdgePoint = line.p2;
  leftEdgePoint = lineIntersection(line, Line(Point(0,0),Point(0,240)));
  rightEdgePoint = lineIntersection(line, Line(Point(240, 0), Point(240, 240)));

  graphics.set_pen(pens.SKY_BLUE_DAY);
  graphics.clear();

  graphics.set_pen(pens.GRASS_GREEN_DAY);

  std::vector<Point> poly;
  poly.push_back(leftEdgePoint);
  poly.push_back(rightEdgePoint);
  poly.push_back(Point(240,240));
  poly.push_back(Point(0,240));

  graphics.polygon(poly);

  Pen cartesianLinesPen = pens.BLACK;
  Pen spriteTransparencyPen = pens.SPRITE_TRANSPARENCY_DARK;

  graphics.set_pen(cartesianLinesPen);
  graphics.line(Point(120, 0), Point(120, 70));
  graphics.line(Point(120, 170), Point(120, 240));
  graphics.line(Point(0, 120), Point(70, 120));
  graphics.line(Point(170, 120), Point(240, 120));

  drawJimny(graphics, 56, 56, jimny_icon_dark_v9, spriteTransparencyPen);
}

void renderSplashFrame(PicoGraphics& graphics, Pens& pens) {
    graphics.set_pen(pens.BLACK);
    graphics.clear();

    drawJimny(graphics, 56, 40, jimny_icon_light_v9, pens.SPRITE_TRANSPARENCY_LIGHT);

    graphics.set_pen(pens.WHITE);
    graphics.text("Jimny I/O", Point(55, 170), WIDTH, 3);
    graphics.text("(c) 2025 Sunny and Rosita LLC", Point(50, 220), WIDTH, 1);

}

void renderFrame(PicoGraphics& graphics, Pens& pens) {

    auto render_start = get_absolute_time();

    switch(state) {
      case SPLASH:
        renderSplashFrame(graphics, pens);
        break;

      case ENVIRONMENT:
        renderEnvironmentFrame(graphics, pens);
        break;
      
      case INCLINOMETER:
        renderInclinometerFrame(graphics, pens);
        break;
    }

    // Render Stats

    sprintf(stringBuffer, "C0 %dus, C1 %dus", loopTime, frameTime);
    Point text_location(0, 0);
    graphics.set_pen(pens.WHITE);
    graphics.text(stringBuffer, text_location, 240);
    
    sprintf(stringBuffer, "Ren: %dus, Mem: %ldk", renderTime, getFreeHeap()/1024);
    text_location.y = 24;
    graphics.text(stringBuffer, text_location, 240);
    auto render_end = get_absolute_time();

    renderTime = absolute_time_diff_us(render_start, render_end);
}

int main() {
  stdio_init_all();
  printf("Hello, multicore!\n");
  multicore_launch_core1(core1_entry);
  jimny_icon_dark_v9[0] = 0xdb;
  jimny_icon_light_v9[0] = 0x92;
  led.set_rgb(0,0,0);

  graphicsAPens = initGraphics(graphicsA);
  graphicsBPens = initGraphics(graphicsB);

  // Render Splash Screen Immediately
  renderFrame(graphicsA, graphicsAPens);
  multicore_fifo_push_blocking(GRAPHICS_A);
  
  // Init Sensors
  bme68x.init();

  sleep_ms(1000);

  state = INCLINOMETER;

  auto currentGraphics = GRAPHICS_B;
  while(true) {
    auto time_start = get_absolute_time();
    if(currentGraphics == GRAPHICS_A) {
      renderFrame(graphicsA, graphicsAPens);
      multicore_fifo_pop_blocking();
      multicore_fifo_push_blocking(GRAPHICS_A);
      currentGraphics = GRAPHICS_B;
    }
    else {
      renderFrame(graphicsB, graphicsBPens);
      multicore_fifo_pop_blocking();
      multicore_fifo_push_blocking(GRAPHICS_B);
      currentGraphics = GRAPHICS_A;
    }  
    auto time_end = get_absolute_time();
    loopTime = absolute_time_diff_us(time_start, time_end);
  }

    return 0;
}
