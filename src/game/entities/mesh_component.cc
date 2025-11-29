#include <filesystem>
#include <functional>

#include <framework/framework.h>
#include <framework/graphics.h>
#include <framework/model.h>
#include <framework/model_manager.h>
#include <framework/model_node.h>
#include <framework/paths.h>
#include <framework/logging.h>
#include <framework/scenegraph.h>

#include <game/entities/entity.h>
#include <game/entities/entity_factory.h>
#include <game/entities/position_component.h>
#include <game/entities/mesh_component.h>
#include <game/entities/ownable_component.h>
#include <game/simulation/player.h>

namespace fs = std::filesystem;
using namespace std::placeholders;

namespace ent {

// register the mesh component with the entity_factory
ENT_COMPONENT_REGISTER("Mesh", MeshComponent);

MeshComponent::MeshComponent() {
}

MeshComponent::MeshComponent(std::shared_ptr<fw::Model> const &model) :
    model_(model) {
}

MeshComponent::~MeshComponent() {
  if (sg_node_) {
    auto sg_node = sg_node_;
    fw::Framework::get_instance()->get_scenegraph_manager()->enqueue(
      [sg_node](fw::sg::Scenegraph& scenegraph) {
        scenegraph.remove_node(std::dynamic_pointer_cast<fw::sg::Node>(sg_node));
      });
  }
}

void MeshComponent::apply_template(fw::lua::Value tmpl) {
  model_name_ = tmpl["FileName"];
}

void MeshComponent::initialize() {
  std::shared_ptr<Entity> entity(entity_);
  fw::Color color = fw::Color::WHITE();
  auto ownable_component = entity->get_component<OwnableComponent>();
  if (ownable_component != nullptr) {
    game::Player* player = ownable_component->get_owner();
    if (player != nullptr) {
      color = player->get_color();
    }
  }

  auto model = fw::Framework::get_instance()->get_model_manager()->get_model(model_name_);
  if (!model.ok()) {
    fw::debug << "ERROR loading model: " << model.status() << std::endl;
  } else {
    auto sg_node = (*model)->create_node(color);
    sg_node_ = sg_node;
    fw::Framework::get_instance()->get_scenegraph_manager()->enqueue(
      [sg_node](fw::sg::Scenegraph& scenegraph) {
        scenegraph.add_node(std::dynamic_pointer_cast<fw::sg::Node>(sg_node));
      });
  }
}

void MeshComponent::update(float dt) {
  std::shared_ptr<Entity> entity(entity_);
  if (!entity) return;

  auto pos = entity->get_component<PositionComponent>();
  if (pos != nullptr) {
    fw::Matrix transform = pos->get_transform();

    auto offset = entity->get_attribute("patch_offset_");
    if (offset != nullptr) {
      transform *= fw::translation(offset->get_value<fw::Vector>());
    }

    fw::Framework::get_instance()->get_scenegraph_manager()->enqueue(
      [transform, sg_node=sg_node_](fw::sg::Scenegraph& sg) {
        sg_node->transform = transform;
      });
  }
}

}
