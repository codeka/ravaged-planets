
#include <boost/locale.hpp>

#include <SDL2/SDL.h>

#include <framework/framework.h>
#include <framework/gui/drawable.h>
#include <framework/gui/gui.h>
#include <framework/gui/textedit.h>
#include <framework/logging.h>
#include <framework/font.h>

namespace conv = boost::locale::conv;

#define STB_TEXTEDIT_CHARTYPE uint32_t
#define STB_TEXTEDIT_POSITIONTYPE int
#define STB_TEXTEDIT_UNDOSTATECOUNT 99
#define STB_TEXTEDIT_UNDOCHARCOUNT 999
#include <stb/stb_textedit.h>

//-----------------------------------------------------------------------------

class textedit_buffer {
public:
  STB_TexteditState state;
  std::shared_ptr<fw::font_face> font;
  std::basic_string<uint32_t> codepoints;

  inline textedit_buffer() {
    font = fw::framework::get_instance()->get_font_manager()->get_face();
  }
};

//-----------------------------------------------------------------------------

#define STB_TEXTEDIT_IMPLEMENTATION

#define STB_TEXTEDIT_STRING textedit_buffer
#define STB_TEXTEDIT_NEWLINE '\n'

int STB_TEXTEDIT_STRINGLEN(textedit_buffer *buffer) {
  return buffer->codepoints.size();
}

char STB_TEXTEDIT_GETCHAR(const textedit_buffer *buffer, int idx) {
  return buffer->codepoints[idx];
}

float STB_TEXTEDIT_GETWIDTH(textedit_buffer *buffer, int line_start_idx, int char_idx) {
  return buffer->font->measure_glyph(buffer->codepoints[char_idx])[0];
}

int STB_TEXTEDIT_KEYTOTEXT(int key) {
  if (key >= 0x10000) {
    return -1;
  }

  SDL_Keymod mod = SDL_GetModState();
  if ((mod & KMOD_SHIFT) != 0) {
    key = std::toupper(key);
  }
  return key;
}

void STB_TEXTEDIT_LAYOUTROW(StbTexteditRow *r, textedit_buffer *buffer, int line_start_idx) {
  const uint32_t *text_remaining = nullptr;

  // TODO: handle actual more than one line...
  const fw::point size = buffer->font->measure_string(buffer->codepoints);
  r->x0 = 0.0f;
  r->x1 = size[0];
  r->baseline_y_delta = size[1];
  r->ymin = 0.0f;
  r->ymax = size[1];
  r->num_chars = static_cast<int>(buffer->codepoints.size());
}

/** Not just spaces, but basically anything we can word wrap on. */
bool STB_TEXTEDIT_IS_SPACE(uint32_t ch) {
  static std::basic_string<uint32_t> spaces = conv::utf_to_utf<uint32_t>(" \t\n\r,;(){}[]<>|");
  return spaces.find(ch) != std::string::npos;
}

void STB_TEXTEDIT_DELETECHARS(textedit_buffer *buffer, int pos, int n) {
  buffer->codepoints.erase(pos, n);
}

bool STB_TEXTEDIT_INSERTCHARS(textedit_buffer *buffer, int pos, const uint32_t* new_text, int new_text_len) {
  buffer->codepoints.insert(pos, new_text, new_text_len);
  return true;
}

#define STB_TEXTEDIT_K_LEFT         SDLK_LEFT
#define STB_TEXTEDIT_K_RIGHT        SDLK_RIGHT // keyboard input to move cursor right
#define STB_TEXTEDIT_K_UP           SDLK_UP // keyboard input to move cursor up
#define STB_TEXTEDIT_K_DOWN         SDLK_DOWN // keyboard input to move cursor down
#define STB_TEXTEDIT_K_LINESTART    SDLK_HOME // keyboard input to move cursor to start of line
#define STB_TEXTEDIT_K_LINEEND      SDLK_END // keyboard input to move cursor to end of line
#define STB_TEXTEDIT_K_TEXTSTART    (1<<29 | 1)  // TODO // keyboard input to move cursor to start of text
#define STB_TEXTEDIT_K_TEXTEND      (1<<29 | 2) // TODO // keyboard input to move cursor to end of text
#define STB_TEXTEDIT_K_DELETE       SDLK_DELETE // keyboard input to delete selection or character under cursor
#define STB_TEXTEDIT_K_BACKSPACE    SDLK_BACKSPACE // keyboard input to delete selection or character left of cursor
#define STB_TEXTEDIT_K_UNDO         (1<<29 | 4) // TODO
#define STB_TEXTEDIT_K_REDO         (1<<29 | 8) // TODO
#define STB_TEXTEDIT_K_WORDLEFT     (1<<29 | 16) // TODO
#define STB_TEXTEDIT_K_WORDRIGHT    (1<<29 | 32) // TODO
#define STB_TEXTEDIT_K_SHIFT        (1<<28) // This is combined with the other key codes, so must be unique bit.

#include <stb/stb_textedit.h>

//-----------------------------------------------------------------------------

namespace fw {
namespace gui {

/** Property that sets the text of the widget. */
class textedit_text_property : public property {
private:
  std::string _text;
public:
  textedit_text_property(std::string const &text) :
    _text(text) {
  }

  void apply(widget *widget) {
    textedit *te = dynamic_cast<textedit *>(widget);
    te->_buffer->codepoints = conv::utf_to_utf<uint32_t>(_text);
  }
};

class textedit_filter_property : public property {
private:
  std::function<bool(std::string ch)> _filter;
public:
  textedit_filter_property(std::function<bool(std::string ch)> filter) :
    _filter(filter) {
  }

  void apply(widget *widget) {
    textedit *te = dynamic_cast<textedit *>(widget);
    te->_filter = _filter;
  }
};

//-----------------------------------------------------------------------------

textedit::textedit(gui *gui) : widget(gui), _buffer(new textedit_buffer()), _cursor_flip_time(0), _draw_cursor(true) {
  _background = gui->get_drawable_manager()->get_drawable("textedit");
  _selection_background = gui->get_drawable_manager()->get_drawable("textedit_selection");
  _cursor = gui->get_drawable_manager()->get_drawable("textedit_cursor");
  stb_textedit_initialize_state(&_buffer->state, true /* is_single_line */);
}

textedit::~textedit() {
  delete _buffer;
}

property *textedit::text(std::string const &text) {
  return new textedit_text_property(text);
}

property *textedit::filter(std::function<bool(std::string ch)> filter) {
  return new textedit_filter_property(filter);
}

void textedit::on_focus_gained() {
  widget::on_focus_gained();
  _draw_cursor = true;
  _cursor_flip_time = 0.0f;
}

void textedit::on_focus_lost() {
  widget::on_focus_lost();
}

bool textedit::on_key(int key, bool is_down) {
  // stb_textedit only supports one shift key, so if it's right-shift just translate to left
  if (key == SDLK_RSHIFT || key == SDLK_LSHIFT) {
    key = STB_TEXTEDIT_K_SHIFT;
  }

  if (is_down) {
    if (_filter) {
      std::string str(1, (char) key); // TODO: this is horrible!
      if (!_filter(str)) {
        return true;
      }
    }

    stb_textedit_key(_buffer, &_buffer->state, key);
  }
  _cursor_flip_time = 0.0f;
  _draw_cursor = true;
  return true;
}

bool textedit::on_mouse_down(float x, float y) {
  widget::on_mouse_down(x, y);
  stb_textedit_click(_buffer, &_buffer->state, x, y);
  _cursor_flip_time = 0.0f;
  _draw_cursor = true;
  return true;
}

bool textedit::on_mouse_up(float x, float y) {
  return widget::on_mouse_up(x, y);
}

void textedit::set_filter(std::function<bool(std::string ch)> filter) {
  _filter = filter;
}

void textedit::select_all() {
  _buffer->state.select_start = 0;
  _buffer->state.cursor = _buffer->state.select_end = _buffer->codepoints.size();
}

std::string textedit::get_text() const {
  return conv::utf_to_utf<char>(_buffer->codepoints);
}

void textedit::set_text(std::string const &text) {
  _buffer->codepoints = conv::utf_to_utf<uint32_t>(text);
}

std::string textedit::get_cursor_name() const {
  return "i-beam";
}

void textedit::update(float dt) {
  if (_focused) {
    _cursor_flip_time += dt;
    if (_cursor_flip_time > 0.75f) {
      _draw_cursor = !_draw_cursor;
      _cursor_flip_time = 0.0f;
    }
  }
}

void textedit::render() {
  _background->render(get_left(), get_top(), get_width(), get_height());

  if (_buffer->state.select_start != _buffer->state.select_end) {
    float left = get_left() /* + something */;
    float width = get_left() + 100 /*something else */;
    _selection_background->render(left, get_top() + 1, width, get_height() - 2);
  }

  if (!_buffer->codepoints.empty()) {
    fw::framework::get_instance()->get_font_manager()->get_face()->draw_string(
        get_left(), get_top() + get_height() / 2, _buffer->codepoints,
        static_cast<fw::font_face::draw_flags>(fw::font_face::align_left | fw::font_face::align_middle),
        fw::colour::BLACK());
  }

  if (_focused && _draw_cursor) {
    int cursor_pos = _buffer->state.cursor;
    fw::point size_to_cursor = _buffer->font->measure_substring(_buffer->codepoints, 0, cursor_pos);
    float left = get_left() + size_to_cursor[0];
    _cursor->render(left, get_top() + 2, 1.0f, get_height() - 4.0f);
  }
}

}
}
