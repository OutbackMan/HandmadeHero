#include "hh.h"

void 
hh_render_gradient(HHPixelBuffer* restrict pixel_buffer, uint green_offset, uint blue_offset)
{
  u8* row = (u8 *)pixel_buffer->memory;
  for (uint y = 0; y < pixel_buffer->height; ++y) {
    u32* pixel = (u32 *)row;
    for (uint x = 0; x < pixel_buffer->width; ++x) {
      u32 green_value = x + green_offset;
      u32 blue_value = y + blue_offset;
      *pixel++ = (green_value << 8 | blue_value); 
    }
    row += pixel_buffer->pitch;
  }
}
