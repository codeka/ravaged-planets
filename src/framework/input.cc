#include <unordered_set>

#include <boost/filesystem.hpp>
#include <boost/unordered_set.hpp>

#include <SDL2/SDL.h>

#include <framework/gui/gui.h>
#include <framework/input.h>
#include <framework/framework.h>
#include <framework/misc.h>
#include <framework/exception.h>
#include <framework/logging.h>
#include <framework/settings.h>

namespace fs = boost::filesystem;

// in order to keep the header clean of platform-specific details, and also since keyboard/mouse Input
// is inherently singleton anyway, we have a bunch of global variables here. Sorry.

typedef std::map<std::string, int, fw::iless<std::string>> key_names_map;
typedef std::map<int, std::string> key_codes_map;
static key_names_map g_key_names;
static key_codes_map g_key_codes;

static volatile uint32_t g_next_token;

static float g_mx, g_my;
static float g_mdx, g_mdy;
static float g_mwdx, g_mwdy;
static bool g_mouse_inside;

// maps a key to a list of Input bindings, we don't necessarily call each binding on each
// key press, it depends on things like whether Ctrl or Shift is pressed, etc
typedef std::map<int, std::map<int, fw::InputBinding> > callback_map;
static callback_map g_callbacks;

typedef std::map<int, std::vector<int> > multikey_binding_map;
static multikey_binding_map g_multikey_bindings;

namespace fw {

// sets up the key_names and key_codes which map string key "names" (that are platform-independent) with
// keycodes (that are platform-specific)
void setup_keynames();

// binds an InputBinding to the specified keycode (basically, adds it to the g_callbacks
// collection)
int bind_key(int keycode, InputBinding const &binding);

// fires the callbacks associated when the specified key is pressed/released
void callback(int key, Uint16 mod, bool is_down);

Input::Input() {
  if (g_key_names.size() == 0) {
    setup_keynames();
  }
}

Input::~Input() {
}

int Input::bind_function(std::string const &name, input_bind_fn fn) {
  Settings stg;
  return bind_key(stg.get_value<std::string>("bind." + name), fn);
}

int Input::bind_key(std::string const &keyname, input_bind_fn fn) {
  // TODO: interlocked increment?
  int token = ++g_next_token;

  std::vector<int> tokens;
  std::vector<std::string> keys = split<std::string>(keyname, ",");
  for(std::string key : keys) {
    boost::trim(key);
    std::vector<std::string> parts = split<std::string>(key, "+");

    int key_no = 0;
    InputBinding binding;
    for(std::string part : parts) {
      boost::trim(part);
      if (boost::iequals(part, "ctrl")) {
        binding.ctrl = true;
      } else if (boost::iequals(part, "shift")) {
        binding.shift = true;
      } else if (boost::iequals(part, "alt")) {
        binding.alt = true;
      } else {
        key_names_map::iterator it = g_key_names.find(part);
        if (it == g_key_names.end()) {
          BOOST_THROW_EXCEPTION(
              fw::Exception() << fw::message_error_info("No such key:" + part));
        }

        key_no = it->second;
      }
    }

    if (key_no == 0) {
      BOOST_THROW_EXCEPTION(fw::Exception()
          << fw::message_error_info("Invalid binding, no key name specified"));
    }

    binding.fn = fn;
    tokens.push_back(fw::bind_key(key_no, binding));
  }

  g_multikey_bindings[token] = tokens;
  return token;
}

int Input::bind_key(std::string keyname, InputBinding const &binding) {
  key_names_map::iterator it = g_key_names.find(keyname);
  if (it == g_key_names.end()) {
    BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info("No such key:" + keyname));
  }

  return fw::bind_key(it->second, binding);
}

int bind_key(int keycode, InputBinding const &binding) {
  // TODO: interlocked increment?
  int token = ++g_next_token;

  callback_map::iterator it = g_callbacks.find(keycode);
  if (it == g_callbacks.end()) {
    g_callbacks[keycode] = std::map<int, InputBinding>();
    it = g_callbacks.find(keycode);
  }

  std::map<int, InputBinding> &callbacks = (*it).second;
  callbacks[token] = binding;

  return token;
}

void Input::unbind_key(int token) {
  // it's either a multi-key binding token, or it's a "regular" token so we'll
  // have to unbind it as appropriate.
  multikey_binding_map::iterator it = g_multikey_bindings.find(token);
  if (it != g_multikey_bindings.end()) {
    std::vector<int> &tokens = (*it).second;
    for(int t : tokens) {
      unbind_key(t);
    }
  } else {
    for(auto &callback_map : g_callbacks) {
      std::map<int, InputBinding> &callbacks = callback_map.second;
      callbacks.erase(token);
    }
  }
}

void Input::initialize() {
}

void Input::update(float dt) {
  std::vector<SDL_Event> pending_events;
  {
    std::unique_lock lock(queued_events_mutex_);
    std::swap(pending_events, queued_events_);
  }

  bool mouse_moved = false;
  bool mouse_wheel_moved = false;
  fw::gui::Gui* gui = fw::Framework::get_instance()->get_gui();
  for (auto& event : pending_events) {
    if (event.type == SDL_KEYDOWN) {
      if (!gui->inject_key(static_cast<int>(event.key.keysym.sym), true)) {
        callback(static_cast<int>(event.key.keysym.sym), event.key.keysym.mod, true);
      }
    } else if (event.type == SDL_KEYUP) {
      if (!gui->inject_key(static_cast<int>(event.key.keysym.sym), false)) {
        callback(static_cast<int>(event.key.keysym.sym), event.key.keysym.mod, false);
      }
    } else if (event.type == SDL_TEXTINPUT) {
      // TODO
    } else if (event.type == SDL_MOUSEMOTION) {
      g_mouse_inside = true;

      // save the current difference and the current position
      g_mdx = event.motion.x - g_mx;
      g_mdy = event.motion.y - g_my;
      g_mx = event.motion.x;
      g_my = event.motion.y;
      mouse_moved = true;
    } else if (event.type == SDL_MOUSEBUTTONDOWN) {
      int keycode = 0xffffff00 + (event.button.button - 1);
      if (!gui->inject_mouse(event.button.button, true, g_mx, g_my)) {
        callback(keycode, 0, true);
      }
    } else if (event.type == SDL_MOUSEBUTTONUP) {
      int keycode = 0xffffff00 + (event.button.button - 1);
      if (!gui->inject_mouse(event.button.button, false, g_mx, g_my)) {
        callback(keycode, 0, false);
      }
    } else if (event.type == SDL_MOUSEWHEEL) {
      g_mwdx = event.wheel.x;
      g_mwdy = event.wheel.y;
      mouse_wheel_moved = true;
    }
  }

  if (!mouse_moved) {
    g_mdx = g_mdy = 0.0f;
  }
  if (!mouse_wheel_moved) {
    g_mwdx = g_mwdy = 0.0f;
  }
}

// Called on the render thread when an event is received. We queue it up to actually run on the update thread.
void Input::process_event(SDL_Event &event) {
  std::unique_lock lock(queued_events_mutex_);
  queued_events_.push_back(event);
}

float Input::mouse_x() const {
  return g_mx;
}
float Input::mouse_y() const {
  return g_my;
}
float Input::mouse_dx() const {
  return g_mdx;
}
float Input::mouse_dy() const {
  return g_mdy;
}
float Input::mouse_wheel_dx() const {
  return g_mwdx;
}
float Input::mouse_wheel_dy() const {
  return g_mwdy;
}

bool Input::key(std::string keyname) const {
  return false;
}

//-------------------------------------------------------------------------

InputBinding::InputBinding() :
    ctrl(false), shift(false), alt(false) {
}

InputBinding::InputBinding(input_bind_fn fn) :
    fn(fn), ctrl(false), shift(false), alt(false) {
}

InputBinding::InputBinding(InputBinding const &copy) :
    fn(copy.fn), ctrl(copy.ctrl), shift(copy.shift), alt(copy.alt) {
}

InputBinding::~InputBinding() {
}

InputBinding &InputBinding::operator =(InputBinding const &copy) {
  fn = copy.fn;
  ctrl = copy.ctrl;
  shift = copy.shift;
  alt = copy.alt;
  return *this;
}

//-------------------------------------------------------------------------

void callback(int key, Uint16 mod, bool is_down) {
  callback_map::const_iterator it = g_callbacks.find(key);
  if (it != g_callbacks.end()) {
    std::map<int, InputBinding> const &list = (*it).second;
    for (std::map<int, InputBinding>::const_iterator it = list.begin();
        it != list.end(); ++it) {
      InputBinding const &binding = it->second;

      // check that the correct ctrl/shift/alt combination is pressed as well
      if (binding.ctrl && (mod & KMOD_CTRL) == 0)
        continue;
      if (binding.shift && (mod & KMOD_SHIFT) == 0)
        continue;
      if (binding.alt && (mod & KMOD_ALT) == 0)
        continue;

      binding.fn(g_key_codes[key], is_down);
    }
  }
}

// sets up the key_names map that maps string names to integer XKSLDK_* values that you'll find in SLD_keycode.h
void setup_keynames() {
  // start off with simple ones in a loop
  for (int key = static_cast<int>('a'); key <= static_cast<int>('z'); key++) {
    char buff[2] = { static_cast<char>(key), 0 };
    g_key_names[buff] = key;
  }
  for (int key = static_cast<int>('0'); key <= static_cast<int>('9'); key++) {
    char buff[2] = { static_cast<char>(key), 0 };
    g_key_names[buff] = key;
  }
  for (int key = 0; key <= 9; key++) {
    std::string name = std::string("NP-")
        + boost::lexical_cast<std::string>(key);
    g_key_names[name] = SDLK_KP_0 + key;
  }
  for (int key = 0; key < 12; key++) {
    std::string name = "F" + boost::lexical_cast<std::string>(key + 1);
    g_key_names[name] = SDLK_F1 + key;
  }

  g_key_names["Left-Mouse"] = KEY_MBTNLEFT;
  g_key_names["Middle-Mouse"] = KEY_MBTNMIDDLE;
  g_key_names["Right-Mouse"] = KEY_MBTNRIGHT;

  g_key_names["Tab"] = SDLK_TAB;
  g_key_names["Return"] = SDLK_RETURN;
  g_key_names["Enter"] = SDLK_RETURN;
  g_key_names["Esc"] = SDLK_ESCAPE;
  g_key_names["Space"] = SDLK_SPACE;
  g_key_names["PgUp"] = SDLK_PAGEUP;
  g_key_names["PgDown"] = SDLK_PAGEDOWN;
  g_key_names["End"] = SDLK_END;
  g_key_names["Home"] = SDLK_HOME;
  g_key_names["Del"] = SDLK_DELETE;
  g_key_names["Ins"] = SDLK_INSERT;

  g_key_names["Left"] = SDLK_LEFT;
  g_key_names["Right"] = SDLK_RIGHT;
  g_key_names["Up"] = SDLK_UP;
  g_key_names["Down"] = SDLK_DOWN;

  // technically OEM keys, these are standard on all keyboards (apparently)
  g_key_names["Comma"] = SDLK_COMMA;
  g_key_names["Period"] = SDLK_PERIOD;
  g_key_names["Dot"] = SDLK_PERIOD;
  g_key_names["Plus"] = SDLK_EQUALS;
  g_key_names["Minus"] = SDLK_MINUS;

  // these are US-keyboard only entries, corresponding "OEM-n" values are there
  // as well
  g_key_names["Colon"] = SDLK_COLON;
  g_key_names["Slash"] = SDLK_SLASH;
  g_key_names["~"] = SDLK_BACKQUOTE;
  g_key_names["["] = SDLK_LEFTBRACKET;
  g_key_names["Backslash"] = SDLK_BACKSLASH;
  g_key_names["]"] = SDLK_RIGHTBRACKET;
  g_key_names["Quote"] = SDLK_QUOTEDBL;

  // now construct the reverse mapping, which is pretty easy...
  for(auto &key : g_key_names) {
    g_key_codes[key.second] = key.first;
  }
}

}
