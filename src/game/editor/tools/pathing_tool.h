#pragma once

#include <memory>
#include <game/editor/tools/tools.h>

class PathingToolWindow;
class CollisionPatch;

namespace fw {
class Model;
class TimedPathFind;
}

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
  std::vector<std::shared_ptr<CollisionPatch>> patches_;
  std::shared_ptr<fw::TimedPathFind> path_find_;

  void on_key(std::string keyname, bool is_down);
  std::shared_ptr<CollisionPatch> bake_patch(int patch_x, int patch_z);
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
