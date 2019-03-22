#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

typedef unsigned int uint;
typedef uint8_t u8;
typedef uint32_t u32;
typedef int16_t int16;

typedef struct {
  uint samples_per_second;
  uint sample_count;
  int16* samples;
} HHSoundBuffer;

static bool want_to_run = false;

static bool
sdl_init_audio(u32 samples_per_second)
{
  SDL_AudioSpec audio_spec = {0};
  audio_spec.freq = samples_per_second;
  audio_spec.format = AUDIO_S16LSB;
  audio_spec.channels = 2;
  // NOTE(Ryan): This is a chunk of audio data. Rough estimate based on hopeful 60fps.
  audio_spec.samples = sizeof(int16) * 2 * samples_per_second / 60;

  if (SDL_OpenAudio(&audio_spec, NULL) < 0) {
    SDL_Log("Unable to open SDL audio: %s", SDL_GetError());
    return false;
  }

  if (audio_spec.format != AUDIO_S16LSB) {
    SDL_Log("Did not recieve desired SDL audio format: %s", SDL_GetError());
    SDL_CloseAudio();
    return false;
  }

  return true;
}

static void
prepare_sound(HHSoundBuffer* sound_buffer)
{
  uint amplitude = 3000;
  uint frequency = 512;
  uint period = sound_buffer->samples_per_second / frequency;
  static float tsinf = 0.0f;

  int16* sample = sound_buffer->samples;
  for (uint sample_i = 0; sample_i < sound_buffer->sample_count; ++sample_i) {
    float sine_value = sinf(tsinf);
    int16 sample_value = (int16)(sine_value * amplitude);
    *sample++ = sample_value; 
    *sample++ = sample_value; 

    tsinf += 2.0f * M_PI * 1.0f / (float)period; 
    if (tsinf > 2.0f * M_PI) {
      tsinf -= 2.0f * M_PI; 
    }
  }
}

static inline void
nop(void)
{
  return;
}

int 
main(int argc, char* argv[argc + 1])
{
  nop();
  // NOTE(Ryan): If we wanted to use specific drivers, we could use SDL_AudioInit()
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
                                        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
                                       );
  if (window == NULL) {
    SDL_Log("Unable to create SDL window: %s", SDL_GetError());
    return EXIT_FAILURE;
  }

  uint samples_per_second = 48000;
  bool sound_is_available = sdl_init_audio(samples_per_second);
  HHSoundBuffer sound_buffer = {0};

  if (sound_is_available) {
    // NOTE(Ryan): At 60fps, this will be 4 frames worth of audio data.
    uint latency_fraction = 15;
    uint num_game_samples = samples_per_second / latency_fraction; 

    sound_buffer.samples_per_second = samples_per_second;
    sound_buffer.samples = calloc(num_game_samples, sizeof(int16) * 2);
    sound_buffer.sample_count = num_game_samples;
  }
  

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
     }
    }

    // NOTE(Ryan): Sample rate is independent of channel, i.e. both sampled at 40kHz
    // latency = queue_size() / (samples_per_second * 2 * sizeof(int16)

    uint target_queue_bytes = sound_buffer.sample_count * sizeof(int16) * 2;
    // NOTE(Ryan): As some frames will run longer than expected, have extra padding bytes to ensure no silence.
    // However, we need to prevent a build up of queued audio data.
    uint bytes_to_write = 0;
    if (SDL_GetQueuedAudioSize(1) < target_queue_bytes) {
      bytes_to_write = target_queue_bytes - SDL_GetQueuedAudioSize(1); 
    }

    if (sound_is_available) {
      prepare_sound(&sound_buffer);

      SDL_QueueAudio(1, sound_buffer.samples, bytes_to_write);

      if (SDL_GetAudioStatus() != SDL_AUDIO_PLAYING) {
        SDL_Log("Playing audio");
        SDL_PauseAudio(0); 
      }
    }

   }

  return 0;
}

