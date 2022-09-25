#include <functional>

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
ENT_COMPONENT_REGISTER("Mesh", MeshComponent);

MeshComponent::MeshComponent() {
}

MeshComponent::MeshComponent(std::shared_ptr<fw::Model> const &Model) :
    model_(Model) {
}

MeshComponent::~MeshComponent() {
}

void MeshComponent::apply_template(luabind::object const &tmpl) {
//  for (luabind::iterator it(tmpl), end; it != end; ++it) {
//    if (it.key() == "FileName") {
//      _model_name = luabind::object_cast<std::string>(*it);
//    }
//  }
}

void MeshComponent::initialize() {
  std::shared_ptr<Entity> Entity(entity_);
  ownable_component_ = Entity->get_component<OwnableComponent>();
}

void MeshComponent::render(fw::sg::Scenegraph &Scenegraph, fw::Matrix const &transform) {
  std::shared_ptr<Entity> Entity(entity_);
  PositionComponent *pos = Entity->get_component<PositionComponent>();
  if (pos != nullptr) {
    if (!model_) {
      model_ = fw::Framework::get_instance()->get_model_manager()->get_model(model_name_);
    }

    if (ownable_component_ != nullptr) {
      game::player *player = ownable_component_->get_owner();
      if (player != nullptr) {
        model_->set_color(player->get_color());
      }
    }

    model_->render(Scenegraph, pos->get_transform() * transform);
  }
}
}
