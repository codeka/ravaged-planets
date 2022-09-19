#pragma once

#include <memory>
#include <vector>

#include <boost/filesystem.hpp>

#include <framework/texture.h>
#include <framework/vector.h>
#include <framework/xml.h>

namespace fw {
class graphics;
class shader;
class shader_parameters;

namespace gui {

// A Drawable is any object that appears on the screen. It's typically a nine-patch or image and is used at the
// background of widgets and windows.
class Drawable {
protected:
  Drawable();

public:
  virtual ~Drawable();
  virtual void render(float x, float y, float width, float height);

  virtual float get_intrinsic_width() const {
    return 0.0f;
  }
  virtual float get_intrinsic_height() const {
    return 0.0f;
  }
};

// A bitmap_drawable is a drawable that's represented by an actual bitmap (i.e. part of a texture).
class BitmapDrawable : public Drawable {
protected:
  int top_;
  int left_;
  int width_;
  int height_;
  bool flipped_;

  friend class DrawableManager;
  BitmapDrawable(std::shared_ptr<fw::Texture> texture);
  BitmapDrawable(std::shared_ptr<fw::Texture> texture, fw::xml::XMLElement* elem);

  std::shared_ptr<fw::Texture> texture_;
  std::shared_ptr<fw::shader> shader_;
  std::shared_ptr<fw::shader_parameters> shader_params_;

  virtual fw::matrix get_pos_transform(float x, float y, float width, float height);
  virtual fw::matrix get_uv_transform();
public:
  virtual ~BitmapDrawable();

  virtual void render(float x, float y, float width, float height);

  float get_intrinsic_width() const {
    return width_;
  }
  float get_intrinsic_height() const {
    return height_;
  }

  void set_flipped(bool flipped) {
    flipped_ = flipped;
  }
};

// A NinePatchDrawable is a \ref bitmap_drawable that is rendered as a nine-patch.
class NinePatchDrawable : public BitmapDrawable {
private:
  int inner_top_;
  int inner_left_;
  int inner_width_;
  int inner_height_;

protected:
  friend class DrawableManager;
  NinePatchDrawable(std::shared_ptr<fw::Texture> texture, fw::xml::XMLElement* elem);

public:
  virtual void render(float x, float y, float width, float height);
};

class StateDrawable : public Drawable {
public:
  enum State {
    kNormal,
    kHover,
    kPressed,
    kSelected,
    kDisabled
  };

private:
  State curr_state_;
  std::map<State, std::shared_ptr<Drawable>> drawable_map_;

public:
  StateDrawable();
  virtual ~StateDrawable();

  void add_drawable(State state, std::shared_ptr<Drawable> drawable);
  void set_current_state(State state);

  virtual void render(float x, float y, float width, float height);
};

/**
 * A class for managing drawables. Loads and parses them from XML files.
 */
class DrawableManager {
private:
  std::map<std::string, std::shared_ptr<Drawable>> drawables_;

  void parse_drawable_element(std::shared_ptr<fw::Texture> texture, fw::xml::XMLElement* elem);
public:
  DrawableManager();
  ~DrawableManager();

  /** Parses the given XML file and extracts all of the drawables. */
  void parse(boost::filesystem::path const& file);

  std::shared_ptr<Drawable> get_drawable(std::string const& name);
  std::shared_ptr<Drawable> build_drawable(std::shared_ptr<fw::Texture> texture,
    float top, float left, float width, float height);
};

}

}