#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <stddef.h>

#include "hh-platform.h"
#include "hh-opengl.c"
#include "hh-common.c"

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
  void (*update_and_render)(HHPixelBuffer*, HHInput*, HHSoundBuffer*, HHMemory*);
  // perhaps also pass in logging functions
  uint last_modification_time;
} SDLHHApi;

INTERNAL void
sdl_load_hh_api(SDLHHApi* hh_api)
{
  hh_api->handle = SDL_LoadObject(sdl_info.abs_object_file_name);
  if (code->handle == NULL) {
    SDL_LogCritical("Unable to load hh api handle: %s", SDL_GetError());
    hh_api->update_and_render = NULL;
    hh_api->last_modification_time = 0;
    return;
  } else {
    hh_api->update_and_render = \
      (void (*)(HHPixelBuffer*, HHInput*, HHSoundBuffer*, HHMemory*))SDL_LoadFunction(hh_code, "hh_update_and_render");
    if (hh_api->update_and_render == NULL) {
      SDL_LogCritical("Unable to load hh api render and update function: %s", SDL_GetError());
      hh_api->last_modification_time = 0;
      return;
    }

    struct stat api_stat = {0};
    stat(sdl_info.abs_object_file_name, &api_stat);
    hh_api->last_modification_time = api_stat.st_mtime;
  }
}

INTERNAL void
sdl_unload_hh_api(SDLHHApi* hh_api)
{
  SDL_UnloadObject(hh_api->handle);
  hh_api->update_and_render = NULL;
}

// TODO(Ryan): Implement memory map for persistent replay buffers.
typedef struct {
  SDL_RWops* input_handle;
  char input_file_name[256];
  void* memory_block; 
} SDLReplayBuffer; 


INTERNAL STATUS
sdl_init_audio(u32 samples_per_second)
{
  SDL_AudioSpec audio_spec = {0};
  audio_spec.freq = samples_per_second;
  audio_spec.format = AUDIO_S16LSB;
  audio_spec.channels = 2;
  audio_spec.samples = sizeof(int16) * 2 * samples_per_second / 60;

  if (SDL_OpenAudio(&audio_spec, NULL) < 0) {
    SDL_LogWarn("Unable to open SDL audio: %s", SDL_GetError());
    return FAILED;
  }

  if (audio_spec.format != AUDIO_S16LSB) {
    SDL_LogWarn("Did not recieve desired SDL audio format: %s", SDL_GetError());
    SDL_CloseAudio();
    return FAILED;
  }

  return SUCCEEDED;
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
  char abs_object_file_name[256];
} sdl_info;

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

  strcpy(sdl_info.abs_object_file_name, sdl_info.base_path);
#if defined(LINUX)
  strcat(sdl_info.abs_object_file_name, "x86_64-desktop-sdl-hh.so");
#elif defined(WINDOWS)
  strcat(sdl_info.abs_object_file_name, "x86_64-desktop-sdl-hh.dll");
#elif defined(MAC)
  strcat(sdl_info.abs_object_file_name, "x86_64-desktop-sdl-hh.dylib");
#endif
}

void
sdl_debug_log_function(void* user_data, int category, SDL_LogPriority priority, char const* msg)
{
  puts(msg); fflush(stdout);
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

  uint window_width = 1920;
  uint window_height = 1080;
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

#if defined(DEBUG)
  SDL_ShowCursor(SDL_ENABLE);
#else
  SDL_ShowCursor(SDL_DISABLE);
#endif

  bool need_to_compute_refresh_rate = true;
  uint refresh_rate = 0;

  if (need_to_compute_refresh_rate) {
    SDL_DisplayMode display_mode = {0};
    int display_index = SDL_GetWindowDisplayIndex(window);
    SDL_GetCurrentDisplayMode(display_index, &display_mode);
    // NOTE(Ryan): This handles the case of variable refresh rate.
    refresh_rate = (display_mode.refresh_rate == 0) ? 60 : display_mode.refresh_rate;
  }

  SDL_GLContext context = SDL_GL_CreateContext(window);
  if (SDL_GL_SetSwapInterval(-1) < 0) {
    SDL_LogDebug("Unable to enable adaptive vsync for sdl opengl context: %s", SDL_GetError());
    if (SDL_GL_SetSwapInterval(1) < 0) {
      SDL_LogCritical("Unable to enable vsync for sdl opengl context: %s", SDL_GetError());
      // TODO(Ryan): Investigate if this occurs frequently enough to merit a fallback.
      return EXIT_FAILURE;
    }
  }

  HHPixelBuffer pixel_buffer = {0};
  pixel_buffer.width = window_width;
  pixel_buffer.height = window_height;
  pixel_buffer.pitch = window_width * BYTES_PER_PIXEL;
  pixel_buffer.memory = calloc(pixel_buffer.pitch * pixel_buffer.height, 1);
  if (pixel_buffer.memory == NULL) {
    SDL_LogCritical("Unable to allocate memory for hh pixel buffer: %s", strerror(errno));
    return EXIT_FAILURE;
  }

  uint samples_per_second = 48000;
  STATUS have_sound = sdl_init_audio(samples_per_second);
  HHSoundBuffer sound_buffer = {0};

  if (have_sound) {
    // NOTE(Ryan): At 60fps, this will be 4 frames worth of audio data.
    uint latency_fraction = 15;
    uint num_game_samples = samples_per_second / latency_fraction; 

    sound_buffer.samples_per_second = samples_per_second;
    sound_buffer.samples = calloc(num_game_samples, sizeof(int16) * 2);
    sound_buffer.sample_count = num_game_samples;
  }

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
  memory.platform_debug_read_entire_file = platform_debug_read_entire_file;
  memory.platform_debug_write_entire_file = platform_debug_write_entire_file;

  SDLHHApi hh_api = {0};
  sdl_load_hh_api(&hh_api);

#if defined(DEBUG)
  // IMPORTANT(Ryan): As each replay buffer allocates approx. 1Gb of memory on startup, limit amount.
  //                  However, unlikely to exceed virtual memory size.
  #define NUM_REPLAY_BUFFERS 1
  SDL_assert(NUM_REPLAY_BUFFERS <= 9);
  SDLReplayBuffer replay_buffers[NUM_REPLAY_BUFFERS] = {0};
  bool are_recording_state = false;
  bool are_looping_state = false;
  uint replay_buffer_i = -1;

  // Need to check if file exists for memory map
  for (uint replay_i = 0; replay_i < NUM_REPLAY_BUFFERS; ++replay_i) {
    SDLReplayBuffer* replay_buffer = &replay_buffers[replay_i];
    sprintf(replay_buffer->input_file_name, "%sloop-edit-%u-input.hmi", sdl_info.base_path, replay_i);
    replay_buffer->memory = malloc(hh_memory.total_memory_size); 
  }
#endif

  sdl_populate_info();

  sdl_find_game_controllers();

  // TODO(Ryan): Add support for multiple keyboards.
  HHInput input = {0};
  input.controllers[0].is_connected = true;

  u8 const* keyboard_state = SDL_GetKeyboardState(NULL);

  // TODO(Ryan): Timing

  want_to_run = true;
  while (want_to_run) {
    if (need_to_compute_refresh_rate) {
    
      input.frame_dt = 1 / refresh_rate;
    }
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
         if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
           window_width = event.window.data1; 
           window_height = event.window.data2; 
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
      // SDL_HapticRumblePlay(controllers[controller_i].haptic, 0.5f, 2000);
    }

    if (keyboard_state[SDL_SCANCODE_LALT] && keyboard_state[SDL_SCANCODE_RETURN]) {
      u32 window_flags = SDL_GetWindowFlags(window);
      if (window_flags & SDL_WINDOW_FULLSCREEN) {
        SDL_SetWindowFullscreen(window, 0); 
      } else {
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN); 
      }
      need_to_compute_refresh_rate = true;
    }

#if defined(DEBUG)
    for (uint sdl_scancode = SDL_SCANCODE_1; sdl_scancode < SDL_SCANCODE_1 + NUM_REPLAY_BUFFERS; ++sdl_scancode) {
      if (keyboard_state[sdl_scancode]) replay_buffer_i = sdl_scancode - SDL_SCANCODE_1;
    }
    bool have_selected_replay_buffer = (replay_buffer_i != -1);

    // INFO(Ryan):
    // **** RECORDING ****
    // * Start recording by pressing 'r' and replay buffer index to save to: [1, NUM_REPLAY_BUFFERS], e.g. r1.
    // * Finish recording by pressing 'r' again.
    // **** LOOPING ****
    // * Start looping by pressing 'l' and replay buffer index to read recording info from, e.g. l1.
    // * Finish looping by pressing 'l' again.
    
    if (keyboard_state[SDL_SCANCODE_R] && !are_looping_state) {
      if (!are_recording_state) {
        if (have_selected_replay_buffer) {
          // start recording
          are_recording_state = true;
          SDLReplayBuffer* replay_buffer =  &replay_buffers[replay_buffer_i];
          memcpy(replay_buffer->memory_block, hh_memory.complete, hh_memory.total_size);
          SDL_RWopen();
        }
      } else {
        // stop recording 
        are_recording_state = false;
        replay_buffer_i = -1;
        SDL_RWclose(replay_buffer->input_file_name);
      }
    }

    if (keyboard_state[SDL_SCANCODE_L] && !are_recording_state) {
      if (!are_looping_state) {
        if (have_selected_replay_buffer) {
          // start looping 
          are_looping_state = true;
          memcpy(hh_memory.complete, replay_buffer->memory_block, hh_memory.total_size);
          replay_buffer->input_file_handle = SDL_RWOpen();
        }
      } else {
        // stop looping replay_buffer_i 
        SDL_RWclose();
        are_looping_state = false;
        replay_buffer_i = -1;
      }
    }

    if (are_recording_state) {
      uint bytes_written = 0;
      if (SDL_RWwrite())
      write_file(input); 
    }
    if (are_looping_state) {
      read_file(input); 
    }

      if (sdl_state->input_playing_index == 0) {
        if (sdl_state->input_recording_index == 0) {
          // sdl_begin_recording_input(sdl_state, 1);
          SDLReplayBuffer* replay_buffer =  &sdl_state.replay_buffers[1];
          if (replay_buffer->memory_block != NULL) {
            sdl_state->input_recording_index = 1; 
            char* file_name = "c:asdasd/asdas/dasd/loop_edit<slot><input>.hmi";
            sdl_state->recording_handle = file_name;
            memcpy(replay_buffer->memory_block, sdl_state->game_memory_block, sdl_state->total_size);  
          }
        } else {
          // sdl_end_recording_input(state);
          close_file(sdl_state->recording_handle);
          sdl_state->input_recording_index = 0;
          // sdl_begin_input_playback(state, 1);
          SDLReplayBuffer* replay_buffer =  &sdl_state.replay_buffers[1];
          if (replay_buffer->memory_block != NULL) {
            sdl_state->input_playing_index = 1; 
            char* file_name = "c:asdasd/asdas/dasd/loop_edit<slot><input>.hmi";
            sdl_state->playback_handle = file_name;
            memcpy(sdl_state->game_memory_block, replay_buffer->memory_block, sdl_state->total_size);  
          }
        }
      } else {
        // sdl_end_input_playback(state); 
        close_file(sdl_state->playback_handle);
        sdl_state->input_playing_index = 0;
      }
    } 
#endif

    if (sdl_state.input_recording_index) {
      // sdl_record_input(&sdl_state, input); 
      write_file(sdl_state->recording_handle, input, sizeof(input));
    }

    if (sdl_state.input_playing_index) {
      // sdl_playback_input(&sdl_state, input); 
      read_file(sdl_state->playback_handle, input, sizeof(input));
      // reached end of stream
      if (bytes_read == 0) {
        sdl_end_input_playback(sdl_state);
        sdl_begin_input_playback(sdl_state, sdl_state->input_playing_index;
        read_file(sdl_state->playback_handle, input, sizeof(input));
      }
    }

    if (hh_api->update_and_render != NULL) {
      hh_api->update_and_render(&hh_pixel_buffer, &hh_input, &hh_sound);
    }
    

    SDL_Rect drawable_region = aspect_ratio_fit(pixel_buffer->width, pixel_buffer->height, window_width, window_height);
    opengl_display_pixel_buffer(&pixel_buffer, &drawable_region);
    SDL_GL_SwapWindow(window);

    uint target_queue_bytes = sound_buffer.sample_count * sizeof(int16) * 2;
    // NOTE(Ryan): As some frames will run longer than expected, have extra padding bytes to ensure no silence.
    // However, we need to prevent a build up of queued audio data.
    uint bytes_to_write = 0;
    if (SDL_GetQueuedAudioSize(1) < target_queue_bytes) {
      bytes_to_write = target_queue_bytes - SDL_GetQueuedAudioSize(1); 
    }

    if (have_sound) {
      SDL_QueueAudio(1, sound_buffer.samples, bytes_to_write);

      if (SDL_GetAudioStatus() != SDL_AUDIO_PLAYING) {
        SDL_PauseAudio(0); 
      }
    }

    for (uint controller_i = 0; controller_i < NUM_GAME_CONTROLLERS_SUPPORTED + 1; ++controller_i) {
      input.controllers[controller_i].stick_x = 0.0f;
      input.controllers[controller_i].stick_y = 0.0f;
    } 

    struct stat hh_api_stat = {0};
    stat(sdl_info.abs_object_file_name, &hh_api_stat);
    if (hh_api_stat.st_mtime > hh_api->last_modification_time) {
      sdl_unload_hh_api(&hh_api); 
      sdl_reload_hh_api(&hh_api); 
      hh_api->last_modification_time = hh_api_stat.st_mtime;
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

    // CHECK THIS RETURN VALUE
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
