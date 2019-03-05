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

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <GL/gl.h>

#include "hh.h"
#include "hh.c"

GLOBAL bool global_want_to_run;

#define INT32_MIN_VALUE -2147483648  
#define UNUSED_JOYSTICK_ID INT32_MIN_VALUE
#define NUM_CONTROLLERS_SUPPORTED 8
typedef struct {
  // NOTE(Ryan): device_id is for controller connections
  int32 joystick_device_id;
  SDL_JoystickID joystick_instance_id;
  SDL_GameController* controller;
  SDL_Haptic* haptic;
} SDLGameController;


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
  //SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller)) as joystick different on added and button down 
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

typedef struct {
  bool is_connected;
  bool is_analog;
  float stick_x;
  float stick_y;  

  union {
    HHButtonState buttons[12];
    struct {
      HHButtonState move_up; 
      HHButtonState move_down; 
      HHButtonState move_left; 
      HHButtonState move_right; 

      HHButtonState action_up; 
      HHButtonState action_down; 
      HHButtonState action_left; 
      HHButtonState action_right; 

      HHButtonState left_shoulder; 
      HHButtonState right_shoulder; 

      HHButtonState back; 
      HHButtonState start; 

      HHButtonState guide; 
    } 
  }
} HHController;

typedef struct {
  HHController controllers[NUM_GAME_CONTROLLERS_SUPPORTED + 1];
} HHInput;

GLOBAL sound_is_playing = false;

INTERNAL void
sdl_init_audio(u32 samples_per_second, u32 buffer_size)
{
  SDL_AudioSpec audio_spec = {0};
  audio_spec.freq = samples_per_second;
  audio_spec.format = AUDIO_S16LSB;
  audio_spec.channels = 2;
  audio_spec.samples = buffer_size;

  if (SDL_OpenAudio(&audio_spec, NULL) < 0) {
    SDL_Log("Unable to open SDL audio: %s", SDL_GetError());
    return;
  }

  if (audio_spec.format != AUDIO_S16LSB) {
    SDL_Log("Did not recieve desired SDL audio format: %s", SDL_GetError());
    SDL_CloseAudio();
  }
}

int 
main(int argc, char* argv[argc + 1])
{
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC | SDL_INIT_AUDIO) < 0) {
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

  uint samples_per_second = 48000;
  uint bytes_per_sample = sizeof(int16) * 2;
  sdl_init_audio(samples_per_second, samples_per_second * bytes_per_sample / 60);

  // NOTE(Ryan): Hardware sound buffer is 1 second long. We only want to write a small amount of this to avoid latency issues
  uint latency_sample_count = samples_per_second / 15;
  int16* samples = calloc(latency_sample_count, bytes_per_sample);

  HHInput input = {0};

  SDL_GLContext context = SDL_GL_CreateContext(window);

  global_want_to_run = true;
  while (global_want_to_run) {
    SDL_Event event = {0};
    while (SDL_PollEvent(&event) != 0) {
     switch (event.type) {
       case SDL_QUIT: {
        want_to_run = false; 
       } break;
       case SDL_WINDOWEVENT: {
         if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
           want_to_run = false;             
         }
       } break;
       case SDL_KEYDOWN:
       case SDL_KEYUP: {
         SDL_Keycode key_code = event.key.keysm.sym; 
         bool is_key_down = (event.key.state == SDL_PRESSED); 
         bool was_key_down = (event.key.state == SDL_RELEASED);
         
         // NOTE(Ryan): Users will have to check if is held
         if (key_code == SDLK_w) {
           input.controller[0].move_up.is_down = is_key_down; 
           input.controller[0].move_up.was_down = was_key_down;
         }
       } break;
       case SDL_CONTROLLERDEVICEADDED: {
         sdl_open_game_controller(event.cdevice.which);
       } break;
       case SDL_CONTROLLERDEVICEREMOVED: {
         sdl_close_game_controller(event.cdevice.which);
       } break;
       case SDL_CONTROLLERAXISMOTION: {
         for (uint controller_i = 0; controller_i < NUM_GAME_CONTROLLERS_SUPPORTED; ++controller_i) {
           if (controllers[controller_i].joystick_instance_id == event.caxis.which) {
           
           }
       } break;
       case SDL_CONTROLLERBUTTONDOWN:
       case SDL_CONTROLLERBUTTONUP: {
         for (uint controller_i = 0; controller_i < NUM_GAME_CONTROLLERS_SUPPORTED; ++controller_i) {
           if (controllers[controller_i].joystick_instance_id == event.cbutton.which) {
             bool is_button_down = (event.cbutton.state == SDL_PRESSED); 
             bool was_button_down = (event.cbutton.state == SDL_RELEASED);

             if (event.cbutton.button == SDL_CONTROLLER_BUTTON_A) {
             
             }
           } 
         }
       } break;
     }
     
    }

    // SDL_HapticRumblePlay(controllers[controller_i].rumble, 0.5f, 2000);

    uint target_queue_bytes = latency_sample_count * bytes_per_sample; 
    uint bytes_to_write = target_queue_bytes - SDL_GetQueuedAudioSize(1);

    HHSoundBuffer hh_sound_buffer = {0};
    hh_sound_buffer.samples_per_second = samples_per_second;
    hh_sound_buffer.sample_count = bytes_to_write / bytes_per_sample;
    hh_sound_buffer.samples = samples;

    hh_render_and_update(&hh_pixel_buffer, &hh_input, &hh_sound_buffer);
    
    SDL_QueueAudio(1, samples, bytes_to_write);

    if (!sound_is_playing) {
      SDL_PauseAudio(0); 
      sound_is_playing = true;
    }

    opengl_display_pixel_buffer(&pixel_buffer);

    SDL_GL_SwapWindow(window);

    // Usable macros have type safety and scope management with expression statements
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
