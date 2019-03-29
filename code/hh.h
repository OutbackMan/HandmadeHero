#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include "hh.h"
#include "hh.c"
#include "hh-opengl.c"

#define INT32_MIN_VALUE -2147483648
#define UNUSED_SDL_INSTANCE_JOYSTICK_ID INT32_MIN_VALUE
#define SDL_GAME_CONTROLLER_STICK_DEADZONE 7849
typedef struct {
  SDL_JoystickID instance_joystick_id;
  SDL_GameController* controller;
  SDL_Haptic* haptic;
} SDLGameController;

GLOBAL SDLGameController sdl_game_controllers[NUM_GAME_CONTROLLERS_SUPPORTED] = {0};

INTERNAL void
sdl_find_game_controllers(void)
{
  int num_joysticks = SDL_NumJoysticks(); 

  for (int joystick_i = 0; joystick_i < num_joysticks; ++joystick_i) {
    if (sdl_game_controllers[NUM_GAME_CONTROLLERS_SUPPORTED - 1].controller != NULL) {
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
        SDL_LogWarn("Unable to open SDL game controller: %s", SDL_GetError());
        return;
      }
      
      SDL_Joystick* joystick = SDL_GameControllerGetJoystick(sdl_game_controllers[controller_i].controller);

      sdl_game_controllers[controller_i].instance_joystick_id = SDL_JoystickInstanceID(joystick);

      sdl_game_controllers[controller_i].haptic = SDL_HapticOpenFromJoystick(joystick);
      if (sdl_game_controllers[controller_i].haptic == NULL) {
        SDL_LogWarn("Unable to open SDL haptic from SDL game controller: %s", SDL_GetError());
      } else {
        if (SDL_HapticRumbleInit(sdl_game_controllers[controller_i].haptic) < 0) {
          SDL_LogWarn("Unable to initialize rumble from SDL haptic: %s", SDL_GetError());
          SDL_HapticClose(sdl_game_controllers[controller_i].haptic);
          sdl_game_controllers[controller_i].haptic = NULL;
        }
      }
    } 
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

      sdl_game_controllers[controller_i].instance_joystick_id = UNUSED_SDL_INSTANCE_JOYSTICK_ID;
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

typedef struct {
  void* handle;
  void (*render_and_update)(HHPixelBuffer*, HHInput*, HHSoundBuffer*, HHMemory*);
  uint last_modification_time;
} SDLHHApi;

INTERNAL void
sdl_load_hh_api(SDLHHApi* hh_api)
{
  code->handle = SDL_LoadObject(sdl_info.object_file_name);
  if (code->handle == NULL) {
    SDL_Log("Unable to load hh code: %s", SDL_GetError());
  } else {
    code->random = (void (*)(void))SDL_LoadFunction(hh_code, "random");
    if (code->random == NULL) {
      printf("Unable to load hh code function: %s", SDL_GetError());
      fflush(stdout);
    }

    struct stat code_stat = {0};
    stat("random.dll", &code_stat);
    code->mod_time = code_stat.st_mtime;
  }
}

static void
sdl_unload_hh_code(HHCode* code)
{
  SDL_UnloadObject(code->random);
  code->handle = NULL;
}

typedef struct {
  SDL_RWops* file_handle;
  SDL_RWops* file_memory_map;
  char file_name[256];
  void* memory_block;
} SDLReplayBuffer; 

typedef struct {
  u64 game_memory_size;
  void* game_memory_block;
  SDLReplayBuffer replay_buffers[4];

  FILE* recording_handle;
  int input_recording_index;

  FILE* playback_handle;
  int input_playing_index;

  char exe_file_name[256];
  char* one_past_last_exe_file_name_slash;
} SDLState;


INTERNAL STATUS
sdl_init_audio(u32 samples_per_second)
{
  SDL_AudioSpec audio_spec = {0};
  audio_spec.freq = samples_per_second;
  audio_spec.format = AUDIO_S16LSB;
  audio_spec.channels = 2;
  audio_spec.samples = sizeof(int16) * 2 * samples_per_second / 60;

  if (SDL_OpenAudio(&audio_spec, NULL) < 0) {
    SDL_Log("Unable to open SDL audio: %s", SDL_GetError());
    return;
  }

  if (audio_spec.format != AUDIO_S16LSB) {
    SDL_Log("Did not recieve desired SDL audio format: %s", SDL_GetError());
    SDL_CloseAudio();
  }
}

PERSIST struct {
  char const* os;
  char const* video_driver;
  char const* audio_driver;
  SDL_Version version;
  int num_logical_cores;
  int ram_mb;
  int l1_cache_line_size;
  // TODO(Ryan): Include power management info.
  char* base_path;
  int gl_major_version;
  int gl_minor_version;
  char object_file_name[256];
} sdl_info;

void
sdl_debug_log_function(void* user_data, int category, SDL_LogPriority priority, char const* msg)
{
  puts(msg); fflush(stdout);
}

INTERNAL void
sdl_get_info(void)
{
  sdl_info.os = SDL_GetPlatform();
  sdl_info.video_driver = SDL_GetCurrentVideoDriver();
  sdl_info.audio_driver = SDL_GetCurrentAudioDriver();
  SDL_GetVersion(&sdl_info.version);
  sdl_info.num_logical_cores = SDL_GetCPUCount();
  sdl_info.ram_mb = SDL_GetSystemRAM();
  sdl_info.l1_cache_line_size = SDL_GetCPUCacheLineSize();
  sdl_info.base_path = SDL_GetBasePath();
  if (SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &sdl_info.gl_major_version) < 0) {
    SDL_LogWarn("Unable to obtain opengl major version: %s", SDL_GetError());
  };
  if (SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &sdl_info.gl_minor_version) < 0) {
    SDL_LogWarn("Unable to obtain opengl minor version: %s", SDL_GetError());
  };
#if defined(LINUX)
  strcpy(sdl_info.object_file_name, "x86_64-desktop-sdl-hh.so");
#elif defined(WINDOWS)
  strcpy(sdl_info.object_file_name, "x86_64-desktop-sdl-hh.dll");
#elif defined(MAC)
  strcpy(sdl_info.object_file_name, "x86_64-desktop-sdl-hh.dylib");
#endif
}

GLOBAL bool want_to_run = false;

int 
main(int argc, char* argv[argc + 1])
{
#if defined(DEBUG)
  SDL_LogSetAllPriority(SDL_LOG_PRIORITY_DEBUG); 
  // NOTE(Ryan): Inside of Eclipse CDT debugger, stdout requires flushing to display in console.
  SDL_LogSetOutputFunction(sdl_debug_log_function, NULL);
#else
  SDL_LogSetAllPriority(SDL_LOG_PRIORITY_ERROR); 
#endif

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC | SDL_INIT_AUDIO) < 0) {
    SDL_LogCritical("Unable to initialize SDL: %s", SDL_GetError());
    return EXIT_FAILURE;
  }

  uint window_width = 1280;
  uint window_height = 720;
  u32 window_flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL;
#if defined(DEBUG)
  if (strcmp(info.video_driver, "x11")) {
    window_flags |= SDL_WINDOW_ALWAYS_ON_TOP;
  }
#endif
  SDL_Window* window = SDL_CreateWindow(
                                        "Handmade Hero", 
                                        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                                        window_width, window_height, 
                                        window_flags 
                                       );
  if (window == NULL) {
    SDL_LogCritical("Unable to create sdl window: %s", SDL_GetError());
    return EXIT_FAILURE;
  }

  SDL_GLContext context = SDL_GL_CreateContext(window);
  if (SDL_GL_SetSwapInterval(1) < 0) {
    SDL_LogCritical("Unable to enable vsync for sdl opengl context: %s", SDL_GetError());
    // TODO(Ryan): Investigate if this occurs frequently enough to merit a fallback.
    return EXIT_FAILURE;
  }

  sdl_init_audio(48000);

  SDLInfo info = {0};
  sdl_get_info(&info);

  HHMemory memory = {0};
  memory.permanent_storage_size = MEGABYTES(64);
  memory.transient_storage_size = GIGABYTES(1);
  uint total_memory_size = memory.permanent_storage_size + memory.transient_storage_size; 
  memory.permanent_storage = malloc(total_memory_size);
  if (memory.permanent_storage == NULL) {
    SDL_LogCritical("Unable to allocate hh memory: %s", strerror(ernno));
    return EXIT_FAILURE;
  }
  memory.transient_storage = ((u8 *)memory.permanent_storage + memory.permanent_storage_size);

  sdl_find_game_controllers();

  HHPixelBuffer pixel_buffer = {0};
  pixel_buffer.memory = malloc();


  // NOTE(Ryan): Hardware sound buffer is 1 second long. We only want to write a small amount of this to avoid latency issues
  uint latency_sample_count = samples_per_second / 15;
  int16* samples = calloc(latency_sample_count, bytes_per_sample);


  
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
#if defined(DEBUG) 
         if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
           if (strcmp(info.video_driver, "x11")) {
             SDL_SetWindowOpacity(window, 0.5f);
           }
         }
         if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
           if (strcmp(info.video_driver, "x11")) {
             SDL_SetWindowOpacity(window, 1.0f);
           }
         }
#endif
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

#if defined(DEBUG)
    if (keyboard_state[SDL_SCANCODE_L]) {
    
    } 
#endif

    u32 mouse_state = SDL_GetMouseState(&input.mouse_x, &input.mouse_y);
    input.left_mouse_button = mouse_state & SDL_BUTTON(SDL_BUTTON_LEFT);
    input.middle_mouse_button = mouse_state & SDL_BUTTON(SDL_BUTTON_MIDDLE);
    input.right_mouse_button = mouse_state & SDL_BUTTON(SDL_BUTTON_RIGHT);
         
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

    // SDL_GetAudioStatus() == SDL_AUDIO_PLAYING
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

void
platform_debug_read_entire_file(char const* file_name, HHDebugPlatformReadFileResult* read_file_result)
{
  SDL_RWops* handle = SDL_RWFromFile(file_name, "rb"); 
  if (handle != NULL) {
    int64 file_size = SDL_RWSeek(handle, 0, RW_SEEK_END);
    if (file_size == -1) {
      SDL_LogWarn("Unable to obtain file size of '%s': %s", file_name, SDL_GetError());
      goto __RETURN_FREE_FILE__;
    }
    if (SDL_RWSeek(handle, 0, RW_SEEK_SET) == -1) {
      SDL_LogWarn("Unable to seek to start of file '%s': %s", file_name, SDL_GetError());
      goto __RETURN_FREE_FILE__;
    }

    void* data = malloc(file_size);
    if (data == NULL) {
      SDL_LogWarn("Unable to allocate file '%s' memory: %s", file_name, strerror(errno));
      goto __RETURN_FREE_FILE__;
    }

    if (SDL_RWread(handle, data, file_size, 1) > 0) {
      read_file_result->size = file_size;
      read_file_result->data = data;
      goto __RETURN_FREE_FILE__;
    } else {
      SDL_LogWarn("File '%s' empty or at EOF: %s", file_name, SDL_GetError());
      goto __RETURN_FREE_FILE__;
    }
  } else {
    SDL_LogWarn("Unable to open file '%s': %s", file_name, SDL_GetError());
    return;
  }
__RETURN_FREE_FILE__:
  SDL_RWclose(handle);
  return;
} 

void 
platform_debug_write_entire_file(char const* file_name, void* memory, uint memory_size)
{
  SDL_RWops* handle = SDL_RWFromFile(file_name, "rb"); 
  if (handle != NULL) {
    if (SDL_RWwrite(handle, memory, memory_size, 1) != memory_size) {
      SDL_LogWarn("Unable to write %u bytes to file '%s': %s", memory_size, file_name, SDL_GetError());
    }
    SDL_RWclose(handle);
    return;
  } else {
    SDL_LogWarn("Unable to open file '%s': %s", file_name, SDL_GetError());
  }
} 


// internal tile map, rendering disguises this, procedural generation
