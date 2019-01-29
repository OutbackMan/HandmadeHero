#include "hh.h"

#include <windows.h>

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
hh_render_gradient(WindowsPixelBuffer* restrict pixel_buffer, uint green_offset, uint blue_offset)
{
  u8* row = (u8 *)pixel_buffer->memory;
  for (uint y = 0; y < pixel_buffer->height; ++y) {
    u32* pixel = (u32 *)row;
    for (uint x = 0; x < pixel_buffer->width; ++x) {
      u32 green_value = x + green_offset;
      u32 blue_value = y + blue_offset;
      *pixel++ = (green_value << 8 | blue_value); 
    }
    row += pixel_buffer->pitch;
  }
}

GLOBAL bool global_want_to_run;
GLOBAL WindowsPixelBuffer global_pixel_buffer;

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
        hh_render_gradient(&global_pixel_buffer, x_offset, y_offset);

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
