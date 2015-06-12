#include <unordered_set>

#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/unordered_set.hpp>

#include <SDL.h>

#include <framework/gui/gui.h>
#include <framework/input.h>
#include <framework/framework.h>
#include <framework/misc.h>
#include <framework/exception.h>
#include <framework/logging.h>
#include <framework/settings.h>

// the SDLK_* values are used by SDL for keys on the keyboard, we'll borrow values
// greater than the maximum possible SDLK for the mouse buttons as well
#define KEY_MBTNLEFT (0xffffff00 + 0)
#define KEY_MBTNMIDDLE (0xffffff00 + 1)
#define KEY_MBTNRIGHT (0xffffff00 + 2)

namespace fs = boost::filesystem;

// in order to keep the header clean of platform-specific details, and also since keyboard/mouse input
// is inherently singleton anyway, we have a bunch of global variables here. Sorry.

typedef std::map<std::string, int, fw::iless<std::string> > key_names_map;
typedef std::map<int, std::string> key_codes_map;
static key_names_map g_key_names;
static key_codes_map g_key_codes;

static volatile uint32_t g_next_token;

static float g_mx, g_my;
static float g_mdx, g_mdy;
static float g_mw, g_mdw;
static bool g_mouse_inside;
static bool g_hide_cursor;

// we keep the current cursor in a "stack". When you set the cursor, you pass your
// "priority" and we display the one with the highest priority.
static std::map<int, std::string> g_cursor_stack;

//todo: cursors
//typedef std::map<std::string, HCURSOR> cursor_map;
//static cursor_map g_cursors;

// maps a key to a list of input bindings, we don't necessarily call each binding on each
// key press, it depends on things like whether Ctrl or Shift is pressed, etc
typedef std::map<int, std::map<int, fw::input_binding> > callback_map;
static callback_map g_callbacks;

typedef std::map<int, std::vector<int> > multikey_binding_map;
static multikey_binding_map g_multikey_bindings;

namespace fw {

// sets up the key_names and key_codes which map string key "names" (that are platform-independent) with
// keycodes (that are platform-specific)
void setup_keynames();

// binds an input_binding to the specified keycode (basically, adds it to the g_callbacks
// collection)
int bind_key(int keycode, input_binding const &binding);

// fires the callbacks associated when the specified key is pressed/released
void callback(int key, Uint16 mod, bool is_down);

input::input() {
  if (g_key_names.size() == 0) {
    setup_keynames();
  }
}

input::~input() {
}

void input::set_cursor(int priority, std::string const &cursor_name) {
  if (cursor_name == "") {
    std::map<int, std::string>::iterator it = g_cursor_stack.find(priority);
    if (it != g_cursor_stack.end()) {
      g_cursor_stack.erase(it);
    }
  } else {
    g_cursor_stack[priority] = cursor_name;
  }

  update_cursor();
}

void input::update_cursor() {
  std::map<int, std::string>::reverse_iterator cit = g_cursor_stack.rbegin();
  if (cit != g_cursor_stack.rend()) {
    /*
     std::string cursor_name = cit->second;
     cursor_map::iterator it = g_cursors.find(cursor_name);
     if (it == g_cursors.end())
     {
     fs::path full_path = fs::initial_path() / "data/cursors" / (cursor_name + ".cur");
     std::wstring wfull_path = to_unicode(full_path.file_string());

     fw::debug << boost::format("loading cursor: \"%1%\"") % full_path << std::endl;
     HCURSOR hcur = ::LoadCursorFromFile(wfull_path.c_str());
     if (hcur == 0)
     {
     BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info("Could not load cursor: " + full_path.string()));
     }

     _cursors[cursor_name] = hcur;
     it = _cursors.find(cursor_name);
     }

     fw::framework::get_instance()->get_window()->set_cursor(it->second);
     */
  }
}

int input::bind_function(std::string const &name, input_bind_fn fn) {
  settings stg;
  return bind_key(stg.get_value<std::string>("bind." + name), fn);
}

int input::bind_key(std::string const &keyname, input_bind_fn fn) {
  int token = static_cast<int>(__sync_add_and_fetch(&g_next_token, 1));

  std::vector<int> tokens;
  std::vector<std::string> keys = split<std::string>(keyname, ",");
  BOOST_FOREACH(std::string key, keys) {
    boost::trim(key);
    std::vector<std::string> parts = split<std::string>(key, "+");

    int key_no = 0;
    input_binding binding;
    BOOST_FOREACH(std::string part, parts) {
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
              fw::exception() << fw::message_error_info("No such key:" + part));
        }

        key_no = it->second;
      }
    }

    if (key_no == 0) {
      BOOST_THROW_EXCEPTION(fw::exception()
          << fw::message_error_info("Invalid binding, no key name specified"));
    }

    binding.fn = fn;
    tokens.push_back(fw::bind_key(key_no, binding));
  }

  g_multikey_bindings[token] = tokens;
  return token;
}

int input::bind_key(std::string keyname, input_binding const &binding) {
  key_names_map::iterator it = g_key_names.find(keyname);
  if (it == g_key_names.end()) {
    BOOST_THROW_EXCEPTION(
        fw::exception() << fw::message_error_info("No such key:" + keyname));
  }

  return fw::bind_key(it->second, binding);
}

int bind_key(int keycode, input_binding const &binding) {
  int token = static_cast<int>(__sync_add_and_fetch(&g_next_token, 1));

  callback_map::iterator it = g_callbacks.find(keycode);
  if (it == g_callbacks.end()) {
    g_callbacks[keycode] = std::map<int, input_binding>();
    it = g_callbacks.find(keycode);
  }

  std::map<int, input_binding> &callbacks = (*it).second;
  callbacks[token] = binding;

  return token;
}

void input::unbind_key(int token) {
  // it's either a multi-key binding token, or it's a "regular" token so we'll
  // have to unbind it as appropriate.
  multikey_binding_map::iterator it = g_multikey_bindings.find(token);
  if (it != g_multikey_bindings.end()) {
    std::vector<int> &tokens = (*it).second;
    BOOST_FOREACH(int t, tokens) {
      unbind_key(t);
    }
  } else {
    BOOST_FOREACH(callback_map::value_type &callback_map, g_callbacks) {
      std::map<int, input_binding> &callbacks = callback_map.second;
      callbacks.erase(token);
    }
  }
}

void input::initialize() {
  set_cursor(0, "arrow");
}

void input::update(float dt) {
  // this is called each frame - we need to reset the _mdx and _mdy to zero (because if we
  // don't get an on_mouse_move, it means the mouse didn't move!)
  g_mdx = g_mdy = 0.0f;
  g_mdw = 0.0f;

  std::unique_lock<std::mutex> lock(_pending_events_mutex);
  BOOST_FOREACH(input_event event, _pending_events) {
    if (event.type == SDL_KEYDOWN) {
      callback(event.key_code, event.key_mod, true);
    } else if (event.type == SDL_KEYUP) {
      callback(event.key_code, event.key_mod, false);
    } else if (event.type == SDL_TEXTINPUT) {
      // TODO
    } else if (event.type == SDL_MOUSEMOTION) {
      g_mouse_inside = true;

      // save the current difference and the current position
      g_mdx =  event.dx - g_mx;
      g_mdy = event.dy - g_my;
      g_mx = event.dx;
      g_my = event.dy;

      // if the cursor is supposed to be hidden, we'll just put it back to the center of the window
      if (g_hide_cursor) {
  //      main_window *wnd = framework::get_instance()->get_window();
  //      if (wnd != 0) {
  //        wnd->show_cursor(false);
          /*
           POINT pt = { wnd->get_width() / 2, wnd->get_height() / 2 };
           ::ClientToScreen(_hwnd, &pt);
           ::SetCursorPos(pt.x, pt.y);
           */
  //      }
      } else {
  //      CEGUI::System::getSingleton().injectMousePosition(evnt.xmotion.x,
  //          evnt.xmotion.y);
      }
    } else if (event.type == SDL_MOUSEBUTTONDOWN) {
      int keycode = 0xffffff00 + (event.button - 1);
      if (!fw::framework::get_instance()->get_gui()->inject_mouse(event.button, true)) {
        callback(keycode, 0, true);
      }
    } else if (event.type == SDL_MOUSEBUTTONUP) {
      int keycode = 0xffffff00 + (event.button - 1);
      if (!fw::framework::get_instance()->get_gui()->inject_mouse(event.button, false)) {
        callback(keycode, 0, false);
      }
    }
  }
  _pending_events.clear();
}

/** Called on the render thread when an event is received. We queue it up to actually run on the update thread. */
void input::process_event(SDL_Event &event) {
  std::unique_lock<std::mutex> lock(_pending_events_mutex);

  input_event ie;
  ie.type = event.type;
  if (event.type == SDL_KEYDOWN) {
    ie.key_code = static_cast<int>(event.key.keysym.sym);
    ie.key_mod = event.key.keysym.mod;
  } else if (event.type == SDL_KEYUP) {
    ie.key_code = static_cast<int>(event.key.keysym.sym);
    ie.key_mod = event.key.keysym.mod;
  } else if (event.type == SDL_TEXTINPUT) {
    // TODO
  } else if (event.type == SDL_MOUSEMOTION) {
    ie.dx = event.motion.x;
    ie.dy = event.motion.y;
  } else if (event.type == SDL_MOUSEBUTTONDOWN) {
    ie.button = event.button.button;
  } else if (event.type == SDL_MOUSEBUTTONUP) {
    ie.button = event.button.button;
  } else {
    // Unknown event, we want to ignore it.
    return;
  }
  _pending_events.push_back(ie);
}

void input::hide_cursor() {
  g_hide_cursor = true;
}
void input::show_cursor() {
  g_hide_cursor = false;
}

float input::mouse_x() const {
  return g_mx;
}
float input::mouse_y() const {
  return g_my;
}
float input::mouse_dx() const {
  return g_mdx;
}
float input::mouse_dy() const {
  return g_mdy;
}
float input::mouse_wheel() const {
  return g_mw;
}
float input::mouse_dwheel() const {
  return g_mdw;
}

bool input::key(std::string keyname) const {
  return false;
}

//-------------------------------------------------------------------------

input_binding::input_binding() :
    ctrl(false), shift(false), alt(false) {
}

input_binding::input_binding(input_bind_fn fn) :
    fn(fn), ctrl(false), shift(false), alt(false) {
}

input_binding::input_binding(input_binding const &copy) :
    fn(copy.fn), ctrl(copy.ctrl), shift(copy.shift), alt(copy.alt) {
}

input_binding::~input_binding() {
}

input_binding &input_binding::operator =(input_binding const &copy) {
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
    std::map<int, input_binding> const &list = (*it).second;
    for (std::map<int, input_binding>::const_iterator it = list.begin();
        it != list.end(); ++it) {
      input_binding const &binding = it->second;

      // check that the correct ctrl/shift/alt combination is pressed as well
      if (binding.ctrl && (mod & KMOD_CTRL) != 0)
        continue;
      if (binding.shift && (mod & KMOD_SHIFT) != 0)
        continue;
      if (binding.alt && (mod & KMOD_ALT) != 0)
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
  BOOST_FOREACH(key_names_map::value_type &key, g_key_names) {
    g_key_codes[key.second] = key.first;
  }
}

}
