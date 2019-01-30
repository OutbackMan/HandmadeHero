#include "hh.h"

#include <AppKit/AppKit.h>

GLOBAL bool global_want_to_run;

// delegate is an object that responds to nswindow events
 
@interface WindowEventDelegate: NSObject<NSWindowDelegate>
@end
@implementation WindowEventDelegate

- (void)windowWillClose: (id)sender {
  global_want_to_run = false;
}

@end

  
int main(int argc, char* argv[argc + 1])
{
  // alloc is required to return usable memory for the object
  // init sets basic information for the object
  WindowEventDelegate* window_event_delegate = [[WindowEventDelegate alloc] init];

  NSRect screen_rect = [[NSScreen mainScreen] frame];

  NSRect window_rect = NSMakeRect(
                                  (screen_rect.size.width - 1280) * 0.5f,
                                  (screen_rect.size.height - 720) * 0.5f,
                                  1280, 
                                  720
                                 );

  NSWindow* window = [[NSWindow alloc] initWithContentRect: window_rect
                                        styleMask: NSWindowStyleMaskTitled | 
                                                   NSWindowStyleMaskClosable | 
                                                   NSWindowStyleMaskMiniaturizable | 
                                                   NSWindowStyleMaskResizable 
                                          backing: NSBackingStoreBuffered 
                                          defer: NO ];
  [window setBackgroundColor: NSColor.redColor];
  [window setTitle: @"Handmade Hero"];
  // NOTE(Ryan): The key window is the window that currently recieves keyboard events, i.e. is in focus 
  [window makeKeyAndOrderFront: nil];
  [window setDelegate: window_event_delegate];


  global_want_to_run = true;
  while (global_want_to_run) {
    NSEvent* event = nil; 

    do {
      event = [NSApp nextEventMatchingMask: NSEventMaskAny
                                 untilDate: nil
                                    inMode: NSDefaultRunLoopMode
                                   dequeue: YES];
      switch ([event type]) {
        default: {
          [NSApp sendEvent: event];          
        } 
      }
    
    } while (event != nil);
  }

  return 0;
}
