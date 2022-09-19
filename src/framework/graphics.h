#pragma once

#include <functional>
#include <mutex>

#define BOOST_BIND_NO_PLACEHOLDERS // so it doesn't auto-include _1, _2 etc.
#include <boost/signals2.hpp>

#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>

#include <framework/logging.h>
#include <framework/colour.h>

typedef void *SDL_GLContext;
struct SDL_Window;

namespace fw {
class Framebuffer;

// this is the type of error info we include with exception's when an error is detected by OpenGL
typedef boost::error_info<struct tag_glerr, GLenum> gl_error_info;

// converts a gl_error_info into a string
std::string to_string(gl_error_info const & err_info);

#if defined(DEBUG)
#define FW_CHECKED(fn) \
  fn; \
  fw::graphics::check_error(#fn)
#else
// in release mode, we don't check errors on each call... but we'll still do it once per frame.
#define FW_CHECKED(fn) \
  fn;
#endif

#if defined(DEBUG)
#define FW_ENSURE_RENDER_THREAD() \
  fw::graphics::ensure_render_thread()
#else
#define FW_ENSURE_RENDER_THREAD()
#endif

class graphics {
private:
  SDL_Window *_wnd;
  SDL_GLContext _context;
  std::mutex _run_queue_mutex;
  std::vector<std::function<void()>> _run_queue;
  std::shared_ptr<fw::Framebuffer> _framebuffer;

  int _width;
  int _height;
  bool _windowed;

public:
  graphics();
  ~graphics();

  void initialize(char const *title);
  void destroy();

  void begin_scene(fw::colour clear_colour = fw::colour(1, 0, 0, 0));
  void end_scene();
  void present();
  void after_render();

  // toggle between windowed and fullscreen mode.
  void toggle_fullscreen();

  // These are called by the CEGUI system just before & just after it renders.
  void before_gui();
  void after_gui();

  // Sets the render target to the given framebuffer.
  void set_render_target(std::shared_ptr<fw::Framebuffer> fb);

  inline int get_width() { return _width; }
  inline int get_height() { return _height; }

  // This signal is fired just before the graphics present().
  boost::signals2::signal<void()> sig_before_present;

  /** Schedules the given function to run on the render thread, just after the scene has finished drawing. */
  void run_on_render_thread(std::function<void()> fn);

  // Checks glGetError() and throws an exception if it detects something.
  static void check_error(char const *msg);

  /** For things that must run on the render thread, this can be sure to enforce it. */
  static void ensure_render_thread();
};

/** An index buffer holds lists of indices into a vertex_buffer. */
class index_buffer: private boost::noncopyable {
private:
  GLuint _id;
  int _num_indices;
  bool _dynamic;

public:
  index_buffer(bool dynamic = false);
  ~index_buffer();

  static std::shared_ptr<index_buffer> create();

  void set_data(int num_indices, uint16_t const *indices, int flags = -1);

  inline int get_num_indices() const {
    return _num_indices;
  }

  // Called just before and just after we're going to render with this index buffer.
  void begin();
  void end();
};

/** A wrapper around a vertex buffer. */
class vertex_buffer: private boost::noncopyable {
public:
  typedef std::function<void()> setup_fn;

private:
  GLuint _id;
  int _num_vertices;
  size_t _vertex_size;
  bool _dynamic;

  setup_fn _setup;

public:
  vertex_buffer(setup_fn setup, size_t vertex_size, bool dynamic = false);
  virtual ~vertex_buffer();

  // Helper function that makes it easier to create vertex buffers by assuming that you're passing a type
  // defined in fw::vertex::xxx (or something compatible).
  template<typename T>
  static inline std::shared_ptr<vertex_buffer> create(bool dynamic = false) {
    return std::shared_ptr<vertex_buffer>(new vertex_buffer(
        T::get_setup_function(), sizeof(T), dynamic));
  }

  void set_data(int num_vertices, void *vertices, int flags = -1);

  inline int get_num_vertices() const {
    return _num_vertices;
  }

  void begin();
  void end();
};

namespace vertex {

struct xyz {
  inline xyz() :
      x(0), y(0), z(0) {
  }

  inline xyz(float x, float y, float z) :
      x(x), y(y), z(z) {
  }

  inline xyz(xyz const &copy) :
      x(copy.x), y(copy.y), z(copy.z) {
  }

  float x, y, z;

  // returns a function that'll set up a vertex buffer (i.e. with calls to glXyzPointer)
  static std::function<void()> get_setup_function();
};

struct xyz_c {
  inline xyz_c() :
      x(0), y(0), z(0), colour(0) {
  }

  inline xyz_c(float x, float y, float z, uint32_t colour) :
      x(x), y(y), z(z), colour(colour) {
  }

  inline xyz_c(float x, float y, float z, fw::colour const &colour) :
      x(x), y(y), z(z), colour(colour.to_abgr()) {
  }

  inline xyz_c(xyz_c const &copy) :
      x(copy.x), y(copy.y), z(copy.z), colour(copy.colour) {
  }

  float x, y, z;
  uint32_t colour;

  // returns a function that'll set up a vertex buffer (i.e. with calls to glXyzPointer)
  static std::function<void()> get_setup_function();
};

struct xyz_uv {
  inline xyz_uv() :
      x(0), y(0), z(0), u(0), v(0) {
  }

  inline xyz_uv(float x, float y, float z, float u, float v) :
      x(x), y(y), z(z), u(u), v(v) {
  }

  inline xyz_uv(xyz_uv const &copy) :
      x(copy.x), y(copy.y), z(copy.z), u(copy.u), v(copy.v) {
  }

  float x, y, z;
  float u, v;

  // returns a function that'll set up a vertex buffer (i.e. with calls to glXyzPointer)
  static std::function<void()> get_setup_function();
};

struct xyz_c_uv {
  inline xyz_c_uv() :
      x(0), y(0), z(0), colour(0), u(0), v(0) {
  }

  inline xyz_c_uv(float x, float y, float z, uint32_t colour, float u, float v) :
      x(x), y(y), z(z), colour(colour), u(u), v(v) {
  }

  inline xyz_c_uv(xyz_c_uv const &copy) :
      x(copy.x), y(copy.y), z(copy.z), colour(copy.colour), u(copy.u), v(copy.v) {
  }

  float x, y, z;
  uint32_t colour;
  float u, v;

  // returns a function that'll set up a vertex buffer (i.e. with calls to glXyzPointer)
  static std::function<void()> get_setup_function();
};

struct xyz_n_uv {
  inline xyz_n_uv() :
      x(0), y(0), z(0), nx(0), ny(0), nz(0), u(0), v(0) {
  }

  inline xyz_n_uv(float x, float y, float z, float nx, float ny, float nz,
      float u, float v) :
      x(x), y(y), z(z), nx(nx), ny(ny), nz(nz), u(u), v(v) {
  }

  inline xyz_n_uv(xyz_n_uv const &copy) :
      x(copy.x), y(copy.y), z(copy.z), nx(copy.nx), ny(copy.ny), nz(copy.nz), u(copy.u), v(copy.v) {
  }

  float x, y, z;
  float nx, ny, nz;
  float u, v;

  // returns a function that'll set up a vertex buffer (i.e. with calls to glXyzPointer)
  static std::function<void()> get_setup_function();
};

struct xyz_n {
  inline xyz_n() :
      x(0), y(0), z(0), nx(0), ny(0), nz(0) {
  }

  inline xyz_n(float x, float y, float z, float nx, float ny, float nz) :
      x(x), y(y), z(z), nx(nx), ny(ny), nz(nz) {
  }

  inline xyz_n(xyz_n const &copy) :
      x(copy.x), y(copy.y), z(copy.z), nx(copy.nx), ny(copy.ny), nz(copy.nz) {
  }

  float x, y, z;
  float nx, ny, nz;

  // returns a function that'll set up a vertex buffer (i.e. with calls to glXyzPointer)
  static std::function<void()> get_setup_function();
};

}
}
