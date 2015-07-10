#pragma once

#include <memory>
#include <game/editor/tools/tools.h>

class pathing_tool_window;
class collision_patch;

namespace fw {
class model;
class timed_path_find;
}

namespace ed {
class editor_world;

class pathing_tool: public tool {
private:
  enum test_mode {
    test_none, test_start, test_end
  };

  test_mode _test_mode;
  pathing_tool_window *_wnd;
  std::vector<bool> _collision_data;
  std::shared_ptr<fw::model> _marker;
  fw::vector _start_pos;
  bool _start_set;
  fw::vector _end_pos;
  bool _end_set;
  bool _simplify;
  std::vector<std::shared_ptr<collision_patch>> _patches;
  std::shared_ptr<fw::timed_path_find> _path_find;

  void on_key(std::string keyname, bool is_down);
  std::shared_ptr<collision_patch> bake_patch(int patch_x, int patch_z);
  void find_path();
public:
  pathing_tool(editor_world *wrld);
  virtual ~pathing_tool();

  virtual void activate();
  virtual void deactivate();

  virtual void render(fw::sg::scenegraph &scenegraph);

  void set_simplify(bool enabled);
  void set_test_start();
  void set_test_end();
  void stop_testing();
};

}
