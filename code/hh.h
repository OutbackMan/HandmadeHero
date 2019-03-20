#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include "hh.h"
#include "hh.c"
#include "hh-opengl.c"

#define INT32_MIN_VALUE -2147483648
#define UNUSED_SDL_JOYSTICK_CONNECTION_ID INT32_MIN_VALUE
#define SDL_GAME_CONTROLLER_STICK_DEADZONE 7849
typedef struct {
  SDL_JoystickID instance_joystick_id;
  SDL_GameController* controller;
  SDL_Haptic* haptic;
} SDLGameController;

GLOBAL SDLGameController sdl_game_controllers[NUM_GAME_CONTROLLERS_SUPPORTED] = {0};

INTERNAL void
sdl_find_exisiting_game_controllers(void)
{
  int num_joysticks = SDL_NumJoysticks(); 

  for (int joystick_i = 0; joystick_i < num_joysticks; ++joystick_i) {
    if (num_sdl_game_controllers_connected == NUM_GAME_CONTROLLERS_SUPPORTED) {
      break; 
    }
    if (!SDL_IsGameController(joystick_i)) {
      continue; 
    }

    sdl_open_game_controller(joystick_i);
  }
}

INTERNAL void
sdl_open_game_controller(int device_joystick_id) 
{
  for (uint controller_i = 0; controller_i < NUM_GAME_CONTROLLERS_SUPPORTED; ++controller_i) {
    if (sdl_game_controllers[controller_i].controller != NULL) {
      sdl_game_controllers[controller_i].controller = SDL_GameControllerOpen(device_joystick_id);
      if (sdl_game_controllers[controller_i].controller == NULL) {
        SDL_Log("Unable to open SDL game controller: %s", SDL_GetError());
        return;
      }
      
      SDL_Joystick* joystick = SDL_GameControllerGetJoystick(sdl_game_controllers[controller_i].controller);

      sdl_game_controllers[controller_i].instance_joystick_id = SDL_JoystickInstanceID(joystick);

      sdl_game_controllers[controller_i].haptic = SDL_HapticOpenFromJoystick(joystick);
      if (sdl_game_controllers[controller_i].haptic == NULL) {
        SDL_Log("Unable to open SDL haptic from SDL game controller: %s", SDL_GetError());
      } else {
        if (SDL_HapticRumbleInit(sdl_game_controllers[controller_i].haptic) < 0) {
          SDL_Log("Unable to initialize rumble from SDL haptic: %s", SDL_GetError());
          SDL_HapticClose(sdl_game_controllers[controller_i].haptic);
          sdl_game_controllers[controller_i].haptic = NULL;
        }
      }
    } 
  }
}

INTERNAL float
sdl_normalize_stick(int16 stick_value)
{
  if (stick_value < -SDL_GAME_CONTROLLER_DEADZONE) {
    return (float)(stick_value + SDL_GAME_CONTROLLER_DEADZONE) / (32768.0f - SDL_GAME_CONTROLLER_DEADZONE);
  } else if (stick_value > SDL_GAME_CONTROLLER_DEADZONE) {
    return (float)(stick_value - SDL_GAME_CONTROLLER_DEADZONE) / (32767.0f - SDL_GAME_CONTROLLER_DEADZONE);
  } else {
    return 0.0f;
  }
}

INTERNAL void
sdl_close_game_controller(SDL_JoystickID instance_joystick_id) 
{
  for (uint controller_i = 0; controller_i < NUM_GAME_CONTROLLERS_SUPPORTED; ++controller_i) {
    if (sdl_game_controllers[controller_i].instance_joystick_id == instance_joystick_id) {
      if (sdl_game_controllers[controller_i].haptic != NULL) {
        SDL_HapticClose(sdl_game_controllers[controller_i].haptic);
        sdl_game_controllers[controller_i].haptic = NULL;
      } 

      SDL_GameControllerClose(sdl_game_controllers[controller_i].controller);
      sdl_game_controllers[controller_i].controller = NULL;

      sdl_game_controllers[controller_i].instance_joystick_id = UNUSED_JOYSTICK_ID;
    } 
  }
}


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

GLOBAL bool want_to_run = false;
GLOBAL bool sound_is_playing = false;

int 
main(int argc, char* argv[argc + 1])
{
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC | SDL_INIT_AUDIO) < 0) {
    // TODO(Ryan): Implement organised logging when game is nearly complete
    // and want to ascertain failure points across different end user machines.
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
    SDL_Log("Unable to create sdl window: %s", SDL_GetError());
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

  SDL_GLContext context = SDL_GL_CreateContext(window);
  if (SDL_GL_SetSwapInterval(-1) < 0) {
    SDL_Log("Unable to enable adaptive vsync for sdl opengl context: %s", SDL_GetError());
    if (SDL_GL_SetSwapInterval(1) < 0) {
      SDL_Log("Unable to enable vsync for sdl opengl context: %s", SDL_GetError());
      // TODO(Ryan): Investigate if this occurs frequently enough to merit a fallback.
      return EXIT_FAILURE;
    }
  }
  
  HHInput input = {0};
  // TODO(Ryan): Add support for multiple keyboards.
  input.controllers[0].is_connected = true;

  u8 const* keyboard_state = SDL_GetKeyboardState(NULL);

  want_to_run = true;
  while (want_to_run) {
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
       case SDL_CONTROLLERDEVICEADDED: {
         sdl_open_game_controller(event.cdevice.which);
       } break;
       case SDL_CONTROLLERDEVICEREMOVED: {
         sdl_close_game_controller(event.cdevice.which);
       } break;
     }

    input.controllers[0].move_up = keyboard_state[SDL_SCANCODE_W];
    input.controllers[0].move_left = keyboard_state[SDL_SCANCODE_A];
    input.controllers[0].move_down = keyboard_state[SDL_SCANCODE_S];
    input.controllers[0].move_right = keyboard_state[SDL_SCANCODE_D];

    input.controllers[0].action_up = keyboard_state[SDL_SCANCODE_UP];
    input.controllers[0].action_left = keyboard_state[SDL_SCANCODE_LEFT];
    input.controllers[0].action_down = keyboard_state[SDL_SCANCODE_DOWN];
    input.controllers[0].action_right = keyboard_state[SDL_SCANCODE_RIGHT];

    input.controllers[0].special_left = keyboard_state[SDL_SCANCODE_Q];
    input.controllers[0].special_right = keyboard_state[SDL_SCANCODE_E];

    input.controllers[0].start = keyboard_state[SDL_SCANCODE_RETURN];
    input.controllers[0].back = keyboard_state[SDL_SCANCODE_ESCAPE];

         
    // NOTE(Ryan): Keyboard controller reserves 0th controller position.
    for (uint game_controller_i = 1; game_controller_i < NUM_GAME_CONTROLLERS_SUPPORTED + 1; ++game_controller_i) {
      SDL_GameController* sdl_game_controller = &sdl_game_controllers[game_controller_i - 1].controller;
      if (sdl_game_controller != NULL) {
        input.controllers[game_controller_i].is_connected = true; 

        int16 normalized_stick_x = \ 
          sdl_normalize_stick(SDL_GameControllerGetAxis(sdl_game_controller, SDL_CONTROLLER_AXIS_LEFTX)); 
        int16 normalized_stick_y = \
          sdl_normalize_stick(SDL_GameControllerGetAxis(sdl_game_controller, SDL_CONTROLLER_AXIS_LEFTY)); 
        if (normalized_stick_x != 0.0f || normalized_stick_y != 0.0f) {
          input.controllers[game_controller_i].is_analog = true; 
        } 

        if (SDL_GameControllerGetButton(sdl_game_controller, SDL_CONTROLLER_BUTTON_DPAD_UP)) {
          normalized_stick_y = 1.0f; 
          input.controllers[game_controller_i].is_analog = false; 
        }
        if (SDL_GameControllerGetButton(sdl_game_controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT)) {
          normalized_stick_x = -1.0f; 
          input.controllers[game_controller_i].is_analog = false; 
        }
        if (SDL_GameControllerGetButton(sdl_controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN)) {
          normalized_stick_y = -1.0f; 
          input.controllers[game_controller_i].is_analog = false; 
        }
        if (SDL_GameControllerGetButton(sdl_game_controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) {
          normalized_stick_x = 1.0f; 
          input.controllers[game_controller_i].is_analog = false; 
        }

        float threshold = 0.5f; 
        input.controllers[game_controller_i].move_up = (normalized_stick_y > threshold); 
        input.controllers[game_controller_i].move_left = (normalized_stick_x < -threshold); 
        input.controllers[game_controller_i].move_down = (normalized_stick_y < -threshold); 
        input.controllers[game_controller_i].move_right = (normalized_stick_x > threshold); 

        input.controllers[game_controller_i].stick_x = normalized_stick_x; 
        input.controllers[game_controller_i].stick_y = normalized_stick_y; 

        input.controllers[game_controller_i].action_up = \ 
          SDL_GameControllerGetButton(sdl_game_controller, SDL_CONTROLLER_BUTTON_Y);
        input.controllers[game_controller_i].action_left = \ 
          SDL_GameControllerGetButton(sdl_game_controller, SDL_CONTROLLER_BUTTON_X);
        input.controllers[game_controller_i].action_down = \ 
          SDL_GameControllerGetButton(sdl_game_controller, SDL_CONTROLLER_BUTTON_A);
        input.controllers[game_controller_i].action_right = \ 
          SDL_GameControllerGetButton(sdl_game_controller, SDL_CONTROLLER_BUTTON_B);

        input.controllers[game_controller_i].special_left = \ 
          SDL_GameControllerGetButton(sdl_game_controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
        input.controllers[game_controller_i].special_right = \ 
          SDL_GameControllerGetButton(sdl_game_controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);

        input.controllers[game_controller_i].start = \ 
          SDL_GameControllerGetButton(sdl_game_controller, SDL_CONTROLLER_BUTTON_START);
        input.controllers[game_controller_i].back = \ 
          SDL_GameControllerGetButton(sdl_game_controller, SDL_CONTROLLER_BUTTON_BACK);
      } else {
        input.controllers[game_controller_i].is_connected = false; 
        input.controllers[game_controller_i].is_analog = false; 
      }
    }

    // SDL_HapticRumblePlay(controllers[controller_i].haptic, 0.5f, 2000);

    hh_render_and_update(&hh_pixel_buffer, &hh_input, &hh_sound_buffer);
    
    SDL_QueueAudio(1, samples, bytes_to_write);

    if (!sound_is_playing) {
      SDL_PauseAudio(0); 
      sound_is_playing = true;
    }

    opengl_display_pixel_buffer(&pixel_buffer);

    SDL_GL_SwapWindow(window);

    for (uint controller_i = 0; controller_i < NUM_GAME_CONTROLLERS_SUPPORTED + 1; ++controller_i) {
      input.controllers[controller_i].stick_x = 0.0f;
      input.controllers[controller_i].stick_y = 0.0f;
    } 
  }

  return 0;
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

// compiler can only perform static analysis, we can do dynamic. this means forceinline can be better based on metrics
// on debug functions, consider using asserts to indicate unlikely scenarios
void* debug_read_entire_file(char const* file_name)
{

} 
