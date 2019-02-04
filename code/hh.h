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

#define RAISE_ERROR_ON_ZERO(input) \
  (0 * sizeof(struct { field: (2 * (input) - 1);}))

#define IS_SAME_TYPE(var1, var2) \
  __builtin_types_compatible_p(typeof(var1), typeof(var1))

#define ARRAY_LENGTH(arr) \
  (sizeof(arr)/sizeof(arr[0])) + RAISE_ERROR_ON_ZERO(IS_SAME_TYPE(arr, &arr[0]))

#define IS_BIT_SET(bitmask, bit) \
  (bitmask & (1 << bit))

typedef struct {
  // These pixels are arranged BB GG RR AA
  void* memory;
  uint width;
  uint height;
  uint pitch;
} HHPixelBuffer;

void 
hh_render_gradient(HHPixelBuffer* restrict pixel_buffer, uint green_offset, uint blue_offset);

#endif
