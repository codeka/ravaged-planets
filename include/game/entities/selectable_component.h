#pragma once

#include <framework/colour.h>
#include <game/entities/entity.h>

namespace fw {
class vertex_buffer;
class index_buffer;
class texture;
}

namespace ent {
class ownable_component;

// The selectable_component is added to entities which can be selected by the user (e.g.
// buildings, units and so on)
class selectable_component: public entity_component {
private:
  bool _is_selected;
  float _selection_radius;
  ownable_component *_ownable;

  static void populate_buffers();

  fw::colour _highlight_colour;
  bool _is_highlighted;

public:
  static const int identifier = 300;

  selectable_component();
  virtual ~selectable_component();

  // this is called after the entity loads all of it's components
  virtual void initialize();

  virtual void apply_template(std::shared_ptr<entity_component_template> comp_template);

  virtual void render(fw::sg::scenegraph &scenegraph,
      fw::matrix const &transform);

  // gets or sets a flag which indicates whether we're selected or not
  void set_is_selected(bool selected);
  bool get_is_selected() const {
    return _is_selected;
  }
  boost::signals2::signal<void(bool)> sig_selected;

  // highlight the entity with the given colour
  void highlight(fw::colour const &col);
  void unhighlight();

  void set_selection_radius(float value);
  float get_selection_radius() const {
    return _selection_radius;
  }

  virtual int get_identifier() {
    return identifier;
  }
};
}
