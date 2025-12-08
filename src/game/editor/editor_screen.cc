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

EditorScreen::EditorScreen() :
    tool_(nullptr), world_(nullptr) {
  ed::statusbar = new StatusbarWindow();
  ed::main_menu = new MainMenuWindow();
  ed::open_map = new OpenMapWindow();
  ed::new_map = new NewMapWindow();
//  ed::message_box = new message_box_window();
  ed::save_map = new SaveMapWindow();
  ed::open_file = new OpenFileWindow();

  ed::main_menu->initialize();
  ed::new_map->initialize();
  ed::save_map->initialize();
  ed::open_map->initialize();
  ed::open_file->Initialize();
  ed::statusbar->initialize();
}

EditorScreen::~EditorScreen() {
  delete ed::main_menu;
  delete ed::open_map;
  delete ed::new_map;
//  delete ed::message_box;
  delete ed::save_map;
  delete ed::open_file;
  delete ed::statusbar;
}

void EditorScreen::show() {
  ed::main_menu->show();
  ed::statusbar->show();

  // Add a light to the scene. TODO: remove it on close? also update it somehow?
  fw::Framework::get_instance()->get_scenegraph_manager()->enqueue([=](fw::sg::Scenegraph& scenegraph) {
    fw::Vector sun(0.485f, 0.485f, 0.727f);
    std::shared_ptr <fw::sg::Light> light(new fw::sg::Light(sun * 200.0f, sun * -1, true));
    scenegraph.add_light(light);
    });
}

void EditorScreen::hide() {
  ed::main_menu->hide();
  ed::statusbar->hide();

  if (tool_ != nullptr) {
    tool_->deactivate();
    delete tool_;
    tool_ = nullptr;
  }

  if (world_ != nullptr) {
    delete world_;
    world_ = nullptr;
  }
}

void EditorScreen::update() {
  if (world_ != nullptr)
    world_->update();

  if (tool_ != nullptr)
    tool_->update();
}

void EditorScreen::new_map(int width, int height) {
  // unset the current tool
  set_active_tool("");

  if (world_ != nullptr) {
    delete world_;
  }
  std::shared_ptr<WorldCreate> creator(new WorldCreate(width, height));
  world_ = new EditorWorld(creator);
  world_->initialize();
  dynamic_cast<EditorWorld *>(world_)->set_name("New Map");
  set_active_tool("heightfield");
}

fw::Status EditorScreen::open_map(std::string const &name) {
  // unset the current tool
  set_active_tool("");

  std::shared_ptr<WorldCreate> reader(new WorldCreate());
  RETURN_IF_ERROR(reader->Read(name));

  delete world_;
  world_ = new EditorWorld(reader);
  world_->initialize();
  set_active_tool("heightfield");
  return fw::OkStatus();
}

void EditorScreen::set_active_tool(std::string const &name) {
  if (world_ == nullptr)
    return;

  if (tool_ != nullptr) {
    tool_->deactivate();
    delete tool_;
    tool_ = nullptr;
  }

  if (name == "")
    return;

  Tool *t = ToolFactory::create_tool(name, world_);
  if (t != nullptr) {
    t->activate();
    tool_ = t;
  }
}

EditorScreen *EditorScreen::get_instance() {
  game::Application *app = dynamic_cast<game::Application *>(fw::Framework::get_instance()->get_app());
  return dynamic_cast<EditorScreen *>(app->get_screen_stack()->get_active_screen());
}

}
