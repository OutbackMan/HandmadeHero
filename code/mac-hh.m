#include "hh.h"

GLOBAL global_want_to_run;

int main(int argc, char* argv[argc + 1])
{
  global_want_to_run = true;
  while (global_want_to_run) {
    NSEvent* event; 

    event = [NSApp nextEventMatchingMask: NSEventMaskAny]
  }

  return 0;
}
