#pragma once

// If you don't define this, we'll disable support for NVPerfHUD (which is useful for final release, for example)
//#define FW_ENABLE_NVPERFHUD

#include <boost/signals2.hpp>

#include <GL/glew.h>
#include <SDL_opengl.h>

#include <framework/logging.h>
#include <framework/colour.h>

typedef void *SDL_GLContext;
struct SDL_Window;

namespace fw {
class texture;

// this is the type of error info we include with exception's when an error is detected by OpenGL
typedef boost::error_info<struct tag_glerr, GLenum> gl_error_info;

// converts a gl_error_info into a string
std::string to_string(gl_error_info const & err_info);

#if defined(DEBUG)
#define FW_CHECKED(fn) \
    fw::debug << #fn << std::endl; \
    fn; \
    fw::graphics::check_error(#fn)
#else
// in release mode, we don't check errors on each call... but we'll still do it once per frame.
#define FW_CHECKED(fn) \
    fn;
#endif

class graphics {
private:
  SDL_Window *_wnd;
  SDL_GLContext _context;

  int _width;
  int _height;
  bool _windowed;

public:
  graphics();
  ~graphics();

  void initialize(char const *title);
  void destroy();
  bool poll_events();

  void begin_scene(fw::colour clear_colour = fw::colour(1, 0, 0, 0));
  void end_scene();
  void present();

  // These are called by the CEGUI system just before & just after it renders.
  void before_gui();
  void after_gui();

  // Sets the render target to the given texture, or if the texture is NULL, back to the main backbuffer.
  void set_render_target(texture *tex);

  inline int get_width() { return _width; }
  inline int get_height() { return _height; }

  // This signal is fired just before the graphics present().
  boost::signals2::signal<void()> sig_before_present;

  // Checks glGetError() and throws an exception if it detects something.
  static void check_error(char const *msg);
};

}
