#include "hh.h"

#include <AppKit/AppKit.h>

GLOBAL bool global_want_to_run;

// delegate is an object that responds to nswindow events
// this means the only methods it has are NSObject and essential NSWindowDelegate ones
@interface WindowEventDelegate: NSObject<NSWindowDelegate>
@end
@implementation WindowEventDelegate

// id is a pointer to any objective-c object
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
  // [window setBackgroundColor: NSColor.redColor];
  [window setTitle: @"Handmade Hero"];
  // NOTE(Ryan): The key window is the window that currently recieves keyboard events, i.e. is in focus 
  [window makeKeyAndOrderFront: nil];
  [window setDelegate: window_event_delegate];
  window.contentView.wantsLayer = YES;

  uint bitmap_width = 1280;
  uint bitmap_height = 720;
  uint bitmap_pitch = bitmap_width * 4;
  void* memory = malloc(bitmap_pitch * bitmap_height);

  // NOTE(Ryan): When resizing stuck in event loop
  NSBitmapImageRep* image_rep = [[NSBitmapImageRep alloc]
                                    initWithBitmapDataPlanes: &memory
                                                  pixelsWide: bitmap_width
                                                  pixelsHigh: bitmap_height
                                               bitsPerSample: 8
                                             samplesPerPixel: 4
                                                    hasAlpha: YES
                                                    isPlanar: NO
                                              colorSpaceName: NSDeviceRGBColorSpace
                                                 bytesPerRow: bitmap_pitch
                                                bitsPerPixel: 32];  
  NSSize image_size = NSMakeSize(bitmap_width, bitmap_height);
  NSImage* image = [[NSImage alloc] initWithSize: image_size];
  [image addRepresentation: image_rep];
  window.contentView.layer.contents = image;
  
  printf("width: %f, height: %f: \n", window.contentView.bounds.size.width, window.contentView.bounds.size.height);

  u8* row = (u8 *)memory;
  for (uint y = 0; y < bitmap_height; ++y) {
    u32* pixel = (u32 *)row; 
    for (uint x = 0; x < bitmap_width; ++x) {
      *pixel++ = 0xffffffff;
    }
    row += bitmap_pitch;
  }
 
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
