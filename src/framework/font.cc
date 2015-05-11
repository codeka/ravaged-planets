
#include <ft2build.h>
#include FT_FREETYPE_H

#include <framework/exception.h>
#include <framework/font.h>
#include <framework/logging.h>
#include <framework/paths.h>

namespace fs = boost::filesystem;

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
}

font_face::~font_face() {

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
