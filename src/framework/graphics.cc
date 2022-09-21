#include <string>
#include <list>
#include <thread>

#include <boost/foreach.hpp>

#include <SDL2/SDL.h>

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

Graphics::Graphics() :
    wnd_(nullptr), context_(nullptr), windowed_(false), width_(0), height_(0) {
}

Graphics::~Graphics() {
}

void Graphics::initialize(char const *title) {
  Settings stg;

  windowed_ = stg.is_set("windowed");
  if (windowed_) {
    width_ = stg.get_value<int>("windowed-width");
    height_ = stg.get_value<int>("windowed-height");
  } else {
    width_ = stg.get_value<int>("fullscreen-width");
    height_ = stg.get_value<int>("fullscreen-height");
  }
  fw::debug << "Graphics initializing; window size=" << width_ << "x" << height_ << ", windowed=" << windowed_
      << std::endl;

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
  if (!windowed_) {
    if (width_ == 0 || height_ == 0) {
      flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    } else {
      flags |= SDL_WINDOW_FULLSCREEN;
    }
  }
  wnd_ = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width_, height_, flags);
  if (wnd_ == nullptr) {
    BOOST_THROW_EXCEPTION(fw::Exception() << fw::sdl_error_info(SDL_GetError()));
  }

  // If we didn't specify a width/height, we'll need to query the window for how big it is.
  if (width_ == 0 || height_ == 0) {
    SDL_GetWindowSize(wnd_, &width_, &height_);
  }

  context_ = SDL_GL_CreateContext(wnd_);
  if (context_ == nullptr) {
    BOOST_THROW_EXCEPTION(fw::Exception() << fw::sdl_error_info(SDL_GetError()));
  }
  SDL_GL_MakeCurrent(wnd_, context_);

  glewExperimental = GL_TRUE;
  GLenum glewError = glewInit();
  if(glewError != GLEW_OK) {
    BOOST_THROW_EXCEPTION(fw::Exception() << fw::gl_error_info(glewError));
  }
  GLenum err = glGetError();
  if (err == GL_INVALID_ENUM) {
    // Some versions of glew can cause this, we can ignore it.
    // See: https://www.opengl.org/wiki/OpenGL_Loading_Library
  } else if (err != GL_NO_ERROR) {
    // Something else happened!
    BOOST_THROW_EXCEPTION(fw::Exception() << fw::gl_error_info(err));
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
  FW_CHECKED(glViewport(0, 0, width_, height_));
}

void Graphics::destroy() {
  SDL_GL_DeleteContext(context_);
  context_ = nullptr;
}

void Graphics::begin_scene(fw::Color clear_color /*= fw::color(1,0,0,0)*/) {
  if (framebuffer_) {
    FW_CHECKED(glViewport(0, 0, framebuffer_->get_width(), framebuffer_->get_height()));
    FW_CHECKED(glScissor(0, 0, framebuffer_->get_width(), framebuffer_->get_height()));
    FW_CHECKED(glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a));
    framebuffer_->clear();
  } else {
    FW_CHECKED(glViewport(0, 0, width_, height_));
    FW_CHECKED(glScissor(0, 0, width_, height_));
    FW_CHECKED(glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a));
    FW_CHECKED(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
  }
}

void Graphics::end_scene() {
}

void Graphics::before_gui() {
  FW_CHECKED(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
  FW_CHECKED(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

void Graphics::after_gui() {
}

void Graphics::present() {
  sig_before_present();

  GLenum err = glGetError();
  if (err != GL_NO_ERROR) {
    // this might happen in Release mode, where we don't actually check errors on every single call (it's expensive).
    BOOST_THROW_EXCEPTION(fw::Exception() << fw::gl_error_info(err));
  }

  SDL_GL_SwapWindow(wnd_);
}

/** Called on the render thread, after we've finished rendering. */
void Graphics::after_render() {
  // Run any functions that were scheduled to run on the render thread
  {
    std::lock_guard<std::mutex> lock(run_queue_mutex_);
    BOOST_FOREACH(std::function<void()> fn, run_queue_) {
      fn();
    }
    run_queue_.clear();
  }
}

void Graphics::run_on_render_thread(std::function<void()> fn) {
  std::lock_guard<std::mutex> lock(run_queue_mutex_);
  run_queue_.push_back(fn);
}

void Graphics::set_render_target(std::shared_ptr<Framebuffer> fb) {
  if (framebuffer_ && !fb) {
    framebuffer_->unbind();
    framebuffer_ = nullptr;
    return;
  }

  framebuffer_ = fb;
  if (framebuffer_) {
    framebuffer_->bind();
  }
}

void Graphics::check_error(char const *msg) {
  GLenum err = glGetError();
  if (err == GL_NO_ERROR)
    return;

  BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info(msg) << fw::gl_error_info(err));
}

void Graphics::ensure_render_thread() {
  std::thread::id this_thread_id = std::this_thread::get_id();
  if (this_thread_id != render_thread_id) {
    BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info("Expected to be running on the render thread."));
  }
}

void Graphics::toggle_fullscreen() {
  fw::debug << boost::format("switching to %1%...") % (windowed_ ? "full-screen" : "windowed") << std::endl;
  windowed_ = !windowed_;

  fw::Settings stg;
  if (windowed_) {
    width_ = stg.get_value<int>("windowed-width");
    height_ = stg.get_value<int>("windowed-height");
    SDL_SetWindowFullscreen(wnd_, 0);
    SDL_SetWindowSize(wnd_, width_, height_);
  } else {
    width_ = stg.get_value<int>("fullscreen-width");
    height_ = stg.get_value<int>("fullscreen-height");
    if (width_ != 0 && height_ != 0) {
      SDL_SetWindowSize(wnd_, width_, height_);
      SDL_SetWindowFullscreen(wnd_, SDL_WINDOW_FULLSCREEN);
    } else {
      SDL_SetWindowFullscreen(wnd_, SDL_WINDOW_FULLSCREEN_DESKTOP);
      SDL_GetWindowSize(wnd_, &width_, &height_);
    }
  }
}

//-----------------------------------------------------------------------------

IndexBuffer::IndexBuffer(bool dynamic/*= false */) :
    num_indices_(0), id_(0), dynamic_(dynamic) {
  FW_CHECKED(glGenBuffers(1, &id_));
}

IndexBuffer::~IndexBuffer() {
  FW_ENSURE_RENDER_THREAD();
  FW_CHECKED(glDeleteBuffers(1, &id_));
}

std::shared_ptr<IndexBuffer> IndexBuffer::create() {
  return std::shared_ptr<IndexBuffer>(new IndexBuffer());
}

void IndexBuffer::set_data(int num_indices, uint16_t const *indices, int flags) {
  FW_ENSURE_RENDER_THREAD();
  num_indices_ = num_indices;

  if (flags <= 0)
    flags = dynamic_ ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;

  FW_CHECKED(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id_));
  FW_CHECKED(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
      num_indices * sizeof(uint16_t), reinterpret_cast<void const *>(indices), flags));
}

void IndexBuffer::begin() {
  FW_ENSURE_RENDER_THREAD();
  FW_CHECKED(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id_));
}

void IndexBuffer::end() {
  FW_ENSURE_RENDER_THREAD();
  FW_CHECKED(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

//-----------------------------------------------------------------------------

VertexBuffer::VertexBuffer(setup_fn setup, size_t vertex_size, bool dynamic /*= false */) :
    num_vertices_(0), vertex_size_(vertex_size), id_(0), dynamic_(dynamic), setup_(setup) {
  FW_CHECKED(glGenBuffers(1, &id_));
}

VertexBuffer::~VertexBuffer() {
  FW_ENSURE_RENDER_THREAD();
  FW_CHECKED(glDeleteBuffers(1, &id_));
}

void VertexBuffer::set_data(int num_vertices, void *vertices, int flags /*= -1*/) {
  FW_ENSURE_RENDER_THREAD();
  if (flags <= 0) {
    flags = dynamic_ ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
  }

  num_vertices_ = num_vertices;

  FW_CHECKED(glBindBuffer(GL_ARRAY_BUFFER, id_));
  FW_CHECKED(glBufferData(GL_ARRAY_BUFFER, num_vertices_ * vertex_size_, vertices, flags));
}

void VertexBuffer::begin() {
  FW_ENSURE_RENDER_THREAD();
  FW_CHECKED(glBindBuffer(GL_ARRAY_BUFFER, id_));
  setup_();
}

void VertexBuffer::end() {
  FW_ENSURE_RENDER_THREAD();
  FW_CHECKED(glBindBuffer(GL_ARRAY_BUFFER, 0));
  // todo: opposite of setup_()?
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
  FW_CHECKED(glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(fw::vertex::xyz_c), OFFSET_OF(xyz_c, color)));
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
  FW_CHECKED(glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(fw::vertex::xyz_c_uv), OFFSET_OF(xyz_c_uv, color)));
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
