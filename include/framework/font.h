#pragma once

#include <memory>
#include <map>
#include <boost/filesystem.hpp>

/* Cut'n'pasted from the freetype.h header so we don't have to include that whole thing. */
typedef struct FT_LibraryRec_ *FT_Library;
typedef struct FT_FaceRec_*  FT_Face;

namespace fw {
class font_manager;

class font_face {
private:
  font_manager *_manager;
  FT_Face _face;
public:
  font_face(font_manager *manager, boost::filesystem::path const &filename);
  ~font_face();
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
