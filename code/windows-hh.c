#include "hh.h"

#include <windows.h>

GLOBAL bool global_want_to_run;

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
      uint x = paint.rcPaint.left;
      uint y = paint.rcPaint.right;
      uint width = paint.rcPaint.right - paint.rcPaint.left; 
      uint height = paint.rcPaint.bottom - paint.rcPaint.top; 
      // TODO(Ryan): Aspect ratio correction before blit --> get to day 5 and then do the debugger walkthrough
      update_window(device_context, x, y, width, height);
      EndPaint(window, &paint);
    } break;
    case WM_SIZE: {
      puts("wm_size");  
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

typedef struct {
  BITMAPINFO info;
  void* memory;
  uint width;
  uint height;
  uint pitch;
} WindowsBackBuffer;

void
windows_create_back_buffer(WindowsBackBuffer* restrict back_buffer, uint width, uint height)
{
  if (back_buffer->memory != NULL) {
    VirtualFree(back_buffer->memory, 0, MEM_RELEASE);  
  }

  
}

// NOTE(Ryan): Recieved redefintion error on using char** for LPSTR
// Keep in mind for 'CALLBACK' functions
int CALLBACK
WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int cmd_show)
{
  WindowsBackBuffer back_buffer = {0};
  windows_create_back_buffer(&back_buffer, 1280, 720);

  WNDCLASS window_class = {0};
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
      global_want_to_run = true;
      while (global_want_to_run) {
        MSG message;
        BOOL message_result = GetMessageA(&message, 0, 0, 0);
        if (message_result > 0) {
          TranslateMessage(&message);
          DispatchMessageA(&message);
        } else {
          break; 
        }
      }       
    } else {
      // TODO(Ryan): Logging 
    }
  } else {
    // TODO(Ryan): Logging 
  }

  return 0;
}
