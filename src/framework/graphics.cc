#include <string>
#include <list>
#include <thread>

#include <boost/foreach.hpp>

#include <SDL.h>

#include <framework/exception.h>
#include <framework/graphics.h>
#include <framework/settings.h>
#include <framework/logging.h>
#include <framework/texture.h>

namespace fw {

// The render thread is the thread we start on, so this is right.
static std::thread::id render_thread_id = std::this_thread::get_id();

std::string to_string(gl_error_info const &err_info) {
  GLenum err = err_info.value();
  char const *err_msg = reinterpret_cast<char const *>(gluErrorString(err));

  return (boost::format("0x%1$08x : %2%") % err % err_msg).str();
}

//--------------------------------------------------------------

graphics::graphics() :
    _wnd(nullptr), _context(nullptr), _windowed(false), _width(0), _height(0) {
}

graphics::~graphics() {
}

void graphics::initialize(char const *title) {
  settings stg;

  _windowed = stg.is_set("windowed");
  if (_windowed) {
    _width = stg.get_value<int>("windowed-width");
    _height = stg.get_value<int>("windowed-height");
  } else {
    _width = stg.get_value<int>("fullscreen-width");
    _height = stg.get_value<int>("fullscreen-height");
  }
  fw::debug << "Graphics initializing; window size=" << _width << "x" << _height << ", windowed=" << _windowed
      << std::endl;

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
  if (!_windowed) {
    if (_width == 0 || _height == 0) {
      flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    } else {
      flags |= SDL_WINDOW_FULLSCREEN;
    }
  }
  _wnd = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, _width, _height, flags);
  if (_wnd == nullptr) {
    BOOST_THROW_EXCEPTION(fw::exception() << fw::sdl_error_info(SDL_GetError()));
  }

  // If we didn't specify a width/height, we'll need to query the window for how big it is.
  if (_width == 0 || _height == 0) {
    SDL_GetWindowSize(_wnd, &_width, &_height);
  }

  _context = SDL_GL_CreateContext(_wnd);
  if (_context == nullptr) {
    BOOST_THROW_EXCEPTION(fw::exception() << fw::sdl_error_info(SDL_GetError()));
  }
  SDL_GL_MakeCurrent(_wnd, _context);

  glewExperimental = GL_TRUE;
  GLenum glewError = glewInit();
  if(glewError != GLEW_OK) {
    BOOST_THROW_EXCEPTION(fw::exception() << fw::gl_error_info(glewError));
  }
  GLenum err = glGetError();
  if (err == GL_INVALID_ENUM) {
    // Some versions of glew can cause this, we can ignore it.
    // See: https://www.opengl.org/wiki/OpenGL_Loading_Library
  } else if (err != GL_NO_ERROR) {
    // Something else happened!
    BOOST_THROW_EXCEPTION(fw::exception() << fw::gl_error_info(err));
  }

  if(SDL_GL_SetSwapInterval(1) < 0) {
    fw::debug << "Unable to set vsync, vsync is disabled. error: " << SDL_GetError() << std::endl;
  }

  fw::debug << "GLEW version: " << glewGetString(GLEW_VERSION) << std::endl;
  fw::debug << "GL vendor: " << glGetString(GL_VENDOR) << std::endl;
  fw::debug << "GL renderer: " << glGetString(GL_RENDERER) << std::endl;
  fw::debug << "GL version: " << glGetString(GL_VERSION) << std::endl;
  fw::debug << "shading language version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

  { // Bind a vertex array, but we never actually use it...
    GLuint id;
    FW_CHECKED(glGenVertexArrays(1, &id));
    FW_CHECKED(glBindVertexArray(id));
  }

  FW_CHECKED(glEnable(GL_DEPTH_TEST));
  FW_CHECKED(glViewport(0, 0, _width, _height));
}

void graphics::destroy() {
  SDL_GL_DeleteContext(_context);
  _context = nullptr;
}

void graphics::begin_scene(fw::colour clear_colour /*= fw::colour(1,0,0,0)*/) {
  if (_framebuffer) {
    FW_CHECKED(glViewport(0, 0, _framebuffer->get_width(), _framebuffer->get_height()));
  } else {
    FW_CHECKED(glViewport(0, 0, _width, _height));
  }
  FW_CHECKED(glClearColor(clear_colour.r, clear_colour.g, clear_colour.b, clear_colour.a));
  FW_CHECKED(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

void graphics::end_scene() {
}

void graphics::before_gui() {
  FW_CHECKED(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
  FW_CHECKED(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

void graphics::after_gui() {
}

void graphics::present() {
  sig_before_present();

  // Run any functions that were scheduled to run on the render thread
  {
    std::unique_lock<std::mutex>(_run_queue_mutex);
    BOOST_FOREACH(std::function<void()> fn, _run_queue) {
      fn();
    }
    _run_queue.clear();
  }

  GLenum err = glGetError();
  if (err != GL_NO_ERROR) {
    // this might happen in Release mode, where we don't actually check errors on every single call (it's expensive).
    BOOST_THROW_EXCEPTION(fw::exception() << fw::gl_error_info(err));
  }

  SDL_GL_SwapWindow(_wnd);
}

void graphics::run_on_render_thread(std::function<void()> fn) {
  std::unique_lock<std::mutex>(_run_queue_mutex);
  _run_queue.push_back(fn);
}

void graphics::set_render_target(std::shared_ptr<framebuffer> fb) {
  if (_framebuffer && !fb) {
    _framebuffer->unbind();
    _framebuffer = nullptr;
    return;
  }

  _framebuffer = fb;
  if (_framebuffer) {
    _framebuffer->bind();
  }
}

void graphics::check_error(char const *msg) {
  GLenum err = glGetError();
  if (err == GL_NO_ERROR)
    return;

  BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info(msg) << fw::gl_error_info(err));
}

void graphics::ensure_render_thread() {
  std::thread::id this_thread_id = std::this_thread::get_id();
  if (this_thread_id != render_thread_id) {
    BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info("Expected to be running on the render thread."));
  }
}

//-----------------------------------------------------------------------------

index_buffer::index_buffer(bool dynamic/*= false */) :
    _num_indices(0), _id(0), _dynamic(dynamic) {
  FW_CHECKED(glGenBuffers(1, &_id));
}

index_buffer::~index_buffer() {
  FW_ENSURE_RENDER_THREAD();
  FW_CHECKED(glDeleteBuffers(1, &_id));
}

void index_buffer::set_data(int num_indices, uint16_t const *indices, int flags) {
  FW_ENSURE_RENDER_THREAD();
  _num_indices = num_indices;

  if (flags <= 0)
    flags = _dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;

  FW_CHECKED(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _id));
  FW_CHECKED(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
      num_indices * sizeof(uint16_t), reinterpret_cast<void const *>(indices), flags));
}

void index_buffer::begin() {
  FW_ENSURE_RENDER_THREAD();
  FW_CHECKED(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _id));
}

void index_buffer::end() {
  FW_ENSURE_RENDER_THREAD();
  FW_CHECKED(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

//-----------------------------------------------------------------------------

vertex_buffer::vertex_buffer(setup_fn setup, size_t vertex_size, bool dynamic /*= false */) :
    _num_vertices(0), _vertex_size(vertex_size), _id(0), _dynamic(dynamic), _setup(setup) {
  FW_CHECKED(glGenBuffers(1, &_id));
}

vertex_buffer::~vertex_buffer() {
  FW_ENSURE_RENDER_THREAD();
  FW_CHECKED(glDeleteBuffers(1, &_id));
}

void vertex_buffer::set_data(int num_vertices, void *vertices, int flags /*= -1*/) {
  FW_ENSURE_RENDER_THREAD();
  if (flags <= 0) {
    flags = _dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
  }

  _num_vertices = num_vertices;

  FW_CHECKED(glBindBuffer(GL_ARRAY_BUFFER, _id));
  FW_CHECKED(glBufferData(GL_ARRAY_BUFFER, _num_vertices * _vertex_size, vertices, flags));
}

void vertex_buffer::begin() {
  FW_ENSURE_RENDER_THREAD();
  FW_CHECKED(glBindBuffer(GL_ARRAY_BUFFER, _id));
  _setup();
}

void vertex_buffer::end() {
  FW_ENSURE_RENDER_THREAD();
  FW_CHECKED(glBindBuffer(GL_ARRAY_BUFFER, 0));
  // todo: opposite of _setup()?
}

//-----------------------------------------------------------------------------

namespace vertex {

#define OFFSET_OF(struct, member) \
  reinterpret_cast<void const *>(offsetof(struct, member))

static void xyz_setup() {
  FW_CHECKED(glEnableVertexAttribArray(0));
  FW_CHECKED(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(fw::vertex::xyz), OFFSET_OF(xyz, x)));
}

std::function<void()> xyz::get_setup_function() {
  return &xyz_setup;
}

static void xyz_c_setup() {
  FW_CHECKED(glEnableVertexAttribArray(0));
  FW_CHECKED(glEnableVertexAttribArray(1));
  FW_CHECKED(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(fw::vertex::xyz_c), OFFSET_OF(xyz_c, x)));
  FW_CHECKED(glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(fw::vertex::xyz_c), OFFSET_OF(xyz_c, colour)));
}

std::function<void()> xyz_c::get_setup_function() {
  return &xyz_c_setup;
}

static void xyz_uv_setup() {
  FW_CHECKED(glEnableVertexAttribArray(0));
  FW_CHECKED(glEnableVertexAttribArray(1));
  FW_CHECKED(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(fw::vertex::xyz_uv), OFFSET_OF(xyz_uv, x)));
  FW_CHECKED(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(fw::vertex::xyz_uv), OFFSET_OF(xyz_uv, u)));
}

std::function<void()> xyz_uv::get_setup_function() {
  return &xyz_uv_setup;
}

void xyz_c_uv_setup() {
  FW_CHECKED(glEnableVertexAttribArray(0));
  FW_CHECKED(glEnableVertexAttribArray(1));
  FW_CHECKED(glEnableVertexAttribArray(2));
  FW_CHECKED(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(fw::vertex::xyz_c_uv), OFFSET_OF(xyz_c_uv, x)));
  FW_CHECKED(glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(fw::vertex::xyz_c_uv), OFFSET_OF(xyz_c_uv, colour)));
  FW_CHECKED(glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(fw::vertex::xyz_c_uv), OFFSET_OF(xyz_c_uv, u)));
}

std::function<void()> xyz_c_uv::get_setup_function() {
  return &xyz_c_uv_setup;
}

void xyz_n_uv_setup() {
  FW_CHECKED(glEnableVertexAttribArray(0));
  FW_CHECKED(glEnableVertexAttribArray(1));
  FW_CHECKED(glEnableVertexAttribArray(2));
  FW_CHECKED(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(fw::vertex::xyz_n_uv), OFFSET_OF(xyz_n_uv, x)));
  FW_CHECKED(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(fw::vertex::xyz_n_uv), OFFSET_OF(xyz_n_uv, nx)));
  FW_CHECKED(glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(fw::vertex::xyz_n_uv), OFFSET_OF(xyz_n_uv, u)));
}

std::function<void()> xyz_n_uv::get_setup_function() {
  return &xyz_n_uv_setup;
}

void xyz_n_setup() {
  FW_CHECKED(glEnableVertexAttribArray(0));
  FW_CHECKED(glEnableVertexAttribArray(1));
  FW_CHECKED(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(fw::vertex::xyz_n), OFFSET_OF(xyz_n_uv, x)));
  FW_CHECKED(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(fw::vertex::xyz_n), OFFSET_OF(xyz_n_uv, nx)));
}

std::function<void()> xyz_n::get_setup_function() {
  return &xyz_n_setup;
}

}
}
