#include "jimney.hpp"

void drawJimny(PicoGraphics& graphics, uint8_t offset_x, uint8_t offset_y, void* data, Pen transparency) {
  const uint8_t SPRITE_XMAX = 15;
  const uint8_t SPRITE_YMAX = 15;

  for(uint8_t sprite_y = 0; sprite_y < SPRITE_YMAX; sprite_y++) {
    for(uint8_t sprite_x = 0; sprite_x < SPRITE_XMAX; sprite_x++) {
      graphics.sprite(data, Point(sprite_x, sprite_y), Point((sprite_x*8)+offset_x, (sprite_y*8)+offset_y), 1, transparency);
    }
  }  
}