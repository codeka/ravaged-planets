#pragma once

#include <memory>
#include <vector>

#include <boost/filesystem.hpp>

#include <framework/xml.h>

namespace fw { namespace gui {

/**
 * A drawable is any object that appears on the screen. It's typically a nine-patch or image and is used at the
 * background of widgets and windows.
 */
class drawable {
private:
  int _top;
  int _left;
  int _width;
  int _height;

protected:
  friend class drawable_manager;
  drawable(fw::xml::XMLElement *elem);

public:
};

/**
 * A ninepatch_drawable is a \ref drawable that is rendered as a nine-patch.
 */
class ninepatch_drawable : public drawable {
private:
protected:
  friend class drawable_manager;
  ninepatch_drawable(fw::xml::XMLElement *elem);

public:
};

/**
 * A class for managing drawables. Loads and parses them from XML files.
 */
class drawable_manager {
private:
  std::map<std::string, std::shared_ptr<drawable>> _drawables;

  void parse_drawable_element(fw::xml::XMLElement *elem);
public:
  drawable_manager();
  ~drawable_manager();

  /** Parses the given XML file and extracts all of the drawables. */
  void parse(boost::filesystem::path const &file);

  std::shared_ptr<drawable> get_drawable(std::string const &name);
};

} }
