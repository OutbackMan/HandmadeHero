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

GLOBAL bool global_want_to_run;

#define INT32_MIN_VALUE -2147483648  
#define UNUSED_JOYSTICK_ID INT32_MIN_VALUE
#define NUM_CONTROLLERS_SUPPORTED 8
typedef struct {
  int32 joystick_id;
  SDL_GameController* controller;
  SDL_Haptic* haptic;
} SDLGameController;

// initialise last as it is not essential
typedef struct {
  
} SDLSoundOutput;

GLOBAL SDLGameController controllers[NUM_CONTROLLERS_SUPPORTED];

INTERNAL void
sdl_find_game_controllers(void)
{
  uint num_joysticks = SDL_NumJoysticks(); 
  uint num_controllers_found = 0;

  for (uint joystick_i = 0; joystick_i < num_joysticks; ++joystick_i) {
    if (!SDL_IsGameController(joystick_i)) {
      continue; 
    }
    if (num_controllers_found >= NUM_CONTROLLERS_SUPPORTED) {
      break; 
    }

    sdl_open_game_controller(joystick_i);

    ++num_controllers_found;
  }
}

INTERNAL void
sdl_close_game_controller(int32 joystick_id) 
{
  for (uint controller_i = 0; controller_i < NUM_CONTROLLERS_SUPPORTED; ++controller_i) {
    if (controllers[controller_i].joystick_id == joystick_id) {
      if (controllers[controller_i].haptic != NULL) {
        SDL_HapticClose(controllers[controller_i].haptic);
        controllers[controller_i].haptic = NULL;
      } 

      SDL_GameControllerClose(controllers[controller_i].controller);
      controllers[controller_i].controller = NULL;

      controllers[controller_i].joystick_id = UNUSED_JOYSTICK_ID;
    } 
  }
}

INTERNAL void
sdl_open_game_controller(int32 joystick_id) 
{
  for (uint controller_i = 0; controller_i < NUM_CONTROLLERS_SUPPORTED; ++controller_i) {
    if (controllers[controller_i].controller != NULL) {
      controllers[controller_i].controller = SDL_GameControllerOpen(joystick_id);
      if (controllers[controller_i].controller == NULL) {
        SDL_Log("Unable to open SDL game controller: %s", SDL_GetError());
        return;
      }
      
      SDL_Joystick* joystick = SDL_GameControllerGetJoystick(controllers[controller_i].controller);

      controllers[controller_i].haptic = SDL_HapticOpenFromJoystick(joystick);
      if (controllers[controller_i].haptic == NULL) {
        SDL_Log("Unable to open SDL haptic from SDL game controller: %s", SDL_GetError());
      } else {
        if (SDL_HapticRumbleInit(controllers[controller_i].haptic) < 0) {
          SDL_Log("Unable to initialise rumble from SDL haptic: %s", SDL_GetError());
          SDL_HapticClose(controllers[controller_i].haptic);
          controllers[controller_i].haptic = NULL;
        }
      }
        
      controllers[controller_i].joystick_id = joystick_id;
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

  sdl_find_game_controllers();

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
       sdl_open_game_controller(event.cdevice.which);
     }
     if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
       sdl_close_game_controller(event.cdevice.which);
     }
    }

    for (uint controller_i = 0; controller_i < NUM_CONTROLLERS_SUPPORTED; ++controller_i) {
      // iterate through controllers here 
    }

    sw_render(&pixel_buffer, &cur_input);

    display_pixel_buffer_in_window();

    // Usable macros have type safety
    SWAP_T(cur_input, prev_input)
  }

  return 0;
}


// place inside hh-opengl.c
void
rendering()
{
  case CLEAR: {
    glClearColor(1.f, 0.f, 1.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT);
  }
  case RECTANGLE: {
    // triangle rendering
  }
}

INTERNAL void
display_pixel_buffer_in_window()
{
  glViewport(0, 0, window_width, window_height);

  // This only needs to be done once. IF AT ALL?? JUST USE ARBITRARY INTS??
  GLunit texture_handle = 0;
  glGenTextures(1, &texture_handle);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  // glTexParameteri();

  // NOTE(Ryan): Little endian writes to memory in opposite order as stored in register
  glBindTexture(GL_TEXTURE_2D, texture_handle);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, buffer->width, buffer->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer->memory);
  glEnable(GL_TEXTURE_2D);
 
  glClearColor(1.f, 0.f, 1.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();

  glBegin(GL_TRIANGLES);

  // NOTE(Ryan): Texture is 0,0 --> 1,1
  glTexCoord2f(.0f, .0f);

  // NOTE(Ryan): Due to clipping to unit cube, put -1, 1
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

    if (optimal_window_width > (float)window_width) {
      result.x = 0;
      result.w = window_width; 

      float empty_vspace = (float)window_height - optimal_window_height;
      // Using intrinsics performs overflow checking for us.
      int32 half_empty_vspace = round_float_to_int(0.5f * empty_vspace);
      int32 rounded_empty_vspace = round_float_to_int(empty_vspace);

      result.y = half_emtpy_vspace;
      result.height = result.y + rounded_empty_vspace;
    } else {
      result.y = 0;
      result.height = window_height; 

      float empty_hspace = (float)window_width - optimal_window_width;
      int32 half_empty_hspace = round_float_to_int(0.5f * empty_hspace);
      int32 rounded_empty_hspace = round_float_to_int(empty_hspace);

      result.x = half_empty_hspace;
      result.width = result.x + rounded_empty_hspace;
    }
  }
  
  return result;
}






