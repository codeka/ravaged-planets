#include <functional>
#include <boost/foreach.hpp>

#include <framework/framework.h>
#include <framework/graphics.h>
#include <framework/model.h>
#include <framework/model_manager.h>
#include <framework/paths.h>
#include <framework/logging.h>
#include <framework/scenegraph.h>

#include <game/entities/entity.h>
#include <game/entities/entity_factory.h>
#include <game/entities/position_component.h>
#include <game/entities/mesh_component.h>
#include <game/entities/ownable_component.h>
#include <game/simulation/player.h>

namespace fs = boost::filesystem;
using namespace std::placeholders;

namespace ent {

// register the mesh component with the entity_factory
ENT_COMPONENT_REGISTER("mesh", mesh_component);

mesh_component::mesh_component() :
    _model(), _colour(1, 1, 1) {
}

mesh_component::mesh_component(std::shared_ptr<fw::model> const &model) :
    _model(model) {
}

mesh_component::~mesh_component() {
}

// this is called when the entity's owner changes
void mesh_component::owner_changed(ownable_component *ownable) {
  game::player *plyr = ownable->get_owner();
  if (plyr != nullptr) {
    _colour = plyr->get_colour();

    if (_model) {
      _model->set_colour(_colour);
    }
  }
}

void mesh_component::apply_template(std::shared_ptr<entity_component_template> comp_template) {
  BOOST_FOREACH(auto & kvp, comp_template->properties) {
    if (kvp.first == "FileName") {
      std::shared_ptr<fw::model> model = fw::framework::get_instance()->get_model_manager()->get_model(kvp.second);
      set_model(model);
    }
  }

  entity_component::apply_template(comp_template);
}

void mesh_component::initialize() {
  std::shared_ptr<entity> entity(_entity);
  ownable_component *ownable = entity->get_component<ownable_component>();
  if (ownable != nullptr) {
    // get a signal if/when the owner changes
    ownable->owner_changed_event.connect(std::bind(&mesh_component::owner_changed, this, _1));
    owner_changed(ownable);
  }
}

void mesh_component::set_model(std::shared_ptr<fw::model> const &model) {
  _model = model;
  _model->set_colour(_colour);
}

void mesh_component::render(fw::sg::scenegraph &scenegraph, fw::matrix const &transform) {
  std::shared_ptr<entity> entity(_entity);
  position_component *pos = entity->get_component<position_component>();
  if (pos != nullptr) {
    _model->render(scenegraph, pos->get_transform() * transform);
  }
}
}
