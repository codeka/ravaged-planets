#pragma once

#include <memory>
#include <vector>

#include <boost/filesystem.hpp>

#include <framework/xml.h>

namespace fw {
class texture;
class shader;
class shader_parameters;

namespace gui {

/**
 * A drawable is any object that appears on the screen. It's typically a nine-patch or image and is used at the
 * background of widgets and windows.
 */
class drawable {
protected:
  int _top;
  int _left;
  int _width;
  int _height;

  friend class drawable_manager;
  drawable(std::shared_ptr<fw::texture> texture);
  drawable(std::shared_ptr<fw::texture> texture, fw::xml::XMLElement *elem);

  std::shared_ptr<fw::texture> _texture;
  std::shared_ptr<fw::shader> _shader;
  std::shared_ptr<fw::shader_parameters> _shader_params;

public:
  virtual ~drawable();

  virtual void render(float x, float y, float width, float height);
};

/**
 * A ninepatch_drawable is a \ref drawable that is rendered as a nine-patch.
 */
class ninepatch_drawable : public drawable {
private:
  int _inner_top;
  int _inner_left;
  int _inner_width;
  int _inner_height;

protected:
  friend class drawable_manager;
  ninepatch_drawable(std::shared_ptr<fw::texture> texture, fw::xml::XMLElement *elem);

public:

  virtual void render(float x, float y, float width, float height);

};

/**
 * A class for managing drawables. Loads and parses them from XML files.
 */
class drawable_manager {
private:
  std::map<std::string, std::shared_ptr<drawable>> _drawables;

  void parse_drawable_element(std::shared_ptr<fw::texture> texture, fw::xml::XMLElement *elem);
public:
  drawable_manager();
  ~drawable_manager();

  /** Parses the given XML file and extracts all of the drawables. */
  void parse(boost::filesystem::path const &file);

  std::shared_ptr<drawable> get_drawable(std::string const &name);
};

} }
