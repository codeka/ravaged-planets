#pragma once

#include <framework/color.h>
#include <framework/gui/window.h>
#include <framework/math.h>
#include <framework/scenegraph.h>

#include <game/world/terrain.h>

namespace ent {
class Entity;
class EntityManager;
class MeshComponent;
class PositionComponent;

// These are the different flags that apply to a single Entity.
enum EntityDebugFlags {
  // If this is set, we should render "steering vectors" which show how the Entity is currently steering.
  kDebugShowSteering = 1,

  // If this is set, we shoudl render some lines to show the path the entity is following.
  kDebugShowPathing = 2,

  kDebugMaxValue = 2
};


// This class contains some debug-related state. It registers the Ctrl+D key to enable/disable
// "Entity debugging" which shows things such as steering behaviours, pathing finding and so on.
class EntityDebug {
private:
  EntityManager *mgr_;
  std::shared_ptr<fw::gui::Window> wnd_;

  bool just_shown_;

  void on_key_press(std::string keyname, bool is_down);
  bool on_show_steering_changed(fw::gui::Widget &w);
  bool on_show_path_changed(fw::gui::Widget &w);

public:
  EntityDebug(EntityManager *mgr);
  ~EntityDebug();

  void initialize();

  // Called each frame to update our state.
  void update();
};

// This is a class that each Entity has access to and allows you to draw various lines and points
// and so on that represent the various debugging information we can visualize.
class EntityDebugView {
private:
  struct Line {
    fw::Vector from;
    fw::Vector to;
    fw::Color col;
  };
  std::vector<Line> lines_;
  MeshComponent* mesh_component_;
  std::shared_ptr<game::Terrain> terrain_;

  // The scenegraph node we are displaying. Only access this on the render thread.
  std::shared_ptr<fw::sg::Node> sg_node_;

public:
  EntityDebugView(Entity* entity);
  ~EntityDebugView();

  void add_line(fw::Vector const &from, fw::Vector const &to,
      fw::Color const &col, bool offset_terrain_height = false);
  void add_circle(fw::Vector const &center, float radius,
      fw::Color const &col);

  void update(float dt);
  void render(fw::sg::Scenegraph &scenegraph, fw::Matrix const &transform);
};

}
