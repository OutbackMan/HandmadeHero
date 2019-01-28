#include "hh.h"

#include <windows.h>

// NOTE(Ryan): Recieved redefintion error on using char** for LPSTR
int CALLBACK
WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int cmd_show)
{
  MessageBoxA(0, "This is Handmade Hero", "Handmade Hero", MB_OK | MB_ICONINFORMATION);
  return 0;
}
