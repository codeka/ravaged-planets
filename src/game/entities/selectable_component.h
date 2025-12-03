#pragma once

#include <framework/color.h>
#include <framework/graphics.h>
#include <framework/scenegraph.h>
#include <framework/signals.h>

#include <game/entities/entity.h>

namespace ent {
class MeshComponent;
class OwnableComponent;
class PositionComponent;

// The selectable_component is added to entities which can be selected by the user (e.g.
// buildings, units and so on)
class SelectableComponent: public EntityComponent {
private:
  bool is_selected_;
  float selection_radius_;
  OwnableComponent *ownable_;
  MeshComponent* mesh_;
  PositionComponent* pos_;

  fw::Color highlight_color_;
  bool is_highlighted_;

  std::shared_ptr<fw::sg::Node> sg_node_;

public:
  static const int identifier = 300;

  SelectableComponent();
  virtual ~SelectableComponent();

  // this is called after the Entity loads all of it's components
  void initialize() override;

  void update(float dt) override;

  void apply_template(fw::lua::Value tmpl) override;

  // gets or sets a flag which indicates whether we're selected or not
  void set_is_selected(bool selected);
  bool get_is_selected() const {
    return is_selected_;
  }
  fw::Signal<bool /*selected*/> sig_selected;

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
