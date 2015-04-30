#include <string>
#include <list>

#include <SDL.h>

#include <framework/exception.h>
#include <framework/graphics.h>
#include <framework/settings.h>
#include <framework/logging.h>

namespace fw {

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

  _width = 640;
  _height = 480;
  _windowed = stg.is_set("windowed");
  fw::debug << "Graphics initializing; window size=" << _width << "x" << _height << ", windowed=" << _windowed
      << std::endl;

  SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
  SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1 );
  SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );

  _wnd = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480,
      SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
  if (_wnd == nullptr) {
    BOOST_THROW_EXCEPTION(fw::exception() << fw::sdl_error_info(SDL_GetError()));
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

  if(SDL_GL_SetSwapInterval(1) < 0) {
    fw::debug << "Unable to set vsync, vsync is disabled. error: " << SDL_GetError() << std::endl;
  }

  fw::debug << "GLEW version: " << glewGetString(GLEW_VERSION) << std::endl;
  fw::debug << "GL vendor: " << glGetString(GL_VENDOR) << std::endl;
  fw::debug << "GL renderer: " << glGetString(GL_RENDERER) << std::endl;
  fw::debug << "GL version: " << glGetString(GL_VERSION) << std::endl;
  fw::debug << "shading language version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

  glEnable(GL_DEPTH_TEST);
  glViewport(0, 0, _width, _height);
}

void graphics::destroy() {
  SDL_GL_DeleteContext(_context);
  _context = nullptr;
}

bool graphics::poll_events() {
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    if (e.type == SDL_QUIT) {
      return false;
    }
  }
  return true;
}

void graphics::begin_scene(fw::colour clear_colour /*= fw::colour(1,0,0,0)*/) {
  glClearColor(static_cast<float>(clear_colour.r),
      static_cast<float>(clear_colour.g), static_cast<float>(clear_colour.b),
      static_cast<float>(clear_colour.a));
  glClear(GL_COLOR_BUFFER_BIT);
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

  SDL_GL_SwapWindow(_wnd);

  GLenum err = glGetError();
  if (err != GL_NO_ERROR) {
    // this might happen in Release mode, where we don't actually check errors on every single call (it's expensive).
    BOOST_THROW_EXCEPTION(fw::exception() << fw::gl_error_info(err));
  }
}

void graphics::set_render_target(texture *tex) {
}

void graphics::check_error(char const *msg) {
  GLenum err = glGetError();
  if (err == GL_NO_ERROR)
    return;

  BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info(msg) << fw::gl_error_info(err));
}

}
