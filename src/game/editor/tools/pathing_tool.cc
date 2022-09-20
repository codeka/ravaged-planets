
#include <functional>
#include <boost/foreach.hpp>

#include <framework/framework.h>
#include <framework/camera.h>
#include <framework/graphics.h>
#include <framework/gui/builder.h>
#include <framework/gui/button.h>
#include <framework/gui/checkbox.h>
#include <framework/gui/gui.h>
#include <framework/gui/widget.h>
#include <framework/gui/window.h>
#include <framework/input.h>
#include <framework/model.h>
#include <framework/model_manager.h>
#include <framework/scenegraph.h>
#include <framework/shader.h>
#include <framework/paths.h>
#include <framework/path_find.h>

#include <game/editor/tools/pathing_tool.h>
#include <game/editor/windows/main_menu.h>
#include <game/editor/editor_terrain.h>
#include <game/editor/editor_world.h>
#include <game/world/terrain_helper.h>

static int PATCH_SIZE = 32; // our patches are independent of the terrain patches

using namespace std::placeholders;
using namespace fw::gui;

//-----------------------------------------------------------------------------

enum IDS {
  START_ID,
  END_ID,
};

class pathing_tool_window {
private:
  Window *_wnd;
  ed::pathing_tool *_tool;

  bool on_start_click(Widget *w);
  bool on_end_click(Widget *w);
  bool on_simplify_click(Widget *w);

public:
  pathing_tool_window(ed::pathing_tool *tool);
  virtual ~pathing_tool_window();

  void show();
  void hide();
};

pathing_tool_window::pathing_tool_window(ed::pathing_tool *tool) :
    _tool(tool), _wnd(nullptr) {
  _wnd = Builder<Window>(px(10), px(30), px(100), px(94)) << Window::background("frame")
      << (Builder<Button>(px(4), px(4), sum(pct(100), px(-8)), px(30)) << Button::text("Start") << Widget::id(START_ID)
          << Widget::click(std::bind(&pathing_tool_window::on_start_click, this, _1)))
      << (Builder<Button>(px(4), px(38), sum(pct(100), px(-8)), px(30)) << Button::text("End") << Widget::id(END_ID)
          << Widget::click(std::bind(&pathing_tool_window::on_end_click, this, _1)))
      << (Builder<Checkbox>(px(4), px(72), sum(pct(100), px(-8)), px(18)) << Checkbox::text("Simplify")
          << Widget::click(std::bind(&pathing_tool_window::on_simplify_click, this, _1)));
  fw::framework::get_instance()->get_gui()->attach_widget(_wnd);
}

pathing_tool_window::~pathing_tool_window() {
  fw::framework::get_instance()->get_gui()->detach_widget(_wnd);
}

void pathing_tool_window::show() {
  _wnd->set_visible(true);
}

void pathing_tool_window::hide() {
  _wnd->set_visible(false);
}

bool pathing_tool_window::on_start_click(Widget *w) {
  ed::statusbar->set_message("Set test start...");
  _tool->set_test_start();
  _wnd->find<Button>(START_ID)->set_pressed(true);
  _wnd->find<Button>(END_ID)->set_pressed(false);
  return true;
}

bool pathing_tool_window::on_end_click(Widget *w) {
  ed::statusbar->set_message("Set test end...");
  _tool->set_test_end();
  _wnd->find<Button>(START_ID)->set_pressed(false);
  _wnd->find<Button>(END_ID)->set_pressed(true);
  return true;
}

bool pathing_tool_window::on_simplify_click(Widget *w) {
  _tool->set_simplify(dynamic_cast<Checkbox *>(w)->is_checked());
  return true;
}

//-----------------------------------------------------------------------------

class collision_patch {
private:
  int _patch_x, _patch_z;
  static std::shared_ptr<fw::IndexBuffer> _ib;
  std::shared_ptr<fw::VertexBuffer> _vb;

public:
  void bake(std::vector<bool> &data, float *heights, int width, int length, int patch_x, int patch_z);

  void render(fw::sg::scenegraph &scenegraph, fw::Matrix const &world);
};

std::shared_ptr<fw::IndexBuffer> collision_patch::_ib;
std::shared_ptr<fw::VertexBuffer> current_path_vb;
std::shared_ptr<fw::IndexBuffer> current_path_ib;

void collision_patch::bake(std::vector<bool> &data, float *heights, int width, int length, int patch_x, int patch_z) {
  if (!_ib) {
    std::vector<uint16_t> indices;
    game::generate_terrain_indices_wireframe(indices, PATCH_SIZE);

    _ib = std::shared_ptr<fw::IndexBuffer>(new fw::IndexBuffer());
    _ib->set_data(indices.size(), &indices[0]);
  }

  std::vector<fw::vertex::xyz_c> vertices((PATCH_SIZE + 1) * (PATCH_SIZE + 1));
  for (int z = 0; z <= PATCH_SIZE; z++) {
    for (int x = 0; x <= PATCH_SIZE; x++) {
      int ix = fw::constrain((patch_x * PATCH_SIZE) + x, width);
      int iz = fw::constrain((patch_z * PATCH_SIZE) + z, length);

      bool passable = data[(iz * width) + ix];
      fw::Color color(passable ? fw::Color(0.1f, 1.0f, 0.1f) : fw::Color(1.0f, 0.1f, 0.1f));

      int index = z * (PATCH_SIZE + 1) + x;
      vertices[index] = fw::vertex::xyz_c(x, heights[iz * width + ix] + 0.1f, z, color);
    }
  }

  _vb = fw::VertexBuffer::create<fw::vertex::xyz_c>();
  _vb->set_data(vertices.size(), vertices.data());
}

void collision_patch::render(fw::sg::scenegraph &scenegraph, fw::Matrix const &world) {
  std::shared_ptr<fw::sg::node> node(new fw::sg::node());
  node->set_world_matrix(world);

  // we have to set up the scenegraph node with these manually
  node->set_vertex_buffer(_vb);
  node->set_index_buffer(_ib);
  node->set_primitive_type(fw::sg::primitive_linelist);
  std::shared_ptr<fw::shader> shader = fw::shader::create("basic.shader");
  std::shared_ptr<fw::shader_parameters> shader_params = shader->create_parameters();
  shader_params->set_program_name("notexture");
  node->set_shader(shader);
  node->set_shader_parameters(shader_params);

  scenegraph.add_node(node);
}

//-----------------------------------------------------------------------------

namespace ed {
REGISTER_TOOL("pathing", pathing_tool);

pathing_tool::pathing_tool(editor_world *wrld) :
    tool(wrld), _start_set(false), _end_set(false), _test_mode(test_none), _simplify(true) {
  _wnd = new pathing_tool_window(this);
  _marker = fw::framework::get_instance()->get_model_manager()->get_model("marker");
}

pathing_tool::~pathing_tool() {
  delete _wnd;
}

void pathing_tool::activate() {
  tool::activate();
  _wnd->show();

  int width = get_terrain()->get_width();
  int length = get_terrain()->get_length();
  _collision_data.resize(width * length);
  get_terrain()->build_collision_data(_collision_data);

  _patches.resize((width / PATCH_SIZE) * (length / PATCH_SIZE));

  std::shared_ptr<fw::timed_path_find> pf(
      new fw::timed_path_find(get_terrain()->get_width(), get_terrain()->get_length(), _collision_data));
  _path_find = pf;

  fw::Input *inp = fw::framework::get_instance()->get_input();
  _keybind_tokens.push_back(
      inp->bind_key("Left-Mouse", fw::InputBinding(std::bind(&pathing_tool::on_key, this, _1, _2))));
}

void pathing_tool::deactivate() {
  tool::deactivate();
  _wnd->hide();
}

int get_patch_index(int patch_x, int patch_z, int patches_width, int patches_length, int *new_patch_x,
    int *new_patch_z) {
  patch_x = fw::constrain(patch_x, patches_width);
  patch_z = fw::constrain(patch_z, patches_length);

  if (new_patch_x != 0)
    *new_patch_x = patch_x;
  if (new_patch_z != 0)
    *new_patch_z = patch_z;

  return patch_z * patches_width + patch_x;
}

void pathing_tool::render(fw::sg::scenegraph &scenegraph) {
  // we want to render the patches centred on where the camera is looking
  fw::Camera *camera = fw::framework::get_instance()->get_camera();
  fw::Vector cam_loc = camera->get_position();
  fw::Vector cam_dir = camera->get_direction();
  fw::Vector location = get_terrain()->get_cursor_location(cam_loc, cam_dir);

  int centre_patch_x = (int) (location[0] / PATCH_SIZE);
  int centre_patch_z = (int) (location[2] / PATCH_SIZE);

  int patch_width = get_terrain()->get_width() / PATCH_SIZE;
  int patch_length = get_terrain()->get_length() / PATCH_SIZE;
  for (int patch_z = centre_patch_z - 1; patch_z <= centre_patch_z + 1; patch_z++) {
    for (int patch_x = centre_patch_x - 1; patch_x <= centre_patch_x + 1; patch_x++) {
      int new_patch_x, new_patch_z;
      int patch_index = get_patch_index(patch_x, patch_z, patch_width, patch_length, &new_patch_x, &new_patch_z);

      if (!_patches[patch_index])
        _patches[patch_index] = bake_patch(new_patch_x, new_patch_z);

      fw::Matrix world = fw::translation(static_cast<float>(patch_x * PATCH_SIZE), 0,
          static_cast<float>(patch_z * PATCH_SIZE));

      _patches[patch_index]->render(scenegraph, world);
    }
  }

  if (_start_set) {
    fw::Matrix loc(fw::translation(_start_pos));
    _marker->set_color(fw::Color(1, 0.1f, 1, 0.1f));
    _marker->render(scenegraph, loc);
  }

  if (_end_set) {
    fw::Matrix loc(fw::translation(_end_pos));
    _marker->set_color(fw::Color(1, 1, 0.1f, 0.1f));
    _marker->render(scenegraph, loc);
  }

  if (current_path_vb) {
    std::shared_ptr<fw::sg::node> sgnode(new fw::sg::node());
    sgnode->set_vertex_buffer(current_path_vb);
    sgnode->set_index_buffer(current_path_ib);
    sgnode->set_cast_shadows(false);
    sgnode->set_primitive_type(fw::sg::primitive_linelist);
    std::shared_ptr<fw::shader> shader = fw::shader::create("basic.shader");
    std::shared_ptr<fw::shader_parameters> shader_params = shader->create_parameters();
    shader_params->set_program_name("notexture");
    sgnode->set_shader(shader);
    sgnode->set_shader_parameters(shader_params);

    scenegraph.add_node(sgnode);
  }
}

std::shared_ptr<collision_patch> pathing_tool::bake_patch(int patch_x, int patch_z) {
  std::shared_ptr<collision_patch> patch(new collision_patch());
  patch->bake(_collision_data, get_terrain()->get_height_data(), get_terrain()->get_width(),
      get_terrain()->get_length(), patch_x, patch_z);
  return patch;
}

void pathing_tool::set_simplify(bool value) {
  _simplify = value;
  find_path();
}

void pathing_tool::set_test_start() {
  _test_mode = test_start;
}

void pathing_tool::set_test_end() {
  _test_mode = test_end;
}

void pathing_tool::stop_testing() {
  _test_mode = test_none;
}

void pathing_tool::on_key(std::string keyname, bool is_down) {
  if (_test_mode == test_none) {
    return;
  }

  if (!is_down) {
    if (_test_mode == test_start) {
      _start_pos = _terrain->get_cursor_location();
      _start_set = true;
    } else if (_test_mode == test_end) {
      _end_pos = _terrain->get_cursor_location();
      _end_set = true;
    }

    find_path();
  }
}

void pathing_tool::find_path() {
  if (!_start_set || !_end_set)
    return;

  std::vector<fw::Vector> full_path;
  if (!_path_find->find(full_path, _start_pos, _end_pos)) {
    statusbar->set_message((boost::format("No path found after %1%ms") % (_path_find->total_time * 1000.0f)).str());
  } else {
    std::vector<fw::Vector> path;
    _path_find->simplify_path(full_path, path);
    statusbar->set_message((boost::format("Path found in %1%ms, %2% nodes, %3% nodes (simplified)")
        % (_path_find->total_time * 1000.0f)
        % full_path.size()
        % path.size()).str());

    std::vector<fw::vertex::xyz_c> buffer;

    if (_simplify) {
      BOOST_FOREACH(fw::Vector loc, path) {
        float height = get_terrain()->get_vertex_height(static_cast<int>(loc[0]), static_cast<int>(loc[2]));
        buffer.push_back(fw::vertex::xyz_c(loc[0], height + 0.2f, loc[2], fw::Color(1, 0.5f, 0.5f, 1)));
      }
    } else {
      BOOST_FOREACH(fw::Vector loc, full_path) {
        float height = get_terrain()->get_vertex_height(static_cast<int>(loc[0]), static_cast<int>(loc[2]));
        buffer.push_back(fw::vertex::xyz_c(loc[0], height + 0.2f, loc[2], fw::Color(1, 0.5f, 0.5f, 1)));
      }
    }

    std::shared_ptr<fw::VertexBuffer> vb = fw::VertexBuffer::create<fw::vertex::xyz_c>();
    vb->set_data(buffer.size(), &buffer[0]);

    current_path_vb = vb;

    current_path_ib = std::shared_ptr<fw::IndexBuffer>(new fw::IndexBuffer());
    std::vector<uint16_t> indices(buffer.size());
    for (int i = 0; i < buffer.size(); i++) {
      indices[i] = i;
    }
    current_path_ib->set_data(indices.size(), indices.data());
  }
}

}
