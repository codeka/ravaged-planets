#pragma once

#include <memory>
#include <vector>

#include <boost/filesystem.hpp>

namespace fw { namespace gui {

/**
 * A drawable is any object that appears on the screen. It's typically a nine-patch or image and is used at the
 * background of widgets and windows.
 */
class drawable {
private:
public:
};

/**
 * A ninepatch_drawable is a \ref drawable that is rendered as a nine-patch.
 */
class ninepatch_drawable : public drawable {
private:
public:
};

/**
 * A class for managing drawables. Loads and parses them from XML files.
 */
class drawable_manager {
private:
  std::vector<std::shared_ptr<drawable>> _drawables;

public:
  drawable_manager();
  ~drawable_manager();

  /** Parses the given XML file and extracts all of the drawables. */
  void parse(boost::filesystem::path const &file);
};

} }
