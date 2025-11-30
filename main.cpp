#include <string.h>
#include <math.h>
#include <vector>
#include <cstdlib>
#include <malloc.h>

#include "types.hpp"
#include "jimney.hpp"
#include "inclinometer.hpp"
#include "environment.hpp"

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

RGBLED led(6, 7, 8);

Button buttonA(A);
Button buttonB(B);
Button buttonX(X);
Button buttonY(Y);

enum GRAPHICS {
  GRAPHICS_A = 1,
  GRAPHICS_B = 2
};

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
}

uint8_t state = SPLASH;
bool statsEnabled = false;

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
    if(statsEnabled) {
      renderStats(graphics, pens);
    }
    
    auto render_end = get_absolute_time();
    renderTime = absolute_time_diff_us(render_start, render_end);
}

int main() {
  stdio_init_all();
  printf("Initializing Jimney I/O");
  multicore_launch_core1(core1_entry);
  led.set_rgb(0,0,0);

  graphicsAPens = initGraphics(graphicsA);
  graphicsBPens = initGraphics(graphicsB);

  // Render Splash Screen Immediately
  renderFrame(graphicsA, graphicsAPens);
  multicore_fifo_push_blocking(GRAPHICS_A);
  
  // Init Sensors
  initEnvironment();

  auto currentGraphics = GRAPHICS_B;
  uint32_t currentFrame = 0;

  while(true) {
    // TODO Process Inputs
    if(buttonA.read()) {
      state = ENVIRONMENT;
    }
    if(buttonB.read()) {
      state = INCLINOMETER;
    }
    
    if(buttonX.read()) {
      statsEnabled = true;
    }

    // Swap away from splash screen after 100 frames
    if(state == SPLASH && currentFrame > 100)
      state = ENVIRONMENT;

    // Render Frame on current framebuffer
    auto time_start = get_absolute_time();
    switch(currentGraphics) {
      case GRAPHICS_A:
        renderFrame(graphicsA, graphicsAPens);
        break;
      case GRAPHICS_B:
        renderFrame(graphicsB, graphicsBPens);
        break;
    }

    // Wait for the previous frame to finish
    multicore_fifo_pop_blocking();

    // Send the next frame
    multicore_fifo_push_blocking(currentGraphics);

    // Swap Current Graphics
    currentGraphics = currentGraphics == GRAPHICS_A ? GRAPHICS_B : GRAPHICS_A;

    auto time_end = get_absolute_time();
    loopTime = absolute_time_diff_us(time_start, time_end);
    currentFrame++;
  }

  return 0;
}
