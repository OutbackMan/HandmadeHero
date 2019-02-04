#include "hh.h"
#include "hh.c"

#include <windows.h>
#include <xinput.h>
#include <dsound.h>

typedef struct {
  BITMAPINFO info;
  void* memory;
  uint width;
  uint height;
  uint pitch;
} WindowsPixelBuffer;

INTERNAL void
windows_create_pixel_buffer(WindowsPixelBuffer* restrict pixel_buffer, uint width, uint height)
{
  pixel_buffer->width = width;
  pixel_buffer->height = height;
  
  uint bits_per_pixel = 32;
  pixel_buffer->info.bmiHeader.biSize = sizeof(pixel_buffer->info.bmiHeader);
  pixel_buffer->info.bmiHeader.biWidth = pixel_buffer->width; 
  pixel_buffer->info.bmiHeader.biHeight = -pixel_buffer->height; 
  pixel_buffer->info.bmiHeader.biPlanes = 1;
  pixel_buffer->info.bmiHeader.biBitCount = bits_per_pixel;
  pixel_buffer->info.bmiHeader.biCompression = BI_RGB;

  uint bytes_per_pixel = 4;
  uint pixel_buffer_size = pixel_buffer->width * bytes_per_pixel * pixel_buffer->height;
  pixel_buffer->memory = VirtualAlloc(NULL, pixel_buffer_size, MEM_COMMIT, PAGE_READWRITE);
  pixel_buffer->pitch = pixel_buffer->width * bytes_per_pixel;
}

// NOTE(Ryan): This should be called for WM_PAINT and inside event loop
// This is because WM_PAINT is only generated when the window has invalid regions, e.g. opened, resized
INTERNAL void
windows_display_pixel_buffer_in_window(WindowsPixelBuffer* restrict pixel_buffer, HDC device_context, HWND window)
{
  RECT rect;
  GetClientRect(window, &rect);
  uint window_width = rect.right - rect.left;
  uint window_height = rect.bottom - rect.top;

  StretchDIBits(
                device_context, 
                0, 0, window_width, window_height,
                0, 0, pixel_buffer->width, pixel_buffer->height,
                pixel_buffer->memory,
                &pixel_buffer->info,
                DIB_RGB_COLORS, SRCCOPY
               );  
}

INTERNAL void
windows_init_dsound(HWND window, int32 samples_per_second, int32 buffer_size)
{
  LPDIRECTSOUND direct_sound = NULL;
  if (DirectSoundCreate(NULL, &direct_sound, NULL) == DS_OK) {
    WAVEFORMATEX wave_format = {0};
    // NOTE(Ryan): PCM is PAM implementation for computers
    wave_format.wFormatTag = WAVE_FORMAT_PCM;
    wave_format.nChannels = 2;
    wave_format.nSamplesPerSec = samples_per_second;
    // NOTE(Ryan): May not need 16, only 8
    // As 2 channel, complete sample is 32bits
    wave_format.wBitsPerSample = 16; 
    wave_format.nBlockAlign = (wave_format.nChannels * wave_format.wBitsPerSample) / 8;
    wave_format.nAvgBytesPerSec = wave_format.nSamplesPerSec * wave_format.nBlockAlign;  
    wave_format.cbSize = 0;

    // 20Hz <--> 20000Hz (neiquist want sampling rate double of audible, i.e. 40000Hz, additional due to anti-aliasing) 
    // NOTE(Ryan): COM interface utilises virtual function in C++. 
    // As no implicit vtable in C, manually specify lpVtbl and pass original pointer to function
    if (direct_sound->lpVtbl->SetCooperativeLevel(direct_sound, window, DSSCL_PRIORITY) == DS_OK) { 
      DSBUFFERDESC buffer_description = {0};
      buffer_description.dwSize = sizeof(buffer_description);
      buffer_description.dwFlags = DSBCAPS_PRIMARYBUFFER;

      LPDIRECTSOUNDBUFFER primary_buffer = NULL;
      if (direct_sound->lpVtbl->CreateSoundBuffer(direct_sound, &buffer_description, &primary_buffer, NULL) == DS_OK) {
        if (primary_buffer->lpVtbl->SetFormat(primary_buffer, &wave_format) != DS_OK) {
          // TODO(Ryan): primary buffer format set fail
        } 
      } else {
        // TODO(Ryan): primary buffer create fail
      }
      memset(&buffer_description, 0, sizeof(buffer_description));
      buffer_description.dwSize = sizeof(buffer_description);
      buffer_description.dwFlags = 0;
      buffer_description.dwBufferBytes = buffer_size;
      buffer_description.lpwfxFormat = &wave_format;
      if (direct_sound->lpVtbl->CreateSoundBuffer(direct_sound, &buffer_description, &global_secondary_buffer, 0) != DS_OK) {
        // TODO(Ryan): secondary buffer create fail 
      } 
    } else {
      // TODO(Ryan): cooperative level fail
    } 
  } else {
    // TODO(Ryan): dsound init fail 
  }
}

GLOBAL bool global_want_to_run;
GLOBAL WindowsPixelBuffer global_pixel_buffer;
GLOBAL LPDIRECTSOUNDBUFFER global_secondary_buffer;

LRESULT CALLBACK
main_window_callback(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
  LRESULT result = 0;
  switch (message) {
    case WM_ACTIVATEAPP: {
      puts("wm_activateapp");  
    } break;
    case WM_PAINT: {
      PAINTSTRUCT paint;
      HDC device_context = BeginPaint(window, &paint);

      windows_display_pixel_buffer_in_window(&global_pixel_buffer, device_context, window);

      EndPaint(window, &paint);
    } break;
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP: {
      u32 vk_code = wparam;
      bool was_down = ((lparam & (1 << 30)) != 0);
      bool is_down = ((lparam & (1 << 31)) == 0);
      if (was_down != is_down) {
        if (vk_code == 'W') {
          if (was_down) {
            // 'w' was down 
          }
          if (is_down) {
            // 'w' is down 
          }
        } 
      }

      // NOTE(Ryan): Responding to SYSKEY means we have to reimplement alt-f4
      bool alt_key_was_down = !IS_BIT_SET(lparam, 29);
      if (vk_code == VK_F4 && alt_key_was_down) {
        global_want_to_run = false; 
      }
    } break;
    case WM_CLOSE: {
      global_want_to_run = false;
    } break;
    case WM_DESTROY: {
      global_want_to_run = false;
    } break;
    default: {
      result = DefWindowProc(window, message, wparam, lparam);  
    }
  }

  return result;
}

// NOTE(Ryan): Recieved redefinition error on using char** for LPSTR
// Keep in mind for 'CALLBACK' functions
int CALLBACK
WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int cmd_show)
{
  windows_create_pixel_buffer(&global_pixel_buffer, 1280, 720);
  HHPixelBuffer pixel_buffer = {
    .memory = global_pixel_buffer.memory,
    .width = global_pixel_buffer.width,
    .height = global_pixel_buffer.height,
    .pitch = global_pixel_buffer.pitch
  };

  WNDCLASS window_class = {0};
  window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  window_class.lpfnWndProc = main_window_callback;
  window_class.hInstance = instance;
  window_class.lpszClassName = "HandmadeHeroClassName";

  // NOTE(Ryan): 'A' for ANSI
  if (RegisterClassA(&window_class) != 0) {
    HWND window_handle = CreateWindowExA(
                                         0,
                                         window_class.lpszClassName,
                                         "Handmade Hero",
                                         WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                         CW_USEDEFAULT,
                                         CW_USEDEFAULT,
                                         CW_USEDEFAULT,
                                         CW_USEDEFAULT,
                                         0, 0,
                                         instance,
                                         0
                                         );
    if (window_handle != NULL) {
      uint x_offset = 0;
      uint y_offset = 0;
      HDC device_context = GetDC(window_handle);

      windows_init_dsound();
      // NOTE(Ryan): Don't require high priority as we are not mixing sounds
      global_secondary_buffer->lpVtbl->Play(global_secondary_buffer, 0, 0, DSBPLAY_LOOPING);

      global_want_to_run = true;
      while (global_want_to_run) {
        MSG message;
        while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
          if (message.message == WM_QUIT) {
            global_want_to_run = false;
          }
          TranslateMessage(&message);
          DispatchMessageA(&message);
        }

        for (uint controller_index = 0; controller_index < XUSER_MAX_COUNT; ++controller_index) {
          XINPUT_STATE controller_state = {0};
          if (XInputGetState(controller_index, &controller_state) == ERROR_SUCCESS) {
            bool up = controller_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP;
            int16 stick_x = controller_state.Gamepad.sThumbLX; 
            x_offset += stick_x >> 12;
          }
        }

        hh_render_gradient(&pixel_buffer, x_offset, y_offset);

        // NOTE(Ryan): As audio is running asynchronously, we use the write cursor 
        // which is ahead of the play cursor
        u32 play_cursor;
        u32 write_cursor;
        global_secondary_buffer->lpVtbl->GetCurrentPosition(global_secondary_buffer, &player_cursor, &write_cursor);
        
        u32 write_pointer;
        u32 num_bytes_to_lock;
        void* region_1;
        u32 region_1_size;
        void* region_2;
        u32 region_2_size;
        global_secondary_buffer->lpVtbl->Lock(
                                              global_secondary_buffer,
                                              write_pointer,
                                              bytes_to_write,
                                              &region_1, &region_1_size,
                                              &region_2, &region_2_size,
                                              0
                                             );
        u16* sample = (u16 *)region_1;
        u32 region_1_sample_count = region_1_size / 4;
        for (u32 complete_sample_index = 0; complete_sample_index < region_1_sample_count; ++complete_sample_index) {
          *sample++ = 16000; 
          *sample++ = 16000; 
        }
        sample = (u16 *)region_2;
        u32 region_2_sample_count = region_2_size / 4;
        for (u32 complete_sample_index = 0; complete_sample_index < region_1_sample_count; ++complete_sample_index) {
          *sample++ = 16000; 
          *sample++ = 16000; 
        }

        global_secondary_buffer->lpVtbl->Unlock(global_secondary_buffer, region_1, region_1_size, region_2, region_2_size);

        windows_display_pixel_buffer_in_window(&global_pixel_buffer, device_context, window_handle);

        ++x_offset;
        y_offset += 2;
      }       
    } else {
      // TODO(Ryan): Logging 
    }
  } else {
    // TODO(Ryan): Logging 
  }

  return 0;
}
