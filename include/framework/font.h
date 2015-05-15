#pragma once

#include <memory>
#include <map>
#include <string>
#include <boost/filesystem.hpp>

/* Cut'n'pasted from the freetype.h header so we don't have to include that whole thing. */
typedef struct FT_LibraryRec_ *FT_Library;
typedef struct FT_FaceRec_*  FT_Face;

namespace fw {
class bitmap;
class font_manager;
class glyph;
class string_cache_entry;
class texture;

class font_face {
private:
  font_manager *_manager;
  FT_Face _face;
  int _size; //<! Size in pixels of this font.

  // Glyphs are rendered into a bitmap and then copied to a texture as required, before we render
  // when draw is called.
  std::shared_ptr<fw::bitmap> _bitmap;
  std::shared_ptr<fw::texture> _texture;
  bool _texture_dirty;

  /** Mapping of UTF-32 character to glyph object describing the glyph. */
  std::map<uint32_t, glyph *> _glyphs;

  /**
   * Mapping of UTF-32 strings to a \ref string_cache_entry which caches the data we need to draw the
   * given string.
   */
  std::map<std::basic_string<uint32_t>, string_cache_entry *> _string_cache;

  void ensure_glyphs(std::basic_string<uint32_t> const &str);
  void draw_string(int x, int y, std::basic_string<uint32_t> const &str);
  string_cache_entry *create_cache_entry(std::basic_string<uint32_t> const &str);
public:
  font_face(font_manager *manager, boost::filesystem::path const &filename);
  ~font_face();

  // Only useful for debugging, gets the atlas bitmap we're using to hold rendered glyphs
  std::shared_ptr<fw::bitmap> get_bitmap() const {
    return _bitmap;
  }

  /**
   * Pre-renders all of the glyphs required to render the given string, useful when starting up to
   * ensure common characters are already available.
   */
  void ensure_glyphs(std::string const &str);

  /**
   * Draws the given string on the screen at the given (x,y) coordinates.
   */
  void draw_string(int x, int y, std::string const &str);
};

class font_manager {
private:
  friend class font_face;

  FT_Library _library;
  std::map<std::string, std::shared_ptr<font_face>> _faces;

public:
  void initialize();

  /** Gets the default \ref font_face. */
  std::shared_ptr<font_face> get_face();

  /** Gets the \ref font_face for the font at the given path (assumed to be a .ttf file). */
  std::shared_ptr<font_face> get_face(boost::filesystem::path const &filename);
};

}
