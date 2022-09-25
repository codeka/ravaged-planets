#pragma once

#include <framework/color.h>
#include <game/entities/entity.h>

namespace fw {
class VertexBuffer;
class IndexBuffer;
class texture;
}

namespace ent {
class OwnableComponent;

// The selectable_component is added to entities which can be selected by the user (e.g.
// buildings, units and so on)
class SelectableComponent: public EntityComponent {
private:
  bool is_selected_;
  float selection_radius_;
  OwnableComponent *ownable_;

  static void populate_buffers();

  fw::Color highlight_color_;
  bool is_highlighted_;

public:
  static const int identifier = 300;

  SelectableComponent();
  virtual ~SelectableComponent();

  // this is called after the Entity loads all of it's components
  virtual void initialize();

  virtual void apply_template(luabind::object const &tmpl);

  virtual void render(fw::sg::Scenegraph &Scenegraph,
      fw::Matrix const &transform);

  // gets or sets a flag which indicates whether we're selected or not
  void set_is_selected(bool selected);
  bool get_is_selected() const {
    return is_selected_;
  }
  boost::signals2::signal<void(bool)> sig_selected;

  // highlight the Entity with the given color
  void highlight(fw::Color const &col);
  void unhighlight();

  void set_selection_radius(float ParticleRotation);
  float get_selection_radius() const {
    return selection_radius_;
  }

  virtual int get_identifier() {
    return identifier;
  }
};
}
