#include "hh.h"

#include <windows.h>

int CALLBACK
WinMain(HINSTANCE instance, HINSTANCE prev_instance, char** cmd_line, int cmd_show)
{
  MessageBoxA(0, "This is Handmade Hero", "Handmade Hero", MB_OK | MB_ICONINFORMATION);
  return 0;
}
