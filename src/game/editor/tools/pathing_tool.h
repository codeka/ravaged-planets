#pragma once

#include <memory>

#include <framework/model.h>
#include <framework/path_find.h>
#include <framework/scenegraph.h>

#include <game/editor/tools/tools.h>

class PathingToolWindow;
class CollisionPatchNode;

namespace ed {
class EditorWorld;

class PathingTool: public Tool {
private:
  enum TestMode {
    kTestNone, kTestStart, kTestEnd
  };

  TestMode test_mode_;
  PathingToolWindow *wnd_;
  std::vector<bool> collision_data_;
  std::shared_ptr<fw::Model> marker_;
  fw::Vector start_pos_;
  bool start_set_;
  fw::Vector end_pos_;
  bool end_set_;
  bool simplify_;
  std::vector<std::shared_ptr<CollisionPatchNode>> patches_;
  std::shared_ptr<fw::TimedPathFind> path_find_;

  std::shared_ptr<fw::sg::Node> current_path_node_;
  std::shared_ptr<fw::ModelNode> start_marker_;
  std::shared_ptr<fw::ModelNode> end_marker_;

  void on_key(std::string keyname, bool is_down);
  void find_path();
public:
  PathingTool(EditorWorld *wrld);
  virtual ~PathingTool();

  virtual void activate();
  virtual void deactivate();

  void set_simplify(bool enabled);
  void set_test_start();
  void set_test_end();
  void stop_testing();
};

}
