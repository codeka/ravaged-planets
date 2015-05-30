#pragma once

#include <memory>
#include <map>
#include <mutex>
#include <string>
#include <boost/filesystem.hpp>

#include <framework/vector.h>

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
public:
  /** Flags we use to control how the string is drawn. */
  enum draw_flags {
    draw_default   = 0x0000,
    align_baseline = 0x0000,
    align_left     = 0x0000,
    align_top      = 0x0001,
    align_bottom   = 0x0002,
    align_middle   = 0x0004,
    align_centre   = 0x0008,
    align_right    = 0x0010,
  };

private:
  font_manager *_manager;
  FT_Face _face;
  int _size; //<! Size in pixels of this font.
  std::mutex _mutex;

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
  std::map<std::basic_string<uint32_t>, std::shared_ptr<string_cache_entry>> _string_cache;

  void ensure_glyphs(std::basic_string<uint32_t> const &str);
  void draw_string(int x, int y, std::basic_string<uint32_t> const &str, draw_flags flags);
  fw::point measure_string(std::basic_string<uint32_t> const &str);
  std::shared_ptr<string_cache_entry> get_or_create_cache_entry(std::basic_string<uint32_t> const &str);
  std::shared_ptr<string_cache_entry> create_cache_entry(std::basic_string<uint32_t> const &str);
public:
  font_face(font_manager *manager, boost::filesystem::path const &filename);
  ~font_face();

  /** Called by the font_managed every update frame. */
  void update(float dt);

  // Only useful for debugging, gets the atlas bitmap we're using to hold rendered glyphs
  std::shared_ptr<fw::bitmap> get_bitmap() const {
    return _bitmap;
  }

  /**
   * Pre-renders all of the glyphs required to render the given string, useful when starting up to
   * ensure common characters are already available.
   */
  void ensure_glyphs(std::string const &str);

  /** Measures the given string and returns the width/height of the final rendered string. */
  fw::point measure_string(std::string const &str);

  /**
   * Draws the given string on the screen at the given (x,y) coordinates.
   */
  void draw_string(int x, int y, std::string const &str, draw_flags flags = draw_default);
};

class font_manager {
private:
  friend class font_face;

  FT_Library _library;
  std::map<std::string, std::shared_ptr<font_face>> _faces;

public:
  void initialize();
  void update(float dt);

  /** Gets the default \ref font_face. */
  std::shared_ptr<font_face> get_face();

  /** Gets the \ref font_face for the font at the given path (assumed to be a .ttf file). */
  std::shared_ptr<font_face> get_face(boost::filesystem::path const &filename);
};

}
