#include <string.h>
#include <math.h>
#include <vector>
#include <cstdlib>
#include <malloc.h>

#include "types.hpp"
#include "jimney.hpp"
#include "inclinometer.hpp"
#include "environment.hpp"
#include "state.hpp"

#include "pico.h"
#include "pico/flash.h"
#include "pico/multicore.h"

#include "drivers/button/button.hpp"
#include "drivers/st7789/st7789.hpp"
#include "rgbled.hpp"

uint32_t getTotalHeap(void) {
  extern char __StackLimit, __bss_end__;
  return &__StackLimit  - &__bss_end__;
}

uint32_t getFreeHeap(void) {
  struct mallinfo m = mallinfo();
  return getTotalHeap() - m.uordblks;
}

ST7789 st7789(WIDTH, HEIGHT, ROTATE_90, false, get_spi_pins(BG_SPI_FRONT));
PicoGraphics_PenRGB332 graphicsA(st7789.width, st7789.height, nullptr);
PicoGraphics_PenRGB332 graphicsB(st7789.width, st7789.height, nullptr);
Pens graphicsAPens;
Pens graphicsBPens;

enum GRAPHICS {
  GRAPHICS_NONE = 0,
  GRAPHICS_A = 1,
  GRAPHICS_B = 2,
};

GRAPHICS lastGraphics = GRAPHICS_NONE;
GRAPHICS currentGraphics = GRAPHICS_NONE;

RGBLED led(6, 7, 8);

Button buttonA(A);
Button buttonB(B);
Button buttonX(X);
Button buttonY(Y);

int loopTime = 0;
int frameTime = 0;
int renderTime = 0;

MODE mode = SPLASH;
UNIT units = CELSIUS;
bool statsEnabled = false;

void core1_entry() {
  flash_safe_execute_core_init();
  while (true) {
    GRAPHICS currentGraphicsSnapshot = currentGraphics;
    // update screen if the buffer was swapped
    if(currentGraphicsSnapshot != GRAPHICS_NONE && lastGraphics != currentGraphicsSnapshot) {
      auto updateStart = get_absolute_time();
      st7789.update(currentGraphicsSnapshot == GRAPHICS_A ? &graphicsA : &graphicsB);
      auto updateEnd = get_absolute_time();
      frameTime = absolute_time_diff_us(updateStart, updateEnd);
      
      // Turn on the screen after the first frame is rendered
      if(lastGraphics == GRAPHICS_NONE) {
        st7789.set_backlight(255);
      }

      lastGraphics = currentGraphicsSnapshot;
    } else {
      sleep_us(10);
    }
  }
}

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

void renderSplashFrame(PicoGraphics& graphics, Pens& pens) {
  graphics.set_pen(pens.BLACK);
  graphics.clear();

  drawJimny(graphics, pens, 56, 40, LIGHT);

  graphics.set_pen(pens.WHITE);
  graphics.text("Jimny I/O", Point(55, 170), WIDTH, 3);
  graphics.text("(c) 2025 Sunny and Rosita LLC", Point(50, 220), WIDTH, 1);
}

void renderStats(PicoGraphics& graphics, Pens& pens) {
  char stringBuffer[128];
  snprintf(stringBuffer, sizeof(stringBuffer), "C0 %dus, C1 %dus", (int)loopTime, (int)frameTime);
  Point text_location(0, 0);
  graphics.set_pen(pens.WHITE);
  graphics.text(stringBuffer, text_location, WIDTH, 2);
  
  snprintf(stringBuffer, sizeof(stringBuffer), "REN %dus, FMEM %ldk", (int)renderTime, getFreeHeap()/1024);
  text_location.y = 24;
  graphics.text(stringBuffer, text_location, WIDTH, 2);

  snprintf(stringBuffer, sizeof(stringBuffer), "SC %dus, FUB 0x%04X, LE: %d", (int)scanTime, firstUnusedByte, lastError);
  text_location.y = 48;
  graphics.text(stringBuffer, text_location, WIDTH, 1);
}

void renderFrame(PicoGraphics& graphics, Pens& pens) {
    auto render_start = get_absolute_time();

    switch(mode) {
      case SPLASH:
        renderSplashFrame(graphics, pens);
        break;

      case ENVIRONMENT:
        renderEnvironmentFrame(graphics, pens, units);
        break;
      
      case INCLINOMETER:
        renderInclinometerFrame(graphics, pens);
        break;
    }

    // Render Stats
    if(statsEnabled) {
      renderStats(graphics, pens);
    }
    
    auto render_end = get_absolute_time();
    renderTime = absolute_time_diff_us(render_start, render_end);
}

void processInput()
{
  if (buttonA.read())
  {
    if (mode == ENVIRONMENT)
    {
      units = units == CELSIUS ? FAHRENHEIT : CELSIUS;
    }
    mode = ENVIRONMENT;
  }

  if (buttonB.read())
  {
    mode = INCLINOMETER;
  }

  if (buttonX.read())
  {
    statsEnabled = true;
  }

  if (buttonY.read())
  {
    statsEnabled = false;
  }
}

int main() {
  stdio_init_all();
  st7789.set_backlight(0);
  printf("Initializing Jimney I/O");
  multicore_launch_core1(core1_entry);
  led.set_rgb(0,0,0);

  graphicsAPens = initGraphics(graphicsA);
  graphicsBPens = initGraphics(graphicsB);

  // Render Splash Screen Immediately
  renderFrame(graphicsA, graphicsAPens);
  currentGraphics = GRAPHICS_A;
  
  // Init Sensors
  initEnvironment();
  State savedState = loadState();
  
  mode = savedState.getMode();
  units = savedState.getUnits();
  
  // Show the pretty splash screen for a bit
  sleep_ms(1000);

  while(true) {
    processInput();

    // Render Frame on current framebuffer
    auto time_start = get_absolute_time();
    switch(currentGraphics) {
      case GRAPHICS_B:
        renderFrame(graphicsA, graphicsAPens);
        break;
      case GRAPHICS_A:
        renderFrame(graphicsB, graphicsBPens);
        break;
    }
    
    // Wait for current frame to finish rendering
    while(lastGraphics != currentGraphics) {
      sleep_us(10);
    }

    // Save state to persistent flash if required
    saveStateIfNeeded(State(mode, units));

    // Signal to render the next frame
    currentGraphics = currentGraphics == GRAPHICS_A ? GRAPHICS_B : GRAPHICS_A;

    auto time_end = get_absolute_time();
    loopTime = absolute_time_diff_us(time_start, time_end);
  }

  return 0;
}

