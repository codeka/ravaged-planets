
#include <boost/foreach.hpp>

#include <SDL.h>

#include <framework/bitmap.h>
#include <framework/cursor.h>
#include <framework/exception.h>
#include <framework/framework.h>
#include <framework/graphics.h>
#include <framework/paths.h>

namespace fw {

cursor::cursor() : _cursor_visible(true) {
}

cursor::~cursor() {
}

void cursor::initialize() {
  set_cursor(0, "arrow");
}

void cursor::destroy() {
  BOOST_FOREACH(auto it, _loaded_cursors) {
    SDL_FreeCursor(it.second);
  }
  _loaded_cursors.clear();
}

SDL_Cursor *cursor::load_cursor(std::string const &name) {
  fw::bitmap bmp(fw::resolve("cursors/" + name + ".png"));

  int hot_x = bmp.get_width() / 2;
  int hot_y = bmp.get_height() / 2;
  if (name == "arrow") {
    // TODO: hard coding this seems a bit... flaky.
    hot_x = hot_y = 0;
  }

  // Well this is a bit annoying.
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    int rmask = 0xff000000;
    int gmask = 0x00ff0000;
    int bmask = 0x0000ff00;
    int amask = 0x000000ff;
#else
    int rmask = 0x000000ff;
    int gmask = 0x0000ff00;
    int bmask = 0x00ff0000;
    int amask = 0xff000000;
#endif

  SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(
      const_cast<void *>(reinterpret_cast<void const *>(bmp.get_pixels().data())),
      bmp.get_width(), bmp.get_height(), 32, bmp.get_width() * 4, rmask, gmask, bmask, amask);
  SDL_Cursor *cursor = SDL_CreateColorCursor(surface, hot_x, hot_y);
  SDL_FreeSurface(surface);
  return cursor;
}

void cursor::set_cursor_for_real(std::string const &name) {
  SDL_Cursor *cursor;
  auto it = _loaded_cursors.find(name);
  if (it == _loaded_cursors.end()) {
    cursor = load_cursor(name);
    _loaded_cursors[name] = cursor;
  } else {
    cursor = it->second;
  }

  SDL_SetCursor(cursor);
}

void cursor::update_cursor() {
  std::map<int, std::string>::reverse_iterator cit = _cursor_stack.rbegin();
  if (cit != _cursor_stack.rend()) {
     std::string cursor_name = cit->second;
     set_cursor_for_real(cursor_name);
  }
}

void cursor::set_cursor(int priority, std::string const &name) {
  if (name == "") {
    auto it = _cursor_stack.find(priority);
    if (it != _cursor_stack.end()) {
      _cursor_stack.erase(it);
    }
  } else {
    _cursor_stack[priority] = name;
  }

  update_cursor();
}

void cursor::set_visible(bool is_visible) {
  SDL_ShowCursor(is_visible ? 1 : 0);
  _cursor_visible = is_visible;
}

bool cursor::is_visible() {
  return _cursor_visible;
}

}
