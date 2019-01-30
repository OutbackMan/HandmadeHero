#include "hh.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <sys/mman.h>

// dpkg internally installs .deb packages, while apt handles dependency management
// ubuntu releases more features than debian which is more stable (suitable for servers)
// apt-get install/remove/purge; apt-cache search/policy; apt list --installed; dpkg -L
// grep -rnw . -e "xcb_setup_roots_iterator"

typedef struct {
  XImage* info;
  void* memory;
  uint width;
  uint height;
  uint pitch;
} LinuxPixelBuffer;

INTERNAL void
linux_resize_or_create_pixel_buffer(LinuxPixelBuffer* restrict pixel_buffer, Display* display, XVisualInfo screen_info, uint width, uint height)
{
  if (pixel_buffer->memory != NULL) {
    XDestroyImage(pixel_buffer->info);
  }

  pixel_buffer->width = width;
  pixel_buffer->height = height;

  uint bytes_per_pixel = 4; 
  uint pixel_buffer_size = pixel_buffer->width * bytes_per_pixel * pixel_buffer->height;

  pixel_buffer->memory = mmap(
                              NULL, 
                              pixel_buffer_size, 
                              PROT_READ | PROT_WRITE, 
                              MAP_PRIVATE | MAP_ANONYMOUS, 
                              -1, 0
                             );

  pixel_buffer->pitch = pixel_buffer->width * bytes_per_pixel;

  pixel_buffer->info = XCreateImage(
		                                display, 
				                            screen_info.visual, 
				                            screen_info.depth, 
				                            ZPixmap, 
				                            0, 
				                            pixel_buffer->memory, 
				                            pixel_buffer->width, 
				                            pixel_buffer->height, 
				                            bytes_per_pixel * 8, 
				                            0
				                           );
}

INTERNAL void
linux_display_pixel_buffer_in_window(LinuxPixelBuffer* restrict pixel_buffer, Display* restrict display, int window_id, int screen_id)
{
  XPutImage(
            display, 
	          window_id, 
	          DefaultGC(display, screen_id), 
	          pixel_buffer->info, 
	          0, 0, 0, 0,
	          pixel_buffer->width,
	          pixel_buffer->height
	         );
}

INTERNAL void
hh_render_gradient(LinuxPixelBuffer* restrict pixel_buffer, uint green_offset, uint blue_offset)
{
  u8* row = (u8 *)pixel_buffer->memory; 
  for (uint y = 0; y < pixel_buffer->height; ++y) {
    u32* pixel = (u32 *)row; 
    for (uint x = 0; x < pixel_buffer->width; ++x) {
      u32 green = x + green_offset; 
      u32 blue = y + blue_offset; 
      *pixel++ = (green << 8 | blue << 16);
    }
    row += pixel_buffer->pitch;
  }
}

GLOBAL bool global_want_to_run;

int 
main(int argc, char* argv[argc + 1])
{

  Display* display = XOpenDisplay(":0.0");
  if (display != NULL) {
    int root_window_id = DefaultRootWindow(display);
    int screen_id = DefaultScreen(display); 

    uint desired_bits_per_pixel = 24;
    XVisualInfo screen_info = {0};
    // NOTE(Ryan): TrueColor segments 24 bits into RGB-8-8-8
    bool screen_has_desired_properties = XMatchVisualInfo(
		                                                      display, 
						                                              screen_id, 
						                                              desired_bits_per_pixel, 
						                                              TrueColor, 
						                                              &screen_info
					                                               ); 
    if (screen_has_desired_properties) {
      // NOTE(Ryan): C does not support assignment of values to struct, rather initialisation
      LinuxPixelBuffer pixel_buffer = {0}; 
      linux_resize_or_create_pixel_buffer(&pixel_buffer, display, screen_info, 1280, 720);
   
      XSetWindowAttributes window_attr = {0};
      window_attr.bit_gravity = StaticGravity;
      window_attr.event_mask = StructureNotifyMask; 
      window_attr.background_pixel = 0; 
      window_attr.colormap = XCreateColormap(
		                                         display, 
					                                   root_window_id, 
					                                   screen_info.visual, 
					                                   AllocNone
					                                  );
      unsigned long attr_mask = CWBackPixel | CWColormap | CWEventMask | CWBitGravity;

      int window_id = XCreateWindow(
		                                display, 
				                            root_window_id, 
				                            0, 0, 1280, 720, 
				                            0, 
				                            screen_info.depth, 
				                            InputOutput,
		                                screen_info.visual, 
				                            attr_mask, 
				                            &window_attr
				                           );
      if (window_id != 0) {
        XStoreName(display, window_id, "Handmade Hero");

        // NOTE(Ryan): This because the window manager fails to correctly close the window, so we tell WM we want access to this
	      Atom WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", False);
	      if (!XSetWMProtocols(display, window_id, &WM_DELETE_WINDOW, 1)) {
          // TODO(Ryan): Logging
	      }

        XMapWindow(display, window_id);
 	      // NOTE(Ryan): Xlib may build up requests for efficiency
        XFlush(display);	

	      global_want_to_run = true;
	      XEvent event = {0};
	      while (global_want_to_run) {
          uint x_offset = 0;
          uint y_offset = 0;
          while (XPending(display) > 0) {
	          XNextEvent(display, &event);
	          switch (event.type) {
	            case ConfigureNotify: {
	              XConfigureEvent* ev = (XConfigureEvent *)&event;		    
	      	      linux_resize_or_create_pixel_buffer(&pixel_buffer, display, screen_info, ev->width, ev->height);
	            } break;
	            case ClientMessage: {
                XClientMessageEvent* ev = (XClientMessageEvent *)&event;
	      	      if (ev->data.l[0] == WM_DELETE_WINDOW) {
                  global_want_to_run = false;
	      	        XDestroyWindow(display, window_id);
                }
	            } break;
              case DestroyNotify: {
	              XDestroyWindowEvent* ev = (XDestroyWindowEvent *)&event;
	      	      if (ev->window == window_id) {
	                global_want_to_run = false;	
	      	      }
	            } break;
	          } 
	        }

          hh_render_gradient(&pixel_buffer, x_offset, y_offset);
	        linux_display_pixel_buffer_in_window(&pixel_buffer, display, window_id, screen_id);

          ++x_offset;
          y_offset += 2;
        }
      } else {
        // TODO(Ryan): Error Logging (window_id)
      }

    } else {
      // TODO(Ryan): Error Logging (screen props)
    }

  } else {
    // TODO(Ryan): Error Logging (display)
  }

  return 0;
}
