
#include <functional>

#include <absl/strings/str_cat.h>

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
#include <framework/model_node.h>
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

int get_patch_index(int patch_x, int patch_z, int patches_width, int patches_length,
  int* new_patch_x, int* new_patch_z) {
  patch_x = fw::constrain(patch_x, patches_width);
  patch_z = fw::constrain(patch_z, patches_length);

  if (new_patch_x != 0)
    *new_patch_x = patch_x;
  if (new_patch_z != 0)
    *new_patch_z = patch_z;

  return patch_z * patches_width + patch_x;
}

class PathingToolWindow {
private:
  Window *wnd_;
  ed::PathingTool *tool_;

  bool on_start_click(Widget *w);
  bool on_end_click(Widget *w);
  bool on_simplify_click(Widget *w);

public:
  PathingToolWindow(ed::PathingTool *Tool);
  virtual ~PathingToolWindow();

  void show();
  void hide();
};

PathingToolWindow::PathingToolWindow(ed::PathingTool *tool) :
    tool_(tool), wnd_(nullptr) {
  wnd_ = Builder<Window>(px(10), px(30), px(100), px(94))
      << Window::background("frame")
      << (Builder<Button>(px(4), px(4), sum(pct(100), px(-8)), px(30))
          << Button::text("Start")
          << Widget::id(START_ID)
          << Widget::click(std::bind(&PathingToolWindow::on_start_click, this, _1)))
      << (Builder<Button>(px(4), px(38), sum(pct(100), px(-8)), px(30))
          << Button::text("End")
          << Widget::id(END_ID)
          << Widget::click(std::bind(&PathingToolWindow::on_end_click, this, _1)))
      << (Builder<Checkbox>(px(4), px(72), sum(pct(100), px(-8)), px(18))
          << Checkbox::text("Simplify")
          << Widget::click(std::bind(&PathingToolWindow::on_simplify_click, this, _1)));
  fw::Framework::get_instance()->get_gui()->attach_widget(wnd_);
}

PathingToolWindow::~PathingToolWindow() {
  fw::Framework::get_instance()->get_gui()->detach_widget(wnd_);
}

void PathingToolWindow::show() {
  wnd_->set_visible(true);
}

void PathingToolWindow::hide() {
  wnd_->set_visible(false);
}

bool PathingToolWindow::on_start_click(Widget *w) {
  ed::statusbar->set_message("Set test start...");
  tool_->set_test_start();
  wnd_->find<Button>(START_ID)->set_pressed(true);
  wnd_->find<Button>(END_ID)->set_pressed(false);
  return true;
}

bool PathingToolWindow::on_end_click(Widget *w) {
  ed::statusbar->set_message("Set test end...");
  tool_->set_test_end();
  wnd_->find<Button>(START_ID)->set_pressed(false);
  wnd_->find<Button>(END_ID)->set_pressed(true);
  return true;
}

bool PathingToolWindow::on_simplify_click(Widget *w) {
  tool_->set_simplify(dynamic_cast<Checkbox *>(w)->is_checked());
  return true;
}

//-----------------------------------------------------------------------------

// CollisionPatchNode renders the 'state' of the collision information for a patch of terrain.
class CollisionPatchNode : public fw::sg::Node {
private:
  int patch_x_, patch_z_;

public:
  void bake(
      std::vector<bool> &data, float *heights, int width, int length, int patch_x, int patch_z);
};

void CollisionPatchNode::bake(
    std::vector<bool> &data, float *heights, int width, int length, int patch_x, int patch_z) {

  std::vector<uint16_t> indices;
  game::generate_terrain_indices_wireframe(indices, PATCH_SIZE);

  auto ib = std::shared_ptr<fw::IndexBuffer>(new fw::IndexBuffer());
  ib->set_data(indices.size(), &indices[0]);
  set_index_buffer(ib);

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

  auto vb = fw::VertexBuffer::create<fw::vertex::xyz_c>();
  vb->set_data(vertices.size(), vertices.data());
  set_vertex_buffer(vb);

  set_primitive_type(fw::sg::PrimitiveType::kLineList);
  auto shader = fw::Shader::CreateOrEmpty("basic.shader");
  auto shader_params = shader->CreateParameters();
  shader_params->set_program_name("notexture");
  set_shader(shader);
  set_shader_parameters(shader_params);

  fw::Matrix world = fw::translation(static_cast<float>(patch_x * PATCH_SIZE), 0,
    static_cast<float>(patch_z * PATCH_SIZE));
  set_world_matrix(world);
}

//-----------------------------------------------------------------------------

namespace ed {
REGISTER_TOOL("pathing", PathingTool);

PathingTool::PathingTool(EditorWorld *wrld) :
    Tool(wrld), start_set_(false), end_set_(false), test_mode_(kTestNone), simplify_(true) {
  wnd_ = new PathingToolWindow(this);
  auto model = fw::Framework::get_instance()->get_model_manager()->get_model("marker");
  if (!model.ok()) {
    fw::debug << "ERROR loading marker: " << model.status() << std::endl;
  } else {
    marker_ = *model;
  }
}

PathingTool::~PathingTool() {
  delete wnd_;
}

void PathingTool::activate() {
  Tool::activate();
  wnd_->show();

  int width = get_terrain()->get_width();
  int length = get_terrain()->get_length();
  collision_data_.resize(width * length);
  auto status = get_terrain()->BuildCollisionData(collision_data_);
  if (!status.ok()) {
    fw::debug << "ERROR building collision data: " << status << std::endl;
  }

  patches_.resize((width / PATCH_SIZE) * (length / PATCH_SIZE));

  std::shared_ptr<fw::TimedPathFind> pf(
      new fw::TimedPathFind(
          get_terrain()->get_width(), get_terrain()->get_length(), collision_data_));
  path_find_ = pf;

  fw::Input *inp = fw::Framework::get_instance()->get_input();
  keybind_tokens_.push_back(
      inp->bind_key(
          "Left-Mouse",
          fw::InputBinding(std::bind(&PathingTool::on_key, this, _1, _2))));

  int patch_width = get_terrain()->get_width() / PATCH_SIZE;
  int patch_length = get_terrain()->get_length() / PATCH_SIZE;
  for (int patch_z = 0; patch_z <= patch_length; patch_z++) {
    for (int patch_x = 0; patch_x <= patch_width; patch_x++) {
      int new_patch_x, new_patch_z;
      int patch_index =
          get_patch_index(patch_x, patch_z, patch_width, patch_length, &new_patch_x, &new_patch_z);

      if (!patches_[patch_index]) {
        patches_[patch_index] = std::make_shared<CollisionPatchNode>();
      }
    }
  }

  start_marker_ = marker_->create_node(fw::Color(0.0f, 1.0f, 0.0f));
  end_marker_ = marker_->create_node(fw::Color(1.0f, 0.0f, 0.0f));

  fw::Framework::get_instance()->get_scenegraph_manager()->enqueue(
    [this, patch_width, patch_length](fw::sg::Scenegraph& sg) {
      auto data = terrain_->get_collision_data();
      auto heights = terrain_->get_height_data();
      int width = terrain_->get_width();
      int length = terrain_->get_length();

      for (int patch_z = 0; patch_z <= patch_length; patch_z++) {
        for (int patch_x = 0; patch_x <= patch_width; patch_x++) {
          int new_patch_x, new_patch_z;
          int patch_index =
              get_patch_index(
                  patch_x, patch_z, patch_width, patch_length, &new_patch_x, &new_patch_z);
          auto patch = patches_[patch_index];
          if (patch) {
            patch->bake(data, heights, width, length, patch_x, patch_z);
            sg.add_node(patch);
          }
        }
      }

      start_marker_->set_enabled(false);
      sg.add_node(start_marker_);
      end_marker_->set_enabled(false);
      sg.add_node(end_marker_);
    });
}

void PathingTool::deactivate() {
  Tool::deactivate();

  fw::Framework::get_instance()->get_scenegraph_manager()->enqueue(
    [this](fw::sg::Scenegraph& sg) {
      for (auto patch : patches_) {
        sg.remove_node(patch);
      }
    });

  wnd_->hide();
}

void PathingTool::set_simplify(bool value) {
  simplify_ = value;
  find_path();
}

void PathingTool::set_test_start() {
  test_mode_ = kTestStart;
}

void PathingTool::set_test_end() {
  test_mode_ = kTestEnd;
}

void PathingTool::stop_testing() {
  test_mode_ = kTestNone;
}

void PathingTool::on_key(std::string keyname, bool is_down) {
  if (test_mode_ == kTestNone) {
    return;
  }

  if (!is_down) {
    fw::Vector cursor_loc = terrain_->get_cursor_location();
    if (test_mode_ == kTestStart) {
      start_pos_ = cursor_loc;
      start_set_ = true;
    } else if (test_mode_ == kTestEnd) {
      end_pos_ = cursor_loc;
      end_set_ = true;
    }

    fw::Framework::get_instance()->get_scenegraph_manager()->enqueue(
      [this, cursor_loc](fw::sg::Scenegraph& sg) {
        if (test_mode_ == kTestStart) {
          start_marker_->set_enabled(true);
          start_marker_->set_world_matrix(fw::translation(cursor_loc));
        } else if (test_mode_ == kTestEnd) {
          end_marker_->set_enabled(true);
          end_marker_->set_world_matrix(fw::translation(cursor_loc));
        }
      });

    find_path();
  }
}

void PathingTool::find_path() {
  if (!start_set_ || !end_set_)
    return;

  std::vector<fw::Vector> full_path;
  if (!path_find_->find(full_path, start_pos_, end_pos_)) {
    statusbar->set_message(
      absl::StrCat("No path found after ", path_find_->total_time * 1000.0f, "ms"));
  } else {
    std::vector<fw::Vector> path;
    path_find_->simplify_path(full_path, path);
    statusbar->set_message(
      absl::StrCat(
        "Path found in ", path_find_->total_time * 1000.0f, "ms, ", full_path.size(), " nodes, ",
        path.size(), " nodes (simplified)"));

    std::vector<fw::vertex::xyz_c> buffer;

    if (simplify_) {
      for(fw::Vector loc : path) {
        float height = get_terrain()->get_vertex_height(
          static_cast<int>(loc[0]), static_cast<int>(loc[2]));
        buffer.push_back(
          fw::vertex::xyz_c(loc[0], height + 0.2f, loc[2], fw::Color(1, 0.5f, 0.5f, 1)));
      }
    } else {
      for(fw::Vector loc : full_path) {
        float height = get_terrain()->get_vertex_height(
          static_cast<int>(loc[0]), static_cast<int>(loc[2]));
        buffer.push_back(
          fw::vertex::xyz_c(loc[0], height + 0.2f, loc[2], fw::Color(1, 0.5f, 0.5f, 1)));
      }
    }

    fw::Framework::get_instance()->get_scenegraph_manager()->enqueue(
      [this, buffer](fw::sg::Scenegraph& sg) {
        if (!current_path_node_) {
          current_path_node_ = std::make_shared<fw::sg::Node>();
         // current_path_node_->set_cast_shadows(false);
          current_path_node_->set_primitive_type(fw::sg::PrimitiveType::kLineStrip);
          auto shader = fw::Shader::CreateOrEmpty("basic.shader");
          auto shader_params = shader->CreateParameters();
          shader_params->set_program_name("notexture");
          current_path_node_->set_shader(shader);
          current_path_node_->set_shader_parameters(shader_params);

          sg.add_node(current_path_node_);
        }

        auto vb = fw::VertexBuffer::create<fw::vertex::xyz_c>();
        vb->set_data(buffer.size(), buffer.data());
        current_path_node_->set_vertex_buffer(vb);

        auto ib = std::shared_ptr<fw::IndexBuffer>(new fw::IndexBuffer());
        std::vector<uint16_t> indices(buffer.size());
        for (int i = 0; i < buffer.size(); i++) {
          indices[i] = i;
        }
        ib->set_data(indices.size(), indices.data());
        current_path_node_->set_index_buffer(ib);
      });
  }
}

}
