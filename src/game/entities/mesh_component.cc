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
ENT_COMPONENT_REGISTER("Mesh", mesh_component);

mesh_component::mesh_component() {
}

mesh_component::mesh_component(std::shared_ptr<fw::model> const &model) :
    _model(model) {
}

mesh_component::~mesh_component() {
}

void mesh_component::apply_template(luabind::object const &tmpl) {
  for (luabind::iterator it(tmpl), end; it != end; ++it) {
    if (it.key() == "FileName") {
      _model_name = luabind::object_cast<std::string>(*it);
    }
  }
}

void mesh_component::initialize() {
  std::shared_ptr<entity> entity(_entity);
  _ownable_component = entity->get_component<ownable_component>();
}

void mesh_component::render(fw::sg::scenegraph &scenegraph, fw::matrix const &transform) {
  std::shared_ptr<entity> entity(_entity);
  position_component *pos = entity->get_component<position_component>();
  if (pos != nullptr) {
    if (!_model) {
      _model = fw::framework::get_instance()->get_model_manager()->get_model(_model_name);
    }

    if (_ownable_component != nullptr) {
      game::player *player = _ownable_component->get_owner();
      if (player != nullptr) {
        _model->set_colour(player->get_colour());
      }
    }

    _model->render(scenegraph, pos->get_transform() * transform);
  }
}
}
