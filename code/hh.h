#if !defined(HH_H)
#define HH_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define GLOBAL static
#define INTERNAL static
#define PERSIST static

typedef unsigned int uint;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

#define IS_SAME_TYPE(var1, var2) \
  (__builtin_types_compatible_p(typeof(var1), typeof(var1)))

#define ARRAY_LENGTH(arr) \
  (sizeof(arr)/sizeof((arr)[0])) + \
    (!IS_SAME_TYPE(arr, &(arr)[0])) ? (_Pragma("GCC Error \"ARRAY_LENGTH macro did not recieve expected array input\"")) : 0

#define SWAP(var1, var2) \
  ({typeof(var1) temp = var1; var1 = var2; var2 = temp;}) + \
    (!IS_SAME_TYPE(var1, var2)) ? (_Pragma("GCC Error \"SWAP macro did not recieve inputs of same type\"")) : 0

#define MIN(var1, var2) \
  ((var1) < (var2) ? (var1) : (var2)) \
    + RAISE_ERROR_ON_ZERO(IS_SAME_TYPE(var1, var2))

#define MAX(var1, var2) \
  ((var1) > (var2) ? (var1) : (var2)) \
    + RAISE_ERROR_ON_ZERO(IS_SAME_TYPE(var1, var2))

#define IS_BIT_SET(bitmask, bit) \
  ((bitmask) & (1 << (bit)))

typedef struct {
  // NOTE(Ryan): These pixels are arranged BB GG RR AA
  void* memory;
  uint width;
  uint height;
  uint pitch;
} HHPixelBuffer;

// NOTE(Ryan): This gives us the opportunity to handle increased polling
typedef struct {
  uint num_times_up_down;
  bool ended_down;
} HHBtnState;

typedef struct {
  bool is_connected;
  bool is_analog;

  union {
    HHBtnState buttons[6];
    struct {
      HHBtnState up_btn;
    };
  };
} HHGameController;

typedef struct {
  HHGameController controllers[4];

} HHInput;

void 
hh_render_gradient(
                   HHPixelBuffer* restrict pixel_buffer, 
                   uint green_offset, 
                   uint blue_offset
                  );

#endif

glMatrixMode(GL_MODELVIEW);
// NOTE(Ryan): Identity matrix is a diagonal row of 1's. Effectively a NOP
glLoadIdentity();

glBegin(GL_TRIANGLES);

// NOTE(Ryan): Using fixed function pipeline as opposed to shaders, so implicit vertex transformations occuring that don't put triangles where we want them
// NOTE(Ryan): An affine transformation is a linear transformation with displacement (not just scaling and skewing)
glVertex2i(0, 0);
glVertex2i(window_width, 0);
glVertex2i(window_width, window_height);

glVertex2i(0, 0);
glVertex2i(0, window_height);
glVertex2i(window_width, window_height);
glEnd();








