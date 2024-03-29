#include <game/screens/hud/build_window.h>

#include <memory>

#include <framework/framework.h>
#include <framework/bitmap.h>
#include <framework/camera.h>
#include <framework/gui/builder.h>
#include <framework/gui/button.h>
#include <framework/gui/drawable.h>
#include <framework/gui/gui.h>
#include <framework/gui/label.h>
#include <framework/gui/window.h>
#include <framework/logging.h>
#include <framework/math.h>
#include <framework/model.h>
#include <framework/model_manager.h>
#include <framework/model_node.h>
#include <framework/paths.h>
#include <framework/scenegraph.h>
#include <framework/texture.h>
#include <framework/timer.h>

#include <game/entities/entity.h>
#include <game/entities/entity_factory.h>
#include <game/entities/builder_component.h>
#include <game/entities/mesh_component.h>
#include <game/simulation/local_player.h>
#include <game/simulation/simulation_thread.h>

using namespace fw::gui;
using namespace std::placeholders;

namespace game {

BuildWindow *hud_build = nullptr;

enum ids {
  FIRST_BUILD_BUTTON_ID = 34636,
};

//-----------------------------------------------------------------------------

// Holds information nessecary to render an icon for a given Entity.
class EntityIcon {
private:
  float rotation_;
  std::string template_name_;
  std::shared_ptr<fw::Model> model_;
  std::shared_ptr<fw::ModelNode> model_node_;
  std::shared_ptr<fw::Framebuffer> framebuffer_;
  std::shared_ptr<fw::Texture> color_texture_;
  std::shared_ptr<fw::Texture> depth_texture_;
  std::shared_ptr<Drawable> drawable_;

  void render();
public:
  EntityIcon();

  void initialize();
  void set_model(std::string const &template_name, std::shared_ptr<fw::Model> mdl);
  void update();
  void reset();

  std::string get_template_name();
  std::shared_ptr<Drawable> get_drawable();
};

EntityIcon::EntityIcon() : rotation_(0.0f) {
}

void EntityIcon::initialize() {
  color_texture_ = std::shared_ptr<fw::Texture>(new fw::Texture());
  color_texture_->create(64, 64);

  depth_texture_ = std::shared_ptr<fw::Texture>(new fw::Texture());
  depth_texture_->create_depth(64, 64);

  framebuffer_ = std::shared_ptr<fw::Framebuffer>(new fw::Framebuffer());
  framebuffer_->set_color_buffer(color_texture_);
  framebuffer_->set_depth_buffer(depth_texture_);

  drawable_ = fw::Framework::get_instance()->get_gui()->get_drawable_manager()
      ->build_drawable(color_texture_, 7, 7, 50, 50);
  std::dynamic_pointer_cast<BitmapDrawable>(drawable_)->set_flipped(true);
  render();
}

void EntityIcon::set_model(std::string const &template_name, std::shared_ptr<fw::Model> mdl) {
  template_name_ = template_name;
  model_ = mdl;
  rotation_ = 0.0f;
  render();
}

void EntityIcon::render() {
  if (!model_) {
    return;
  }

  if (!model_node_) {
    // TODO: get correct color
    model_node_ = model_->create_node(fw::Color(1, 0, 0));
  }

  // TODO: keep the same scenegraph and just update the ndoe
  fw::sg::Scenegraph sg;
  sg.set_clear_color(fw::Color(0, 0, 0, 0));

  fw::LookAtCamera cam;
  cam.set_distance(2.0f);
  cam.update(1.0f / 30.0f);
  sg.push_camera(cam.get_render_state());

  fw::Vector sun(0.485f, 0.485f, 0.727f);
  std::shared_ptr <fw::sg::Light> Light(new fw::sg::Light(sun * 200.0f, sun * -1, true));
  sg.add_light(Light);

  model_node_->set_world_matrix(fw::rotate_axis_angle(fw::Vector(0, 1, 0), rotation_).to_matrix());
  sg.add_node(model_node_);

  fw::render(sg, framebuffer_, false);
}

void EntityIcon::update() {
  rotation_ += 3.14159f * fw::Framework::get_instance()->get_timer()->get_update_time();
  fw::Framework::get_instance()->get_graphics()->run_on_render_thread([=]() {
    render();
  });
}

void EntityIcon::reset() {
  rotation_ = 0.0f;
  fw::Framework::get_instance()->get_graphics()->run_on_render_thread([=]() {
    render();
  });
}

std::string EntityIcon::get_template_name() {
  return template_name_;
}

std::shared_ptr<Drawable> EntityIcon::get_drawable() {
  return drawable_;
}

//-----------------------------------------------------------------------------

BuildWindow::BuildWindow() : wnd_(nullptr), require_refresh_(false), mouse_over_button_id_(-1) {
}

BuildWindow::~BuildWindow() {
  fw::Framework::get_instance()->get_gui()->detach_widget(wnd_);
}

void BuildWindow::initialize() {
  wnd_ = Builder<Window>(sum(pct(100), px(-210)), px(220), px(200), px(200))
      << Window::background("frame") << Widget::visible(false)
      << (Builder<Button>(px(10), px(10), px(54), px(54)) << Widget::id(FIRST_BUILD_BUTTON_ID + 0))
      << (Builder<Button>(px(73), px(10), px(54), px(54)) << Widget::id(FIRST_BUILD_BUTTON_ID + 1))
      << (Builder<Button>(px(136), px(10), px(54), px(54)) << Widget::id(FIRST_BUILD_BUTTON_ID + 2))
      << (Builder<Button>(px(10), px(73), px(54), px(54)) << Widget::id(FIRST_BUILD_BUTTON_ID + 3))
      << (Builder<Button>(px(73), px(73), px(54), px(54)) << Widget::id(FIRST_BUILD_BUTTON_ID + 4))
      << (Builder<Button>(px(136), px(73), px(54), px(54)) << Widget::id(FIRST_BUILD_BUTTON_ID + 5))
      << (Builder<Button>(px(10), px(136), px(54), px(54)) << Widget::id(FIRST_BUILD_BUTTON_ID + 6))
      << (Builder<Button>(px(73), px(136), px(54), px(54)) << Widget::id(FIRST_BUILD_BUTTON_ID + 7))
      << (Builder<Button>(px(136), px(136), px(54), px(54)) << Widget::id(FIRST_BUILD_BUTTON_ID + 8));
  fw::Framework::get_instance()->get_gui()->attach_widget(wnd_);

  for (int i = 0; i < 9; i++) {
    int id = FIRST_BUILD_BUTTON_ID + i;
    Button *btn = wnd_->find<Button>(id);
    btn->sig_mouse_over.connect(std::bind(&BuildWindow::on_mouse_over_button, this, id));
    btn->sig_mouse_out.connect(std::bind(&BuildWindow::on_mouse_out_button, this, id));
    btn->set_on_click(std::bind(&BuildWindow::on_build_clicked, this, _1, id));
  }
}

void BuildWindow::show() {
  wnd_->set_visible(true);
}

void BuildWindow::hide() {
  wnd_->set_visible(false);
}

void BuildWindow::refresh(std::weak_ptr<ent::Entity> entity, std::string build_group) {
  entity_ = entity;
  build_group_ = build_group;
  require_refresh_ = true;
}

bool BuildWindow::on_build_clicked(Widget *w, int id) {
  Button *btn = dynamic_cast<Button *>(w);
  auto iconp = boost::any_cast<std::shared_ptr<EntityIcon>>(&btn->get_data());
  if (iconp != nullptr) {
    std::string tmpl_name = (*iconp)->get_template_name();
    std::shared_ptr<ent::Entity> Entity(entity_);
    if (Entity) {
      Entity->get_component<ent::BuilderComponent>()->build(tmpl_name);
    }
  }

  return true;
}

void BuildWindow::on_mouse_over_button(int id) {
  mouse_over_button_id_ = id;
}

void BuildWindow::on_mouse_out_button(int id) {
  if (mouse_over_button_id_ == id) {
    Button *btn = wnd_->find<Button>(mouse_over_button_id_);
    if (btn != nullptr) {
      auto iconp = boost::any_cast<std::shared_ptr<EntityIcon>>(&btn->get_data());
      if (iconp != nullptr) {
        (*iconp)->reset();
      }
    }

    mouse_over_button_id_ = -1;
  }
}

void BuildWindow::do_refresh() {
  std::vector<fw::lua::Value> templates;
  ent::EntityFactory ent_factory;
  ent_factory.get_buildable_templates(build_group_, templates);

  int index = 0;
  for(auto &tmpl : templates) {
    fw::debug << " checking " << std::string(tmpl["name"]) << std::endl;
    Button *btn = wnd_->find<Button>(FIRST_BUILD_BUTTON_ID + index);
    if (btn == nullptr) {
      continue; // TODO
    }

    auto iconp = boost::any_cast<std::shared_ptr<EntityIcon>>(&btn->get_data());
    std::shared_ptr<EntityIcon> icon;
    if (iconp != nullptr) {
      icon = *iconp;
    } else {
      icon = std::shared_ptr<EntityIcon>(new EntityIcon());
      btn->set_data(icon);
      fw::Framework::get_instance()->get_graphics()->run_on_render_thread([=]() {
        icon->initialize();
        btn->set_icon(icon->get_drawable());
      });
    }

    std::string mesh_file_name = tmpl["components"]["Mesh"]["FileName"];
    std::string tmpl_name = tmpl["name"];
    fw::Framework::get_instance()->get_graphics()->run_on_render_thread([=]() {
      std::shared_ptr<fw::Model> mdl =
          fw::Framework::get_instance()->get_model_manager()->get_model(mesh_file_name);
//      mdl->set_color(game::SimulationThread::get_instance()->get_local_player()->get_color());
      icon->set_model(tmpl_name, mdl);
    });

    index ++;
  }
}

void BuildWindow::update() {
  if (require_refresh_) {
    do_refresh();
    require_refresh_ = false;
  }

  // if the mouse is over a button, call the icon's update so that it rotates the icon.
  if (mouse_over_button_id_ > 0) {
    Button *btn = wnd_->find<Button>(mouse_over_button_id_);
    if (btn != nullptr) {
      auto icon = boost::any_cast<std::shared_ptr<EntityIcon>>(&btn->get_data());
      if (icon != nullptr) {
        (*icon)->update();
      }
    }
  }
}

}
