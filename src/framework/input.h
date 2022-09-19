#pragma once

#include <string>
#include <functional>
#include <mutex>
#include <boost/algorithm/string.hpp>

union SDL_Event;

// the SDLK_* values are used by SDL for keys on the keyboard, we'll borrow values
// greater than the maximum possible SDLK for the mouse buttons as well
#define KEY_MBTNLEFT (0xffffff00 + 0)
#define KEY_MBTNMIDDLE (0xffffff00 + 1)
#define KEY_MBTNRIGHT (0xffffff00 + 2)

namespace fw {

typedef std::function<void(std::string keyname, bool is_down)> input_bind_fn;

// this is the details we want about a given key binding
struct input_binding {
  input_bind_fn fn;
  bool ctrl;  // only fire if CTRL is down
  bool shift; // only fire if SHIFT is down
  bool alt;   // only fire if ALT is down

  input_binding();
  input_binding(input_bind_fn fn);
  input_binding(input_binding const &copy);
  ~input_binding();

  input_binding &operator =(input_binding const &copy);
};

class input {
private:
  void update_cursor();

public:
  input();
  ~input();

  void initialize();
  void update(float dt);
  void process_event(SDL_Event &event);
  void release();

  // bind the specified function to be called when the given key (or mouse button)
  // is pressed/released. When you call bind_key, you get back an integer token
  // which you can later pass to unbind_key to unbind that key/function. It's a bit
  // annoying that you can't compare boost::function<> objects, which is why this
  // "token" system is required.
  int bind_key(std::string keyname, input_binding const &binding);
  void unbind_key(int token);

  // binds the specified name key (or keys, separated by a comma) to the given callback
  // this is useful for use in the configuration where you can let the user define the
  // keys to bind to each function. This version will parse things like "Ctrl+A" and
  // construct an appropriate input_binding structure.
  int bind_key(std::string const &keyname, input_bind_fn fn);

  // binds the specified "function name" (basically, a "bind.function" in the config file)
  // to the specified method
  int bind_function(std::string const &keyname, input_bind_fn fn);

  float mouse_x() const;
  float mouse_y() const;
  float mouse_dx() const;
  float mouse_dy() const;
  float mouse_wheel_dx() const;
  float mouse_wheel_dy() const;
  bool key(std::string keyname) const;
};
}
