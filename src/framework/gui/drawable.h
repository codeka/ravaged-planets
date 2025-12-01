#pragma once

#include <filesystem>
#include <memory>
#include <vector>

#include <framework/graphics.h>
#include <framework/math.h>
#include <framework/shader.h>
#include <framework/texture.h>
#include <framework/xml.h>

namespace fw::gui {

// A Drawable is any object that appears on the Screen. It's typically a nine-patch or image and is used at the
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
public:
  BitmapDrawable(std::shared_ptr<fw::Texture> texture);
  virtual ~BitmapDrawable();

  fw::Status Initialize(XmlElement const &element);

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

protected:
  int top_;
  int left_;
  int width_;
  int height_;
  bool flipped_;

  friend class DrawableManager;

  std::shared_ptr<fw::Texture> texture_;
  std::shared_ptr<fw::Shader> shader_;
  std::shared_ptr<fw::ShaderParameters> shader_params_;

  virtual fw::Matrix get_pos_transform(float x, float y, float width, float height);
  virtual fw::Matrix get_uv_transform();
};

// A NinePatchDrawable is a \ref bitmap_drawable that is rendered as a nine-patch.
class NinePatchDrawable : public BitmapDrawable {
public:
  NinePatchDrawable(std::shared_ptr<fw::Texture> texture);

  virtual void render(float x, float y, float width, float height);

protected:
  friend class DrawableManager;
  fw::Status Initialize(XmlElement const &element);

private:
  int inner_top_;
  int inner_left_;
  int inner_width_;
  int inner_height_;
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

  fw::Status ParseDrawableElement(std::shared_ptr<fw::Texture> texture, XmlElement const &elem);
public:
  DrawableManager();
  ~DrawableManager();

  /** Parses the given XML file and extracts all of the drawables. */
  fw::Status Parse(std::filesystem::path const &file);

  std::shared_ptr<Drawable> get_drawable(std::string const& name);
  std::shared_ptr<Drawable> build_drawable(std::shared_ptr<fw::Texture> texture,
    float top, float left, float width, float height);
};

}
