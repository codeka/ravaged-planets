#pragma once

#include <game/entities/entity.h>
#include <framework/colour.h>

namespace fw {
class model;
}

namespace ent {
class ownable_component;

// the mesh component is a member of all entities that have mesh data, basically
// all the visible entities
class mesh_component: public entity_component {
private:
  ownable_component *_ownable_component;
  std::string _model_name;
  std::shared_ptr<fw::model> _model;

public:
  static const int identifier = 200;

  mesh_component();
  mesh_component(std::shared_ptr<fw::model> const &model);
  virtual ~mesh_component();

  virtual void apply_template(luabind::object const &tmpl);

  // this is called after the entity has added all of it's components
  virtual void initialize();

  std::shared_ptr<fw::model> get_model() const {
    return _model;
  }
  std::string get_model_name() const {
    return _model_name;
  }

  virtual void render(fw::sg::scenegraph &scenegraph, fw::Matrix const &transform);

  virtual int get_identifier() {
    return identifier;
  }
};

}
