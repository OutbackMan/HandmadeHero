#include "hh.h"
#include "hh.c"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <alsa/asoundlib.h>

#include <libudev.h>

#include <unistd.h>
#include <linux/joystick.h>
#include <fcntl.h>

INTERNAL u32
linux_joysticks(void)
{
  struct udev* udev_handle = udev_new();
  if (udev_handle == NULL) {
    // TODO(Ryan): unable to open udev   
  }

  struct udev_monitor* monitor = udev_monitor_new_from_netlink(udev_handle, "udev");
  udev_monitor_filter_add_match_subsystem_devtype(monitor, "input", NULL);
  udev_monitor_enable_recieving(monitor);
  int monitor_fd = udev_monitor_get_fd(monitor); 

  struct udev_enumerate* enumerate = udev_enumerate_new(udev_handle);
  udev_enumerate_add_match_subsystem(enumerate, "input");
  udev_enumerate_scan_devices(enumerate);

  struct udev_list_entry* devices = udev_enumerate_get_list_entry(enumerate); 
  struct udev_list_entry* device = NULL;
  udev_list_entry_foreach(device, devices) {
    char const* device_sysfs_path = udev_list_entry_get_name(device);
    struct udev_device* udev_device = udev_device_new_from_syspath(udev, device_sysfs_path);
    
    while (!udev_device_get_sysattr_value(udev_device, "capabilites/ev")) {
      udev_device_get_parent_with_subsystem_devtype(udev_device, "input", NULL); 
    }

    char const* val = udev_device_get_property_value(dev, "ID_INPUT_JOYSTICK");
    if (val != NULL && strcmp(val, "1") == 0) {
      char const* devfs_path = udev_device_get_devnode(udev_device);
      // add to global list of controllers
    }

      // this will be in event loop
  }

  udev_enumerate_unref(enumerate);

  while (true) {
    fd_set fds;  
    FD_ZERO(&fds);
    FD_SET(monitor_fd, &fds);
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    int ret = select(monitor_fd + 1, &fds, NULL, NULL, &tv);
    if (ret > 0 && FD_ISSET(monitor_fd, &fds)) {
      struct udev_device* device = udev_monitor_recieve_device(udev_monitor); 
      char const* action = udev_device_get_action(device);
      if (strcmp(action, "add") == 0) {
      
      }
      if (strcmp(action, "remove") == 0) {
      
      }
      // button_down/up, axis ??
    }
    // control the polling rate
    usleep(250 * 1000);
  }
}

// buttonDownFunc/UpFunc(), axisMoveFunc()
// processEvents() every frame

#if 0
GLOBAL struct ff_effect global_force_feedback_effect = {0};
force_feedback_effect.type = FF_RUMBLE;
force_feedback_effect.u.rumble.strong_magnitude = 60000;
force_feedback_effect.u.rumble.weak_magnitude = 0;
force_feedback_effect.replay.length = 200;
force_feedback_effect.replay.delay = 0;
force_feedback_effect.id = -1;
#endif

INTERNAL void
joystick_keys() 
{

  int fd = open("/dev/....", O_RDONLY);

  struct js_event event;
  while (read(fd, &event, sizeof(event)) > 0) {
    switch (event.type) {
      case JS_EVENT_BUTTON: {
        if (event.number == JOYSTICK_BUTTON_A) {
          puts("button a was pressed"); 
        }                        
      } break;
      case JS_EVENT_AXIS: {
                          
      } break;
    }
  }
}

INTERNAL void
alsa_audio(void)
{
  snd_pcm_t* pcm_handle = NULL;
  snd_pcm_hw_params_t* hw_params = NULL;

  // sound card <-> device
  // 48000, 4p, 512ps
  // TODO(Ryan): Investigate on implications of 'default'
  if (snd_pcm_open(&pcm_handle, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0) {
    // unable to open sound card
  }

  if (snd_pcm_hw_params_malloc(&hw_params) < 0) {
    // unable to obtain memory for hw configuration 
  }

  if (snd_pcm_hw_params_any(pcm_handle, hw_params) < 0) {
    // unable to load hw configuration
    goto __ALSA_INIT_ERROR__;
  }

  if (snd_pcm_hw_params_set_access(pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
    // unable to set sound buffer to consecutive alternate channel data
    goto __ALSA_INIT_ERROR__;
  } 

  if (snd_pcm_hw_params_set_format(pcm_handle, hw_params, SND_PCM_FORMAT_S16_LE) < 0) {
    goto __ALSA_INIT_ERROR__;
  } 

__ALSA_INIT_ERROR__:
  if (hw_params != NULL) {
    snd_pcm_hw_params_free(hw_params);
  }
}

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
linux_resize_or_create_pixel_buffer(LinuxPixelBuffer* restrict pixel_buffer, Display* display, XVisualInfo* restrict visual_info, uint width, uint height)
{
  if (pixel_buffer->memory != NULL) {
    XDestroyImage(pixel_buffer->info);
  }

  pixel_buffer->width = width;
  pixel_buffer->height = height;

  uint bytes_per_pixel = 4; 
  uint pixel_buffer_size = pixel_buffer->width * bytes_per_pixel * pixel_buffer->height;

  // NOTE(Ryan): Issue with using mmap() here
  pixel_buffer->memory = malloc(pixel_buffer_size);

  pixel_buffer->pitch = pixel_buffer->width * bytes_per_pixel;

  pixel_buffer->info = XCreateImage(
		                                display, 
				                            visual_info->visual, 
				                            visual_info->depth, 
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
linux_display_pixel_buffer_in_window(LinuxPixelBuffer* restrict pixel_buffer, Display* restrict display, Window window, int screen)
{
  XPutImage(
            display, 
	          window, 
	          DefaultGC(display, screen), 
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
    Window root_window = DefaultRootWindow(display);
    int screen = DefaultScreen(display); 

    uint desired_bits_per_pixel = 24;
    XVisualInfo visual_info = {0};
    // NOTE(Ryan): TrueColor segments 24 bits into RGB-8-8-8
    bool screen_has_desired_properties = XMatchVisualInfo(
		                                                      display, 
						                                              screen, 
						                                              desired_bits_per_pixel, 
						                                              TrueColor, 
						                                              &visual_info
					                                               ); 
    if (screen_has_desired_properties) {
      // NOTE(Ryan): C does not support assignment of values to struct, rather initialisation
      LinuxPixelBuffer pixel_buffer = {0}; 
      linux_resize_or_create_pixel_buffer(&pixel_buffer, display, &visual_info, 1280, 720);
   
      XSetWindowAttributes window_attr = {0};
      window_attr.bit_gravity = StaticGravity;
      window_attr.event_mask = StructureNotifyMask | KeyPressMask | KeyReleaseMask;
      window_attr.background_pixel = 0; 
      window_attr.colormap = XCreateColormap(
		                                         display, 
					                                   root_window, 
					                                   visual_info.visual, 
					                                   AllocNone
					                                  );
      unsigned long attr_mask = CWBackPixel | CWColormap | CWEventMask | CWBitGravity;

      Window window = XCreateWindow(
		                                display, 
				                            root_window, 
				                            0, 0, 1280, 720, 
				                            0, 
				                            visual_info.depth, 
				                            InputOutput,
		                                visual_info.visual, 
				                            attr_mask, 
				                            &window_attr
				                           );
      if (window != 0) {
        XStoreName(display, window, "Handmade Hero");

        // NOTE(Ryan): This because the window manager fails to correctly close the window, so we tell WM we want access to this
	      Atom WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", False);
	      if (!XSetWMProtocols(display, window, &WM_DELETE_WINDOW, 1)) {
          // TODO(Ryan): Logging
	      }

        XMapWindow(display, window);
 	      // NOTE(Ryan): Xlib may build up requests for efficiency
        XFlush(display);	

	      global_want_to_run = true;
	      XEvent event = {0};
        uint x_offset = 0;
        uint y_offset = 0;
	      while (global_want_to_run) {
          while (XPending(display) > 0) {
	          XNextEvent(display, &event);
	          switch (event.type) {
              case KeyPress: {
                XKeyPressedEvent* ev = (XKeyPressedEvent *)&event; 
                if (ev->keycode == XKeysymToKeycode(display, XK_Left)) puts("left key was pressed");
              } break;
              case KeyRelease: {
                 
              } break;
	            case ConfigureNotify: {
	              XConfigureEvent* ev = (XConfigureEvent *)&event;		    
	      	      linux_resize_or_create_pixel_buffer(&pixel_buffer, display, &visual_info, ev->width, ev->height);
	            } break;
	            case ClientMessage: {
                XClientMessageEvent* ev = (XClientMessageEvent *)&event;
	      	      if ((Atom)ev->data.l[0] == WM_DELETE_WINDOW) {
                  global_want_to_run = false;
	      	        XDestroyWindow(display, window);
                }
	            } break;
              case DestroyNotify: {
	              XDestroyWindowEvent* ev = (XDestroyWindowEvent *)&event;
	      	      if (ev->window == window) {
	                global_want_to_run = false;	
	      	      }
	            } break;
	          } 
	        }

          hh_render_gradient(&pixel_buffer, x_offset, y_offset);
	        linux_display_pixel_buffer_in_window(&pixel_buffer, display, window, screen);

          ++x_offset;
          y_offset += 2;
        }
      } else {
        // TODO(Ryan): Error Logging (window)
      }

    } else {
      // TODO(Ryan): Error Logging (screen props)
    }

  } else {
    // TODO(Ryan): Error Logging (display)
  }

  return 0;
}
