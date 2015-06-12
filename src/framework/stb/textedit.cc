
#include <string>
#include <boost/locale.hpp>

#include <SDL_keycode.h>

#include <framework/font.h>
#include <framework/input.h>

namespace conv = boost::locale::conv;

#define STB_TEXTEDIT_CHARTYPE uint32_t
#define STB_TEXTEDIT_POSITIONTYPE int
#define STB_TEXTEDIT_UNDOSTATECOUNT 99
#define STB_TEXTEDIT_UNDOCHARCOUNT 999

#include <stb/stb_textedit.h>

#define STB_TEXTEDIT_IMPLEMENTATION

namespace {

/** This is a wrapper class we use to store the actual string data used by std_textedit. */
class string_wrapper {
public:
  std::shared_ptr<fw::font_face> font;
  std::basic_string<uint32_t> str;
};

#define STB_TEXTEDIT_STRING string_wrapper
#define STB_TEXTEDIT_NEWLINE '\n'


int STB_TEXTEDIT_STRINGLEN(string_wrapper *wrapper) {
  return wrapper->str.size();
}

char STB_TEXTEDIT_GETCHAR(const string_wrapper* wrapper, int idx) {
  return wrapper->str[idx];
}

float STB_TEXTEDIT_GETWIDTH(string_wrapper* wrapper, int line_start_idx, int char_idx) {
  return wrapper->font->measure_glyph(wrapper->str[char_idx])[0];
}

int STB_TEXTEDIT_KEYTOTEXT(int key) {
  return key >= 0x10000 ? 0 : key;
}

void STB_TEXTEDIT_LAYOUTROW(StbTexteditRow *r, string_wrapper *wrapper, int line_start_idx) {
  const uint32_t *text_remaining = nullptr;

  // TODO: handle actual more than one line...
  const fw::point size = wrapper->font->measure_string(wrapper->str);
  r->x0 = 0.0f;
  r->x1 = size[0];
  r->baseline_y_delta = size[1];
  r->ymin = 0.0f;
  r->ymax = size[1];
  r->num_chars = static_cast<int>(wrapper->str.size());
}

/** Not just spaces, but basically anything we can word wrap on. */
bool STB_TEXTEDIT_IS_SPACE(uint32_t ch) {
  static std::basic_string<uint32_t> spaces = conv::utf_to_utf<uint32_t>(" \t\n\r,;(){}[]<>|");
  return spaces.find(ch) >= 0;
}

void STB_TEXTEDIT_DELETECHARS(string_wrapper *wrapper, int pos, int n) {
  wrapper->str.erase(pos, n);
}

bool STB_TEXTEDIT_INSERTCHARS(string_wrapper *wrapper, int pos, const uint32_t* new_text, int new_text_len) {
  wrapper->str.insert(pos, new_text, new_text_len);
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
#define STB_TEXTEDIT_K_SHIFT        (1<<28) // TODO

#include <stb/stb_textedit.h>

}
