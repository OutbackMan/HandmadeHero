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

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <GL/gl.h>

#include "hh.h"
#include "hh.c"

GLOBAL bool global_want_to_run;

#define MAX_CONTROLLERS 4
typedef struct {
  int32 joystick_id;
  SDL_GameController* controller;
  SDL_Haptic* haptic;
} SDLGameController;

GLOBAL SDLGameController controllers[MAX_CONTROLLERS];
GLOBAL uint num_controllers_connected = 0;

INTERNAL void
sdl_load_currently_connected_game_controllers(void)
{
  uint num_joysticks = SDL_NumJoysticks(); 
  for (uint joystick_i = 0; joystick_i < num_joysticks; ++joystick_i) {
    if (!SDL_IsGameController(joystick_i)) {
      continue; 
    }
    if (num_controllers_connected >= MAX_CONTROLLERS) {
      break; 
    }

    controllers[num_controllers_connected]->controller = SDL_GameControllerOpen(joystick_i);
    SDL_Joystick* joystick = SDL_GameControllerGetJoystick(controllers[num_controllers_connected]->controller);
    controllers[num_controllers_connected]->haptic = SDL_HapticOpenFromJoystick(joystick);
    if (SDL_HapticRumbleInit(controllers[num_controllers_connected]->haptic) < 0) {
      SDL_HapticClose(controllers[num_controllers_connected]->haptic);
    }
      
    controllers[num_controllers_connected]->joystick_id = joystick_i;

    ++num_controllers_connected;
  }
}


CONTROLLER_ADDED: {
  if (num_controllers_connected < MAX_CONTROLLERS) {
    for (uint controller_i = 0; controller_i < MAX_CONTROLLERS; ++controller_i) {
      SDLGameController* controller = &controllers[controller_i];
      if (controller->is_connected) {
        continue; 
      }
      // setup controller
    }                  
  }
}

CONTROLLER_REMOVED: {
  for (uint controller_i = 0; controller_i < MAX_CONTROLLERS; ++controller_i) {
    SDLGameController* controller = &controllers[controller_i];
    if (controller->joystick_id == cdevice.joystick_id) {
      // remove controller
      --num_controllers_connected;
    }
  }                  
}

int 
main(int argc, char* argv[argc + 1])
{
  if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
    // TODO(Ryan): Implement organised logging when game is nearly complete
    //             and want to ascertain failure points across different end user machines.
    SDL_Log("Unable to initialise SDL: %s", SDL_GetError());
    return EXIT_FAILURE;
  }

  uint window_width = 1280;
  uint window_height = 720;
  SDL_Window* window = SDL_CreateWindow(
                                        "Handmade Hero", 
                                        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                                        window_width, window_height, 
                                        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL
                                       );
  if (window == NULL) {
    SDL_Log("Unable to create SDL window: %s", SDL_GetError());
    return EXIT_FAILURE;
  }

  sdl_load_currently_connected_game_controllers();

  HHPixelBuffer pixel_buffer = {0};
  pixel_buffer.memory = malloc();

  HHInput inputs[2];
  HHInput* cur_input = &inputs[0];
  HHInput* prev_input = &inputs[1];

  SDL_GLContext context = SDL_GL_CreateContext(window);

  global_want_to_run = true;
  while (global_want_to_run) {
    SDL_Event event = {0};
    while (SDL_PollEvent(&event) != 0) {
     if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE) {
        global_want_to_run = false; 
     }
     if (event.type == SDL_QUIT) {
        global_want_to_run = false; 
     }
     if (event.type == SDL_CONTROLLERDEVICEADDED) {

     }
     if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
             
     }
    }

    sw_render(&pixel_buffer, &cur_input);

    display_pixel_buffer_in_window();

    SW_SWAP(cur_input, prev_input)
  }

  return 0;
}

INTERNAL void
display_pixel_buffer_in_window()
{
  glViewport(0, 0, window_width, window_height);
  glClearColor(1.f, 0.f, 1.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  glBegin(GL_TRIANGLES);

  glVertex2i(0, 0);
  glVertex2i(window_width, 0);
  glVertex2i(window_width, window_height);

  glVertex2i(0, 0);
  glVertex2i(0, window_height);
  glVertex2i(window_width, window_height);
  glEnd();

  SDL_GL_SwapWindow(window);
}

INTERNAL SDL_Rect
aspect_ratio_fit(uint render_width, uint render_height, uint window_width, uint window_height)
{
  SDL_Rect result = {0};

  // NOTE(Ryan): Better to only have parameter checking on public calls, but do everywhere when developing
  if ((render_width > 0) &&
      (render_height > 0) &&
      (window_width > 0) &&
      (window_height > 0)) {

    float optimal_window_width = (float)window_height * ((float)render_width / (float)render_height);
    float optimal_window_height = (float)window_width * ((float)render_height / (float)render_width);

    // 32:00
    if (optimal_window_width > (float)window_width) {
      result.x = 0;
      result.w = window_width; 
    } else {
    
    }

  }








