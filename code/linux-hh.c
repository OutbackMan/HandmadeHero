#include "hh.h" // to call game
// platform specific helpers #include "linux-hh.h" 


// although wanting to use XCB, documentation is sparse and doesn't support opengl
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <sys/mman.h>
// sometimes have to interact with the window manager (there is a window manager spec)

// dpkg internally installs .deb packages, while apt handles dependency management
// ubuntu releases more features than debian which is more stable (suitable for servers)
// apt-get install/remove/purge; apt-cache search/policy; apt list --installed; dpkg -L
// grep -rnw . -e "xcb_setup_roots_iterator"

typedef struct {
  XImage* image;
  Display* display;
  int screen_id;
} LinuxPixelBufferInfo;

typedef struct {
  int id;
  uint width;
  uint height;
} LinuxWindow;

typedef struct {
  LinuxPixelBufferOutputInfo* output_info;
  byte* memory;
  uint width;
  uint height;
  uint pitch;
} LinuxPixelBuffer;

INTERNAL void 
linux_render_grid(LinuxPixelBuffer* pixel_buffer)
{

}

INTERNAL void
linux_display_pixel_buffer_in_window(LinuxPixelBuffer* pixel_buffer, LinuxWindow* window)
{
  XPutImage(
            pixel_buffer->output_info->display, 
	    window->id, 
	    DefaultGC(pixel_buffer->output_info->display, pixel_buffer->output_info->screen_id), 
	    pixel_buffer->output_info->image, 
	    0, 0, 0, 0, 
	    window->width,
	    window->height
	   );
}

INTERNAL void
linux_create_pixel_buffer(LinuxPixelBuffer* pixel_buffer, uint width, uint height)
{
  uint bits_per_pixel = 32;
  uint bytes_per_pixel = bits_per_pixel / 8;

  pixel_buffer->width = width;
  pixel_buffer->height = height;
  pixel_buffer->pitch = width * bytes_per_pixel;

  pixel_buffer->mem = mmap(NULL, pixel_buffer->pitch * height, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (pixel_buffer->mem == MAP_FAILED) {
    fprintf(stderr, "mmap failed for pixel buffer");
    exit(1);
  }
  // NOTE(Ryan): Bitmap is something stored locally. Pixmap can be sent to X 
  // NOTE(Ryan): As no pixel pad, 0 instructs self calculation
  pixel_buffer->info = XCreateImage(
		                    global_display, 
				    global_screen_info.visual, 
				    global_screen_info.depth, 
				    ZPixmap, 
				    0, 
				    pixel_buffer->memory, 
				    width, 
				    height, 
				    bits_per_pixel, 
				    0
				   );
}

GLOBAL bool global_is_running;
GLOBAL LinuxPixelBuffer pixel_buffer;

int 
main(int argc, char* argv[argc + 1])
{
  linux_create_pixel_buffer(&pixel_buffer, 1280, 720);

  Display* display = XOpenDisplay(":0.0");
  if (display != NULL) {
    int root_window_id = DefaultRootWindow(display);
    int screen_id = DefaultScreen(display); 

    uint desired_bits_per_pixel = 24;
    XVisualInfo screen_info = {0};
    // NOTE(Ryan): TrueColor segments 24 bits into RGB-8-8-8
    bool screen_has_desired_properties = XMatchVisualInfo(
		                                          display, 
						          default_screen_id, 
						          desired_bits_per_pixel, 
						          TrueColor, 
						          &screen_info
					                 ); 
    if (screen_has_desired_properties) {
      XSetWindowAttributes window_attr = {0};
      window_attr.bit_gravity = StaticGravity;
      window_attr.event_mask = StructureNotifyMask | KeyPressMask | KeyReleaseMask;
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
				    0, 0, 600, 600, 
				    0, 
				    screen_info.depth, 
				    InputOutput,
		                    screen_info.visual, 
				    attr_mask, 
				    &window_attr
				   );
      if (window_id != 0) {
        XStoreName(display, window_id, "Hello, world!");

        XSizeHints hints = {0};
	hints.min_width = 600;
	hints.min_height = 600;
	hints.flags |= PMinSize;
	XSetWMNormalHints(display, window, &hints);
	
        // NOTE(Ryan): This because the window manager fails to correctly close the window, so we tell WM we want access to this
	Atom WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", False);
	if (!XSetWMProtocols(display, window, &WM_DELETE_WINDOW, 1)) {
          // exit()	
	}

        XMapWindow(display, window);
	// NOTE(Ryan): Xlib may build up requests for efficiency
        XFlush(display);	

	LinuxPixelBuffer pixel_buffer = {0};
	linux_resize_pixel_buffer(&pixel_buffer, 1280, 720);

	global_is_running = true;
	XEvent event = {0};
	while (global_is_running) {
          while (XPending(display) > 0) {
	    XNextEvent(display, &event);
	    switch (event.type) {
	      case KeyPress: {
		XKeyPressedEvent* ev = (XKeyPressedEvent *)&event;
		if (ev->keycode == XKeysymToKeycode(display, XK_Left)) {
	          puts("pressed left arrow");	
		}
	      } break;
	      case ConfigureNotify: {
                // might be able to stretch the bits instead of reallocating
	        XConfigureEvent* ev = (XConfigureEvent *)&event;		    
		// linux_resize_pixel_buf();
		width = ev->width; 
		height = ev->height;
	      } break;
	      case ClientMessage: {
                XClientMessageEvent* ev = (XClientMessageEvent *)&event;
		if (ev->data.l[0] == WM_DELETE_WINDOW) {
		  XDestroyWindow(display, window);
		  window_is_opened = false;
		}
	      } break;
              case DestroyNotify: {
	        XDestroyNotifyEvent* ev = (XDestroyNotifyEvent *)&event;
		if (ev->window == window) {
		  // TODO(Ryan): Handle this as an error - recreate window?
	          window_is_opened = false;	
		}
	      } break;
	    } 
	  }

	// evdev for gamepads
        int pitch = width * bytes_per_pixel; 	
	for (int y = 0; y < height; ++y) {
          byte* row = mem + y * pitch;
	  for (int x = 0; x < width; ++x) {
	    u32* pixel = (u32 *)(row + x * bytes_per_pixel);
	    if (x % 16 && y % 16) {
              *pixel = 0xffffffff;
	    } else {
              *pixel = 0;
	    }
	  }
	}

        render_grid(back_buffer);

	display_buffer_in_window();

	}

      } else {
        // TODO(Ryan): Error Logging
      }

    } else {
      // TODO(Ryan): Error Logging
    }

  } else {
    // TODO(Ryan): Error Logging
  }

  return 0;
}
