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
typedef unsigned long word;

#define RAISE_ERROR_ON_ZERO(input) \
  (0 * sizeof(struct { field: (2 * (input) - 1);}))

#define IS_SAME_TYPE(var1, var2) \
  __builtin_types_compatible_p(typeof(var1), typeof(var1))

#define ARRAY_LENGTH(arr) \
  (sizeof(arr)/sizeof(arr[0])) + RAISE_ERROR_ON_ZERO(IS_SAME_TYPE(arr, &arr[0])

#define BITS_PER_WORD (sizeof(word) * 8)

// NOTE(Ryan): The '-1', '+1' is to account for integer truncation 
#define NUM_WORD_TO_REPR_NUM_BITS(num_bits) \
  ((num_bits - 1)/BITS_PER_WORD + 1)

#define BIT_OFFSET_IN_WORD(bit_num) \
  (bit_num % BITS_PER_WORD)

#define BIT_NUM_TO_WORD_ARRAY_INDEX(bit_num) \
  (bit_num / BITS_PER_WORD)

#define CHECK_BIT_IN_WORD_ARRAY(arr, bit_num) \
  ((arr[BIT_NUM_TO_WORD_ARRAY_INDEX(bit_num)] & (1UL << BIT_OFFSET_IN_WORD(bit_num))

#endif
