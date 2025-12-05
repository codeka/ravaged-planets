#include <string>
#include <list>
#include <thread>

#include <absl/strings/str_cat.h>

#include <SDL2/SDL.h>

#include <framework/graphics.h>
#include <framework/settings.h>
#include <framework/logging.h>
#include <framework/texture.h>

namespace fw {
namespace {
// The render thread is the thread we start on, so this is right.
static std::thread::id render_thread_id = std::this_thread::get_id();

int query(GLenum pname) {
  int value;
  glGetIntegerv(pname, &value);
  return value;
}

void GLAPIENTRY debug_callback(
    GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message,
    const void* userParam) {
  std::string_view source_name = absl::StrCat(source);
  switch (source) {
    case GL_DEBUG_SOURCE_API:
      source_name = "GL_DEBUG_SOURCE_API";
      break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
      source_name = "GL_DEBUG_SOURCE_WINDOW_SYSTEM";
      break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
      source_name = "GL_DEBUG_SOURCE_SHADER_COMPILER";
      break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:
      source_name = "GL_DEBUG_SOURCE_THIRD_PARTY";
      break;
    case GL_DEBUG_SOURCE_APPLICATION:
      source_name = "GL_DEBUG_SOURCE_APPLICATION";
      break;
    case GL_DEBUG_SOURCE_OTHER:
      source_name = "GL_DEBUG_SOURCE_OTHER";
      break;
  }

  std::string_view type_name = absl::StrCat(type);
  switch (type) {
    case GL_DEBUG_TYPE_ERROR:
      type_name = "GL_DEBUG_TYPE_ERROR";
      break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
      type_name = "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR";
      break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
      type_name = "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR";
      break;
    case GL_DEBUG_TYPE_PORTABILITY:
      type_name = "GL_DEBUG_TYPE_PORTABILITY";
      break;
    case GL_DEBUG_TYPE_PERFORMANCE:
      type_name = "GL_DEBUG_TYPE_PERFORMANCE";
      break;
    case GL_DEBUG_TYPE_MARKER:
      type_name = "GL_DEBUG_TYPE_MARKER";
      break;
    case GL_DEBUG_TYPE_PUSH_GROUP:
      type_name = "GL_DEBUG_TYPE_PUSH_GROUP";
      break;
    case GL_DEBUG_TYPE_POP_GROUP:
      type_name = "GL_DEBUG_TYPE_POP_GROUP";
      break;
    case GL_DEBUG_TYPE_OTHER:
      type_name = "GL_DEBUG_TYPE_OTHER";
      break;
  }

  std::string_view severity_name = absl::StrCat(severity);
  switch (severity) {
    case GL_DEBUG_SEVERITY_LOW:
      severity_name = "GL_DEBUG_SEVERITY_LOW";
      break;
    case GL_DEBUG_SEVERITY_MEDIUM:
      severity_name = "GL_DEBUG_SEVERITY_MEDIUM";
      break;
    case GL_DEBUG_SEVERITY_HIGH:
      severity_name = "GL_DEBUG_SEVERITY_HIGH";
      break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
      severity_name = "GL_DEBUG_SEVERITY_NOTIFICATION";
      break;
  }


  LOG(ERR) << "opengl error\n  source=" << source_name << "\n  type=" << type_name
           << "\n  severity=" << severity_name << "\n  id=" << id << "\n  "
           << message;
  // TODO: can we use userParam?
}

}  // namespace

Graphics::Graphics() :
    wnd_(nullptr), context_(nullptr), windowed_(false), width_(0), height_(0) {
}

Graphics::~Graphics() {
}

fw::Status Graphics::initialize(char const *title) {
  windowed_ = Settings::get<bool>("windowed");
  if (windowed_) {
    width_ = Settings::get<int>("windowed-width");
    height_ = Settings::get<int>("windowed-height");
  } else {
    width_ = Settings::get<int>("fullscreen-width");
    height_ = Settings::get<int>("fullscreen-height");
  }
  LOG(INFO) << "Graphics initializing; window size=" << width_ << "x" << height_ 
            << ", windowed=" << windowed_;

  // Add SDL_GL_CONTEXT_DEBUG_FLAG to the default set of context flags. This will let us work with
  // GL_ARB_debug_output to get better error information.
  int context_flags = 0;
  SDL_GL_GetAttribute(SDL_GL_CONTEXT_FLAGS, &context_flags);
  context_flags |= SDL_GL_CONTEXT_DEBUG_FLAG;
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, context_flags);

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 4);
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
  wnd_ = SDL_CreateWindow(
      title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width_, height_, flags);
  if (wnd_ == nullptr) {
    return fw::ErrorStatus("Could not create window: ") << SDL_GetError();
  }

  // If we didn't specify a width/height, we'll need to query the window for how big it is.
  if (width_ == 0 || height_ == 0) {
    SDL_GetWindowSize(wnd_, &width_, &height_);
  }

  context_ = SDL_GL_CreateContext(wnd_);
  if (context_ == nullptr) {
    return fw::ErrorStatus("Could not create context: ") << SDL_GetError();
  }
  SDL_GL_MakeCurrent(wnd_, context_);

  glewExperimental = GL_TRUE;
  GLenum glewError = glewInit();
  if(glewError != GLEW_OK) {
    return fw::ErrorStatus("Error initializing GLEW: ") << glewError;
  }
  GLenum err = glGetError();
  if (err == GL_INVALID_ENUM) {
    // Some versions of glew can cause this, we can ignore it.
    // See: https://www.opengl.org/wiki/OpenGL_Loading_Library
  } else if (err != GL_NO_ERROR) {
    // Something else happened!
    return fw::ErrorStatus("Error after initializing GLEW: ") << err;
  }

  if(SDL_GL_SetSwapInterval(0) < 0) {
    LOG(INFO) << "Unable to set vsync, vsync is disabled. error: " << SDL_GetError();
  }

  LOG(INFO) << "GLEW version: " << glewGetString(GLEW_VERSION);
  LOG(INFO) << "GL vendor: " << glGetString(GL_VENDOR);
  LOG(INFO) << "GL renderer: " << glGetString(GL_RENDERER);
  LOG(INFO) << "GL version: " << glGetString(GL_VERSION);
  LOG(INFO) << "shading language version: " << glGetString(GL_SHADING_LANGUAGE_VERSION);
  LOG(INFO) << "  GL_MAX_TEXTURE_SIZE=" << query(GL_MAX_TEXTURE_SIZE);
  LOG(INFO) << "  GL_MAX_TEXTURE_IMAGE_UNITS=" << query(GL_MAX_TEXTURE_IMAGE_UNITS);
  LOG(INFO) << "  GL_MAX_ARRAY_TEXTURE_LAYERS=" << query(GL_MAX_ARRAY_TEXTURE_LAYERS);

  if (GLEW_ARB_debug_output) {
    glDebugMessageCallbackARB(&debug_callback, nullptr);
#ifdef DEBUG
    // In debug builds, enable synchronous output for easier debugging (performance heavy)
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
#endif

    // Limit the debug messages to just errors.
    // TODO: add a command-line flag to enable more detailed debugging?
    glDebugMessageControl(
        GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_FALSE);
    glDebugMessageControl(
        GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_ERROR, GL_DONT_CARE, 0, NULL, GL_TRUE);

    LOG(DBG) << "ARB_debug_output enabled";
  }

  { // Bind a vertex array, but we never actually use it...
    GLuint id;
    glGenVertexArrays(1, &id);
    glBindVertexArray(id);
  }

  glEnable(GL_DEPTH_TEST);
  glViewport(0, 0, width_, height_);
  return OkStatus();
}

void Graphics::destroy() {
  SDL_GL_DeleteContext(context_);
  context_ = nullptr;
}

void Graphics::begin_scene(fw::Color clear_color /*= fw::color(1,0,0,0)*/) {
  if (framebuffer_) {
    glViewport(0, 0, framebuffer_->get_width(), framebuffer_->get_height());
    glScissor(0, 0, framebuffer_->get_width(), framebuffer_->get_height());
    glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
    framebuffer_->clear();
  } else {
    glViewport(0, 0, width_, height_);
    glScissor(0, 0, width_, height_);
    glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }
}

void Graphics::end_scene() {
}

void Graphics::before_gui() {
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Graphics::after_gui() {
}

void Graphics::present() {
  sig_before_present.Emit();

  GLenum err = glGetError();
  if (err != GL_NO_ERROR) {
    // This might happen in Release mode, where we don't actually check errors on every single call
    // (it's expensive).
    LOG(ERR) << "opengl error occured: " << err;
    // TODO: better error info?
  }

  SDL_GL_SwapWindow(wnd_);
}

/** Called on the render thread, after we've finished rendering. */
void Graphics::after_render() {
  // Run any functions that were scheduled to run on the render thread
  {
    std::lock_guard<std::mutex> lock(run_queue_mutex_);
    for(std::function<void()> fn : run_queue_) {
      fn();
    }
    run_queue_.clear();
  }
}

void Graphics::run_on_render_thread(std::function<void()> fn) {
  if (is_render_thread()) {
    // If we're already on the render thread, just run it now.
    fn();
    return;
  }

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

/* static */
bool Graphics::is_render_thread() {
  std::thread::id this_thread_id = std::this_thread::get_id();
  return this_thread_id == render_thread_id;
}

/* static */
void Graphics::ensure_render_thread() {
  if (!is_render_thread()) {
    LOG(ERR) << ErrorStatus("expected to be running on the render thread");
    // TODO: something better?
    std::terminate();
  }
}

void Graphics::toggle_fullscreen() {
  LOG(INFO) << "switching to " << (windowed_ ? "full-screen" : "windowed");
  windowed_ = !windowed_;

  if (windowed_) {
    width_ = Settings::get<int>("windowed-width");
    height_ = Settings::get<int>("windowed-height");
    SDL_SetWindowFullscreen(wnd_, 0);
    SDL_SetWindowSize(wnd_, width_, height_);
  } else {
    width_ = Settings::get<int>("fullscreen-width");
    height_ = Settings::get<int>("fullscreen-height");
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
  glGenBuffers(1, &id_);
}

IndexBuffer::~IndexBuffer() {
  FW_ENSURE_RENDER_THREAD();
  glDeleteBuffers(1, &id_);
}

std::shared_ptr<IndexBuffer> IndexBuffer::create() {
  return std::shared_ptr<IndexBuffer>(new IndexBuffer());
}

void IndexBuffer::set_data(int num_indices, uint16_t const *indices, int flags) {
  FW_ENSURE_RENDER_THREAD();
  num_indices_ = num_indices;

  if (flags <= 0)
    flags = dynamic_ ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
      num_indices * sizeof(uint16_t), reinterpret_cast<void const *>(indices), flags);
}

void IndexBuffer::begin() {
  FW_ENSURE_RENDER_THREAD();
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id_);
}

void IndexBuffer::end() {
  FW_ENSURE_RENDER_THREAD();
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

//-----------------------------------------------------------------------------

VertexBuffer::VertexBuffer(setup_fn setup, size_t vertex_size, bool dynamic /*= false */) :
    num_vertices_(0), vertex_size_(vertex_size), id_(0), dynamic_(dynamic), setup_(setup) {
  FW_ENSURE_RENDER_THREAD();
  glGenBuffers(1, &id_);
}

VertexBuffer::~VertexBuffer() {
  FW_ENSURE_RENDER_THREAD();
  glDeleteBuffers(1, &id_);
}

void VertexBuffer::set_data(int num_vertices, const void *vertices, int flags /*= -1*/) {
  FW_ENSURE_RENDER_THREAD();
  if (flags <= 0) {
    flags = dynamic_ ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
  }

  num_vertices_ = num_vertices;

  glBindBuffer(GL_ARRAY_BUFFER, id_);
  glBufferData(GL_ARRAY_BUFFER, num_vertices_ * vertex_size_, vertices, flags);
}

void VertexBuffer::begin() {
  FW_ENSURE_RENDER_THREAD();
  glBindBuffer(GL_ARRAY_BUFFER, id_);
  setup_();
}

void VertexBuffer::end() {
  FW_ENSURE_RENDER_THREAD();
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  // todo: opposite of setup_()?
}

//-----------------------------------------------------------------------------

namespace vertex {

#define OFFSET_OF(struct, member) \
  reinterpret_cast<void const *>(offsetof(struct, member))

static void xyz_setup() {
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(fw::vertex::xyz), OFFSET_OF(xyz, x));
}

std::function<void()> xyz::get_setup_function() {
  return &xyz_setup;
}

static void xyz_c_setup() {
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(fw::vertex::xyz_c), OFFSET_OF(xyz_c, x));
  glVertexAttribPointer(
      1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(fw::vertex::xyz_c), OFFSET_OF(xyz_c, color));
}

std::function<void()> xyz_c::get_setup_function() {
  return &xyz_c_setup;
}

static void xyz_uv_setup() {
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(fw::vertex::xyz_uv), OFFSET_OF(xyz_uv, x));
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(fw::vertex::xyz_uv), OFFSET_OF(xyz_uv, u));
}

std::function<void()> xyz_uv::get_setup_function() {
  return &xyz_uv_setup;
}

void xyz_c_uv_setup() {
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(
      0, 3, GL_FLOAT, GL_FALSE, sizeof(fw::vertex::xyz_c_uv), OFFSET_OF(xyz_c_uv, x));
  glVertexAttribPointer(
      1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(fw::vertex::xyz_c_uv), OFFSET_OF(xyz_c_uv, color));
  glVertexAttribPointer(
      2, 2, GL_FLOAT, GL_FALSE, sizeof(fw::vertex::xyz_c_uv), OFFSET_OF(xyz_c_uv, u));
}

std::function<void()> xyz_c_uv::get_setup_function() {
  return &xyz_c_uv_setup;
}

void xyz_n_uv_setup() {
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(
      0, 3, GL_FLOAT, GL_FALSE, sizeof(fw::vertex::xyz_n_uv), OFFSET_OF(xyz_n_uv, x));
  glVertexAttribPointer(
      1, 3, GL_FLOAT, GL_FALSE, sizeof(fw::vertex::xyz_n_uv), OFFSET_OF(xyz_n_uv, nx));
  glVertexAttribPointer(
      2, 2, GL_FLOAT, GL_FALSE, sizeof(fw::vertex::xyz_n_uv), OFFSET_OF(xyz_n_uv, u));
}

std::function<void()> xyz_n_uv::get_setup_function() {
  return &xyz_n_uv_setup;
}

void xyz_n_setup() {
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(
      0, 3, GL_FLOAT, GL_FALSE, sizeof(fw::vertex::xyz_n), OFFSET_OF(xyz_n_uv, x));
  glVertexAttribPointer(
      1, 3, GL_FLOAT, GL_FALSE, sizeof(fw::vertex::xyz_n), OFFSET_OF(xyz_n_uv, nx));
}

std::function<void()> xyz_n::get_setup_function() {
  return &xyz_n_setup;
}

}
}
