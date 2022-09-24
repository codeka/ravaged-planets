#include <framework/framework.h>
#include <framework/gui/gui.h>
#include <framework/scenegraph.h>
#include <framework/camera.h>

#include <game/application.h>

#include <game/editor/editor_screen.h>
#include <game/editor/editor_terrain.h>
#include <game/editor/editor_world.h>
#include <game/editor/tools/heightfield_tool.h>
#include <game/editor/windows/main_menu.h>
#include <game/editor/windows/save_map.h>
#include <game/editor/windows/open_map.h>
#include <game/editor/windows/new_map.h>
#include <game/editor/windows/save_map.h>
#include <game/editor/windows/message_box.h>
#include <game/editor/windows/open_file.h>

namespace ed {

editor_screen::editor_screen() :
    _tool(nullptr), world_(nullptr) {
  ed::statusbar = new statusbar_window();
  ed::main_menu = new MainMenuWindow();
  ed::open_map = new open_map_window();
  ed::new_map = new new_map_window();
//  ed::message_box = new message_box_window();
  ed::save_map = new save_map_window();
  ed::open_file = new open_file_window();

  ed::main_menu->initialize();
  ed::new_map->initialize();
  ed::save_map->initialize();
  ed::open_map->initialize();
  ed::open_file->initialize();
  ed::statusbar->initialize();
}

editor_screen::~editor_screen() {
  delete ed::main_menu;
  delete ed::open_map;
  delete ed::new_map;
//  delete ed::message_box;
  delete ed::save_map;
  delete ed::open_file;
  delete ed::statusbar;
}

void editor_screen::show() {
  ed::main_menu->show();
  ed::statusbar->show();
}

void editor_screen::hide() {
  ed::main_menu->hide();
  ed::statusbar->hide();

  if (_tool != nullptr) {
    _tool->deactivate();
    delete _tool;
    _tool = nullptr;
  }

  if (world_ != nullptr) {
    delete world_;
    world_ = nullptr;
  }
}

void editor_screen::update() {
  if (world_ != nullptr)
    world_->update();
  if (_tool != nullptr)
    _tool->update();
}

void editor_screen::render(fw::sg::Scenegraph &Scenegraph) {
  if (world_ != nullptr) {
    // set up the properties of the sun that we'll use to Light and also cast shadows
    fw::Vector sun(0.485f, 0.485f, 0.727f);
    fw::Camera *old_cam = fw::Framework::get_instance()->get_camera();
    fw::Vector cam_pos = old_cam->get_position();
    fw::Vector cam_dir = old_cam->get_forward();
    fw::Vector lookat = world_->get_terrain()->get_cursor_location(cam_pos, cam_dir);

    std::shared_ptr <fw::sg::Light> Light(new fw::sg::Light(lookat + sun * 200.0f, sun * -1, true));
    Scenegraph.add_light(Light);
  }

  if (world_ != nullptr) {
    world_->render(Scenegraph);
  }

  if (_tool != nullptr) {
    // Only render the tool if the mouse isn't currently over a widget.
    if (!fw::Framework::get_instance()->get_gui()->is_mouse_over_widget()) {
      _tool->render(Scenegraph);
    }
  }
}

void editor_screen::new_map(int width, int height) {
  // unset the current tool
  set_active_tool("");

  if (world_ != nullptr) {
    delete world_;
  }
  std::shared_ptr<world_create> creator(new world_create(width, height));
  world_ = new editor_world(creator);
  world_->initialize();
  dynamic_cast<editor_world *>(world_)->set_name("New Map");
  set_active_tool("heightfield");
}

void editor_screen::open_map(std::string const &name) {
  // unset the current tool
  set_active_tool("");

  std::shared_ptr<world_create> reader(new world_create());
  reader->read(name);

  delete world_;
  world_ = new editor_world(reader);
  world_->initialize();
  set_active_tool("heightfield");
}

void editor_screen::set_active_tool(std::string const &name) {
  if (world_ == nullptr)
    return;

  if (_tool != nullptr) {
    _tool->deactivate();
    delete _tool;
    _tool = nullptr;
  }

  if (name == "")
    return;

  tool *t = tool_factory::create_tool(name, world_);
  if (t != nullptr) {
    t->activate();
    _tool = t;
  }
}

editor_screen *editor_screen::get_instance() {
  game::Application *app = dynamic_cast<game::Application *>(fw::Framework::get_instance()->get_app());
  return dynamic_cast<editor_screen *>(app->get_screen_stack()->get_active_screen());
}

}
