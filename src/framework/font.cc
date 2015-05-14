
#include <boost/foreach.hpp>
#include <boost/locale.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <framework/bitmap.h>
#include <framework/exception.h>
#include <framework/font.h>
#include <framework/logging.h>
#include <framework/paths.h>
#include <framework/texture.h>

typedef std::basic_string<uint32_t> utf32string;

namespace fs = boost::filesystem;
namespace conv = boost::locale::conv;

#define FT_CHECK(fn) { \
  FT_Error err = fn; \
  if (err) { \
    BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info(#fn)/*TODO << freetype_error_info(err)*/); \
  } \
}

namespace fw {

font_face::font_face(font_manager *manager, fs::path const &filename) :
    _manager(manager) {
  FT_CHECK(FT_New_Face(_manager->_library, filename.string().c_str(), 0, &_face));
  fw::debug << "Loaded " << filename << std::endl;
  fw::debug << "  " << _face->num_faces << " face(s) " << _face->num_glyphs << " glyph(s)" << std::endl;

  // Set the size to 12px (todo: allow multiple sizes?)
  FT_CHECK(FT_Set_Pixel_Sizes(_face, 0, 16));

  // TODO: allow us to resize the bitmap?
  _bitmap = std::shared_ptr<fw::bitmap>(new fw::bitmap(256, 256));
  _texture = std::shared_ptr<fw::texture>(new fw::texture());
  _texture_dirty = true;
}

font_face::~font_face() {
}

void font_face::cache_string(std::string const &str) {
  BOOST_FOREACH(uint32_t ch, conv::utf_to_utf<uint32_t>(str)) {
    int glyph_index = FT_Get_Char_Index(_face, ch);

    // Load the glyph into the glyph slot and render it if it's not a bitmap
    FT_CHECK(FT_Load_Glyph(_face, glyph_index, FT_LOAD_DEFAULT));
    if (_face->glyph->format != FT_GLYPH_FORMAT_BITMAP) {
      // TODO: FT_RENDER_MODE_LCD?
      FT_CHECK(FT_Render_Glyph(_face->glyph, FT_RENDER_MODE_NORMAL));
    }

    for (int y = 0; y < _face->glyph->bitmap.rows; y++) {
      for (int x = 0; x < _face->glyph->bitmap.width; x++) {
        uint32_t rgba = 0xffffff00 | _face->glyph->bitmap.buffer[y * _face->glyph->bitmap.width + x];
        _bitmap->set_pixel(x, y, rgba);
      }
    }
  }
}

//-----------------------------------------------------------------------------

void font_manager::initialize() {
  FT_CHECK(FT_Init_FreeType(&_library));
}

std::shared_ptr<font_face> font_manager::get_face() {
  return get_face(fw::resolve("gui/SaccoVanzetti.ttf"));
}

/** Gets the \ref font_face for the font at the given path (assumed to be a .ttf file). */
std::shared_ptr<font_face> font_manager::get_face(fs::path const &filename) {
  // TODO: thread-safety?
  std::shared_ptr<font_face> face = _faces[filename.string()];
  if (!face) {
    face = std::shared_ptr<font_face>(new font_face(this, filename));
    _faces[filename.string()] = face;
  }
  return face;
}

}
