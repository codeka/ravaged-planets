#pragma once

#include <mutex>

#include <framework/scenegraph.h>

#include <game/editor/editor_terrain.h>

namespace ed {

// This is a scenegraph Node that we can use to render a circle indicating the "area of effect" of tools.
// TODO: support different shapes, circles, squares, etc.
class IndicatorNode : public fw::sg::Node {
private:
  fw::sg::ScenegraphManager* mgr_;
  EditorTerrain* terrain_;

  // All these fields must be accessed only from the render thread. The accessors enforce that.
  float radius_ = 1.0f;
  fw::Vector center_;
  bool dirty_ = false;
  bool initialized_ = false;

public:
  IndicatorNode(EditorTerrain* terrain);
  virtual ~IndicatorNode();

  void set_radius(float radius);
  void set_center(fw::Vector center);

  void render(fw::sg::Scenegraph* sg, fw::Matrix const& model_matrix = fw::identity()) override;

};

}
