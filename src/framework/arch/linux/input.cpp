//
// Copyright (c) 2008-2011, Dean Harding. All rights reserved.
//

#include <unordered_set>

#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/unordered_set.hpp>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <CEGUIWindow.h>
#include <CEGUISystem.h>
#include <CEGUIInputEvent.h>

#include <framework/input.h>
#include <framework/framework.h>
#include <framework/main_window.h>
#include <framework/misc.h>
#include <framework/exception.h>
#include <framework/logging.h>
#include <framework/settings.h>

// the XK_* values are used by X11 for keys on the keyboard, we'll borrow values
// greater than the maximum possible XK for the mouse buttons as well
#define XK_MBTNLEFT (0xffffff00 + 0)
#define XK_MBTNMIDDLE (0xffffff00 + 1)
#define XK_MBTNRIGHT (0xffffff00 + 2)

namespace fs = boost::filesystem;

CEGUI::MouseButton get_cegui_mousebutton(int button);
CEGUI::uint get_cegui_keycode(int key);

// in order to keep the header clean of platform-specific details, and also since keyboard/mouse input
// is inherently singlton anyway, we have a bunch of global variables here. Sorry.

typedef std::map<std::string, int, fw::iless<std::string> > key_names_map;
typedef std::map<int, std::string> key_codes_map;
static key_names_map g_key_names;
static key_codes_map g_key_codes;

static fw::input *g_instance = 0;

static boost::unordered_set<int> g_pressed_keys;

static volatile uint32_t g_next_token;

static float g_mx, g_my;
static float g_mdx, g_mdy;
static float g_mw, g_mdw;
static bool g_mbtns[3];
static bool g_mouse_inside;
static bool g_hide_cursor;

// we keep the current cursor in a "stack". When you set the cursor, you pass your
// "priority" and we display the one with the highest priority.
static std::map<int, std::string> g_cursor_stack;

//todo: cursors
//typedef std::map<std::string, HCURSOR> cursor_map;
//static cursor_map g_cursors;

// maps a key to a list of input bindings, we don't nessecarily call each binding on each
// key press, it depends on things like whether Ctrl or Shift is pressed, etc
typedef std::map<int, std::map<int, fw::input_binding> > callback_map;
static callback_map g_callbacks;

typedef std::map<int, std::vector<int> > multikey_binding_map;
static multikey_binding_map g_multikey_bindings;

namespace fw {

	// this is called by main_window when it gets a keyboard/mouse event. We need to fire off
	// the corresponding callbacks, inject it into CEGUI, etc
	void input_handle_event(XEvent &evnt);

	// sets up the key_names and key_codes which map string key "names" (that are platform-independent) with
	// keycodes (that are platform-specific)
	void setup_keynames();

	// binds an input_binding to the specified keycode (basically, adds it to the g_callbacks
	// collection)
	int bind_key(int keycode, input_binding const &binding);

	// determines whether the given key is currently pressed or not
	bool get_key_state(int keycode);

	// fires the callbacks associated when the specified key is pressed/released
	void callback(int key, bool is_down);

	input::input()
	{
		g_instance = this;

		if (g_key_names.size() == 0)
		{
			setup_keynames();
		}
	}

	input::~input()
	{
	}

	void input::set_cursor(int priority, std::string const &cursor_name)
	{
		if (cursor_name == "")
		{
			std::map<int, std::string>::iterator it = g_cursor_stack.find(priority);
			if (it != g_cursor_stack.end())
			{
				g_cursor_stack.erase(it);
			}
		}
		else
		{
			g_cursor_stack[priority] = cursor_name;
		}

		update_cursor();
	}

	void input::update_cursor()
	{
		std::map<int, std::string>::reverse_iterator cit = g_cursor_stack.rbegin();
		if (cit != g_cursor_stack.rend())
		{
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

	int input::bind_function(std::string const &name, input_bind_fn fn)
	{
		settings stg;
		return bind_key(get_value<std::string>(stg, "bind." + name), fn);
	}

	int input::bind_key(std::string const &keyname, input_bind_fn fn)
	{
		int token = static_cast<int>(__sync_add_and_fetch(&g_next_token, 1));

		std::vector<int> tokens;
		std::vector<std::string> keys = split<std::string>(keyname, ",");
		BOOST_FOREACH(std::string key, keys)
		{
			boost::trim(key);
			std::vector<std::string> parts = split<std::string>(key, "+");

			int key_no = 0;
			input_binding binding;
			BOOST_FOREACH(std::string part, parts)
			{
				boost::trim(part);
				if (boost::iequals(part, "ctrl"))
				{
					binding.ctrl = true;
				}
				else if (boost::iequals(part, "shift"))
				{
					binding.shift = true;
				}
				else if (boost::iequals(part, "alt"))
				{
					binding.alt = true;
				}
				else
				{
					key_names_map::iterator it = g_key_names.find(part);
					if (it == g_key_names.end())
					{
						BOOST_THROW_EXCEPTION(fw::exception() <<
							fw::message_error_info("No such key:" + part));
					}

					key_no = it->second;
				}
			}

			if (key_no == 0)
			{
				BOOST_THROW_EXCEPTION(fw::exception() <<
					fw::message_error_info("Invalid binding, no key name specified"));
			}

			binding.fn = fn;
			tokens.push_back(fw::bind_key(key_no, binding));
		}

		g_multikey_bindings[token] = tokens;
		return token;
	}

	int input::bind_key(std::string keyname, input_binding const &binding)
	{
		key_names_map::iterator it = g_key_names.find(keyname);
		if (it == g_key_names.end())
		{
			BOOST_THROW_EXCEPTION(fw::exception() <<
				fw::message_error_info("No such key:" + keyname));
		}

		return fw::bind_key(it->second, binding);
	}
	
	int bind_key(int keycode, input_binding const &binding)
	{
		int token = static_cast<int>(__sync_add_and_fetch(&g_next_token, 1));

		callback_map::iterator it = g_callbacks.find(keycode);
		if (it == g_callbacks.end())
		{
			g_callbacks[keycode] = std::map<int, input_binding>();
			it = g_callbacks.find(keycode);
		}

		std::map<int, input_binding> &callbacks = (*it).second;
		callbacks[token] = binding;

		return token;
	}

	void input::unbind_key(int token)
	{
		// it's either a multi-key binding token, or it's a "regular" token so we'll
		// have to unbind it as appropriate.
		multikey_binding_map::iterator it = g_multikey_bindings.find(token);
		if (it != g_multikey_bindings.end())
		{
			std::vector<int> &tokens = (*it).second;
			BOOST_FOREACH(int t, tokens)
			{
				unbind_key(t);
			}
		}
		else
		{
			BOOST_FOREACH(callback_map::value_type &callback_map, g_callbacks)
			{
				std::map<int, input_binding> &callbacks = callback_map.second;
				callbacks.erase(token);
			}
		}
	}

	void input::initialise()
	{
		set_cursor(0, "arrow");
	}

	void input::update(float dt)
	{
		// this is called each frame - we need to reset the _mdx and _mdy to zero (because if we
		// don't get an on_mouse_move, it means the mouse didn't move!)
		g_mdx = g_mdy = 0.0f;
		g_mdw = 0.0f;
	}

	void input::hide_cursor() { g_hide_cursor = true; }
	void input::show_cursor() { g_hide_cursor = false; }

	float input::mouse_x() const { return g_mx; }
	float input::mouse_y() const { return g_my; }
	float input::mouse_dx() const { return g_mdx; }
	float input::mouse_dy() const { return g_mdy; }
	float input::mouse_wheel() const { return g_mw; }
	float input::mouse_dwheel() const { return g_mdw; }
	bool input::mouse_left() const { return g_mbtns[0]; }
	bool input::mouse_middle() const { return g_mbtns[1]; }
	bool input::mouse_right() const { return g_mbtns[2]; }

	bool input::key(std::string keyname) const
	{
		return false;
	}

	//-------------------------------------------------------------------------

	input_binding::input_binding()
		: ctrl(false), shift(false), alt(false)
	{
	}

	input_binding::input_binding(input_bind_fn fn)
		: fn(fn), ctrl(false), shift(false), alt(false)
	{
	}

	input_binding::input_binding(input_binding const &copy)
		: fn(copy.fn), ctrl(copy.ctrl), shift(copy.shift), alt(copy.alt)
	{
	}

	input_binding::~input_binding()
	{
	}

	input_binding &input_binding::operator =(input_binding const &copy)
	{
		fn = copy.fn;
		ctrl = copy.ctrl;
		shift = copy.shift;
		alt = copy.alt;
		return *this;
	}

	//-------------------------------------------------------------------------

	void input_handle_event(XEvent &evnt)
	{
		if (evnt.type == KeyPress)
		{
			int keycode = XLookupKeysym(&evnt.xkey, 0);
			if (keycode == NoSymbol)
				return;

			g_pressed_keys.emplace(keycode);

			CEGUI::uint cegui_key = get_cegui_keycode(keycode);
			if (cegui_key == 0 || !CEGUI::System::getSingleton().injectKeyDown(cegui_key))
			{
				callback(keycode, true);
			}
		}
		else if (evnt.type == KeyRelease)
		{
			int keycode = XLookupKeysym(&evnt.xkey, 0);
			if (keycode == NoSymbol)
				return;

			g_pressed_keys.erase(keycode);

			CEGUI::uint cegui_key = get_cegui_keycode(keycode);
			if (cegui_key == 0 || !CEGUI::System::getSingleton().injectKeyUp(cegui_key))
			{
				callback(keycode, false);
			}

			// now check whether it corresponds to a written character and pass that to CEGUI
			// as well (since CEGUI differentiates between characters and buttons)
			char buffer[30] = {0};
			XLookupString(&evnt.xkey, buffer, sizeof(buffer), 0, 0);
			if (buffer[0] != 0)
			{
				// todo: utf-8 to utf-32...
				CEGUI::System::getSingleton().injectChar(static_cast<CEGUI::utf32>(buffer[0]));
			}
		}
		else if (evnt.type == KeymapNotify)
		{
			// this is called in the (somewhat unlikely) event that someone changes the keymapping
			XRefreshKeyboardMapping(&evnt.xmapping);
		}
		else if (evnt.type == MotionNotify)
		{
			g_mouse_inside = true;

			// save the current difference and the current position
			g_mdx = evnt.xmotion.x - g_mx;
			g_mdy = evnt.xmotion.y - g_my;
			g_mx = evnt.xmotion.x;
			g_my = evnt.xmotion.y;

			// if the cursor is supposed to be hidden, we'll just put it back to the center of the window
			if (g_hide_cursor)
			{
				main_window *wnd = framework::get_instance()->get_window();
				if (wnd != 0)
				{
					wnd->show_cursor(false);
/*
					POINT pt = { wnd->get_width() / 2, wnd->get_height() / 2 };
					::ClientToScreen(_hwnd, &pt);
					::SetCursorPos(pt.x, pt.y);
*/
				}
			}
			else
			{
				CEGUI::System::getSingleton().injectMousePosition(evnt.xmotion.x, evnt.xmotion.y);
			}
		}
		else if (evnt.type == ButtonPress)
		{
			int keycode = 0xffffff00 + (evnt.xbutton.button - 1);
			g_pressed_keys.emplace(keycode);
			
			CEGUI::MouseButton cegui_button = get_cegui_mousebutton(evnt.xbutton.button);
			if (cegui_button < 0 || !CEGUI::System::getSingleton().injectMouseButtonDown(cegui_button))
			{
				callback(keycode, true);
			}
		}
		else if (evnt.type == ButtonRelease)
		{
			int keycode = 0xffffff00 + (evnt.xbutton.button - 1);
			g_pressed_keys.erase(keycode);

			CEGUI::MouseButton cegui_button = get_cegui_mousebutton(evnt.xbutton.button);
			if (cegui_button < 0 || !CEGUI::System::getSingleton().injectMouseButtonUp(cegui_button))
			{
				callback(keycode, false);
			}
		}
	}

	void callback(int key, bool is_down)
	{
		callback_map::const_iterator it = g_callbacks.find(key);
		if (it != g_callbacks.end())
		{
			std::map<int, input_binding> const &list = (*it).second;
			// todo: can we use std::for_each? I couldn't get it to compile...
			for(std::map<int, input_binding>::const_iterator it = list.begin(); it != list.end(); ++it)
			{
				input_binding const &binding = it->second;

				// check that the correct ctrl/shift/alt combination is pressed as well
				if (binding.ctrl && !get_key_state(XK_Control_L) && !get_key_state(XK_Control_R))
					continue;
				if (binding.shift && !get_key_state(XK_Shift_L) && !get_key_state(XK_Shift_R))
					continue;
				if (binding.alt && !get_key_state(XK_Alt_L) && !get_key_state(XK_Alt_R))
					continue;

				binding.fn(g_key_codes[key], is_down);
			}
		}
	}

	bool get_key_state(int keycode)
	{
		return (g_pressed_keys.find(keycode) != g_pressed_keys.end());
	}

	// sets up the key_names map that maps string names to integer XK_* values that you'll find in X11/keysym.h
	void setup_keynames()
	{
		// start off with simple ones in a loop
		for(int key = static_cast<int>('A'); key <= static_cast<int>('Z'); key++)
		{
			char buff[2] = {static_cast<char>(key), 0};
			g_key_names[buff] = key;
		}
		for(int key = static_cast<int>('0'); key <= static_cast<int>('9'); key++)
		{
			char buff[2] = {static_cast<char>(key), 0};
			g_key_names[buff] = key;
		}
		for(int key = 0; key <= 9; key++)
		{
			std::string name = std::string("NP-") + boost::lexical_cast<std::string>(key);
			g_key_names[name] = XK_KP_0 + key;
		}
		for(int key = 0; key < 12; key++)
		{
			std::string name = "F" + boost::lexical_cast<std::string>(key + 1);
			g_key_names[name] = XK_F1 + key;
		}

		g_key_names["Left-Mouse"] = XK_MBTNLEFT;
		g_key_names["Middle-Mouse"] = XK_MBTNMIDDLE;
		g_key_names["Right-Mouse"] = XK_MBTNRIGHT;

		g_key_names["Tab"] = XK_Tab;
		g_key_names["Return"] = XK_Return;
		g_key_names["Enter"] = XK_Return;
		g_key_names["Esc"] = XK_Escape;
		g_key_names["Space"] = XK_space;
		g_key_names["PgUp"] = XK_Page_Up;
		g_key_names["PgDown"] = XK_Page_Down;
		g_key_names["End"] = XK_End;
		g_key_names["Home"] = XK_Home;
		g_key_names["Del"] = XK_Delete;
		g_key_names["Ins"] = XK_Insert;

		g_key_names["Left"] = XK_Left;
		g_key_names["Right"] = XK_Right;
		g_key_names["Up"] = XK_Up;
		g_key_names["Down"] = XK_Down;

		// technically OEM keys, these are standard on all keyboards (apparently)
		g_key_names["Comma"] = XK_comma;
		g_key_names["Period"] = XK_period;
		g_key_names["Dot"] = XK_period;
		g_key_names["Plus"] = XK_plus;
		g_key_names["Minus"] = XK_minus;

		// these are US-keyboard only entries, corresponding "OEM-n" values are there
		// as well
		g_key_names["Colon"] = XK_colon;
		g_key_names["Slash"] = XK_slash;
		g_key_names["~"] = XK_asciitilde;
		g_key_names["["] = XK_bracketleft;
		g_key_names["Backslash"] = XK_backslash;
		g_key_names["]"] = XK_bracketright;
		g_key_names["Quote"] = XK_quotedbl;

		// now construct the reverse mapping, which is pretty easy...
		BOOST_FOREACH(key_names_map::value_type &key, g_key_names)
		{
			g_key_codes[key.second] = key.first;
		}
	}

}

// Converts our internal mouse button numbers to CEGUI mouse buttons.
CEGUI::MouseButton get_cegui_mousebutton(int button)
{
	if (button == Button1)
		return CEGUI::LeftButton;
	else if (button == Button2)
		return CEGUI::MiddleButton;
	else if (button == Button3)
		return CEGUI::RightButton;

	return static_cast<CEGUI::MouseButton>(-1);
}

// Converts VK_xxx style scan codes to the ones used by CEGUI. There might be more
// that are useful, but this should cover the majority...
CEGUI::uint get_cegui_keycode(int keysym)
{
	switch(keysym)
	{
	case XK_Left:
		return CEGUI::Key::ArrowLeft;
	case XK_Right:
		return CEGUI::Key::ArrowRight;
	case XK_Up:
		return CEGUI::Key::ArrowUp;
	case XK_Down:
		return CEGUI::Key::ArrowDown;
	case XK_Delete:
		return CEGUI::Key::Delete;
	case XK_BackSpace:
		return CEGUI::Key::Backspace;
	case XK_Escape:
		return CEGUI::Key::Escape;
	case XK_Tab:
		return CEGUI::Key::Tab;
	case XK_Shift_L:
		return CEGUI::Key::LeftShift;
	case XK_Shift_R:
		return CEGUI::Key::RightShift;
	case XK_Control_L:
		return CEGUI::Key::LeftControl;
	case XK_Control_R:
		return CEGUI::Key::RightControl;
	}

	return 0;
}
