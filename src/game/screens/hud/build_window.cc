
#include <memory>
#include <boost/foreach.hpp>

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
#include <framework/model.h>
#include <framework/model_manager.h>
#include <framework/paths.h>
#include <framework/scenegraph.h>
#include <framework/texture.h>
#include <framework/timer.h>
#include <framework/vector.h>

#include <game/entities/entity.h>
#include <game/entities/entity_factory.h>
#include <game/entities/builder_component.h>
#include <game/entities/mesh_component.h>
#include <game/screens/hud/build_window.h>
#include <game/simulation/local_player.h>
#include <game/simulation/simulation_thread.h>

using namespace fw::gui;
using namespace std::placeholders;

namespace game {

build_window *hud_build = nullptr;

enum ids {
  FIRST_BUILD_BUTTON_ID = 34636,
};

//-----------------------------------------------------------------------------

/**
 * Holds information nessecary to render an icon for a given entity.
 */
class entity_icon {
private:
  float _rotation;
  std::string _template_name;
  std::shared_ptr<fw::model> _model;
  std::shared_ptr<fw::framebuffer> _framebuffer;
  std::shared_ptr<fw::texture> _colour_texture;
  std::shared_ptr<fw::texture> _depth_texture;
  std::shared_ptr<Drawable> _drawable;

  void render();
public:
  entity_icon();

  void initialize();
  void set_model(std::string const &template_name, std::shared_ptr<fw::model> mdl);
  void update();
  void reset();

  std::string get_template_name();
  std::shared_ptr<Drawable> get_drawable();
};

entity_icon::entity_icon() : _rotation(0.0f) {
}

void entity_icon::initialize() {
  _colour_texture = std::shared_ptr<fw::texture>(new fw::texture());
  _colour_texture->create(64, 64, false);

  _depth_texture = std::shared_ptr<fw::texture>(new fw::texture());
  _depth_texture->create(64, 64, true);

  _framebuffer = std::shared_ptr<fw::framebuffer>(new fw::framebuffer());
  _framebuffer->set_colour_buffer(_colour_texture);
  _framebuffer->set_depth_buffer(_depth_texture);

  _drawable = fw::framework::get_instance()->get_gui()->get_drawable_manager()
      ->build_drawable(_colour_texture, 7, 7, 50, 50);
  std::dynamic_pointer_cast<BitmapDrawable>(_drawable)->set_flipped(true);
  render();
}

void entity_icon::set_model(std::string const &template_name, std::shared_ptr<fw::model> mdl) {
  _template_name = template_name;
  _model = mdl;
  _rotation = 0.0f;
  render();
}

void entity_icon::render() {
  if (!_model) {
    return;
  }

  fw::sg::scenegraph sg;
  sg.set_clear_colour(fw::colour(0, 0, 0, 0));

  fw::lookat_camera cam;
  cam.set_distance(2.0f);
  cam.update(1.0f / 30.0f);
  sg.push_camera(&cam);

  fw::vector sun(0.485f, 0.485f, 0.727f);
  std::shared_ptr <fw::sg::light> light(new fw::sg::light(sun * 200.0f, sun * -1, true));
  sg.add_light(light);

  _model->render(sg, fw::rotate_axis_angle(fw::vector(0, 1, 0), _rotation));
  fw::render(sg, _framebuffer, false);
}

void entity_icon::update() {
  _rotation += 3.14159f * fw::framework::get_instance()->get_timer()->get_frame_time();
  fw::framework::get_instance()->get_graphics()->run_on_render_thread([=]() {
    render();
  });
}

void entity_icon::reset() {
  _rotation = 0.0f;
  fw::framework::get_instance()->get_graphics()->run_on_render_thread([=]() {
    render();
  });
}

std::string entity_icon::get_template_name() {
  return _template_name;
}

std::shared_ptr<Drawable> entity_icon::get_drawable() {
  return _drawable;
}

//-----------------------------------------------------------------------------

build_window::build_window() : _wnd(nullptr), _require_refresh(false), _mouse_over_button_id(-1) {
}

build_window::~build_window() {
  fw::framework::get_instance()->get_gui()->detach_widget(_wnd);
}

void build_window::initialize() {
  _wnd = Builder<Window>(sum(pct(100), px(-210)), px(220), px(200), px(200))
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
  fw::framework::get_instance()->get_gui()->attach_widget(_wnd);

  for (int i = 0; i < 9; i++) {
    int id = FIRST_BUILD_BUTTON_ID + i;
    Button *btn = _wnd->find<Button>(id);
    btn->sig_mouse_over.connect(std::bind(&build_window::on_mouse_over_button, this, id));
    btn->sig_mouse_out.connect(std::bind(&build_window::on_mouse_out_button, this, id));
    btn->set_on_click(std::bind(&build_window::on_build_clicked, this, _1, id));
  }
}

void build_window::show() {
  _wnd->set_visible(true);
}

void build_window::hide() {
  _wnd->set_visible(false);
}

void build_window::refresh(std::weak_ptr<ent::entity> entity, std::string build_group) {
  _entity = entity;
  _build_group = build_group;
  _require_refresh = true;
}

bool build_window::on_build_clicked(Widget *w, int id) {
  Button *btn = dynamic_cast<Button *>(w);
  auto iconp = boost::any_cast<std::shared_ptr<entity_icon>>(&btn->get_data());
  if (iconp != nullptr) {
    std::string tmpl_name = (*iconp)->get_template_name();
    std::shared_ptr<ent::entity> entity(_entity);
    if (entity) {
      entity->get_component<ent::builder_component>()->build(tmpl_name);
    }
  }

  return true;
}

void build_window::on_mouse_over_button(int id) {
  _mouse_over_button_id = id;
}

void build_window::on_mouse_out_button(int id) {
  if (_mouse_over_button_id == id) {
    Button *btn = _wnd->find<Button>(_mouse_over_button_id);
    if (btn != nullptr) {
      auto iconp = boost::any_cast<std::shared_ptr<entity_icon>>(&btn->get_data());
      if (iconp != nullptr) {
        (*iconp)->reset();
      }
    }

    _mouse_over_button_id = -1;
  }
}

void build_window::do_refresh() {
  std::vector<luabind::object> templates;
  ent::entity_factory ent_factory;
  ent_factory.get_buildable_templates(_build_group, templates);

  int index = 0;
  BOOST_FOREACH(luabind::object const &tmpl, templates) {
    fw::debug << " checking " << ""/*tmpl["name"]*/ << std::endl;
    Button *btn = _wnd->find<Button>(FIRST_BUILD_BUTTON_ID + index);
    if (btn == nullptr) {
      continue; // TODO
    }

    auto iconp = boost::any_cast<std::shared_ptr<entity_icon>>(&btn->get_data());
    std::shared_ptr<entity_icon> icon;
    if (iconp != nullptr) {
      icon = *iconp;
    } else {
      icon = std::shared_ptr<entity_icon>(new entity_icon());
      btn->set_data(icon);
      fw::framework::get_instance()->get_graphics()->run_on_render_thread([=]() {
        icon->initialize();
        btn->set_icon(icon->get_drawable());
      });
    }

    std::string mesh_file_name = "";// luabind::object_cast<std::string>(tmpl["components"]["Mesh"]["FileName"]);
    fw::framework::get_instance()->get_graphics()->run_on_render_thread([=]() {
      std::shared_ptr<fw::model> mdl =
          fw::framework::get_instance()->get_model_manager()->get_model(mesh_file_name);
      mdl->set_colour(game::simulation_thread::get_instance()->get_local_player()->get_colour());
      icon->set_model(""/*luabind::object_cast<std::string>(tmpl["name"])*/, mdl);
    });

    index ++;
  }
}

void build_window::update() {
  if (_require_refresh) {
    do_refresh();
    _require_refresh = false;
  }

  // if the mouse is over a button, call the icon's update so that it rotates the icon.
  if (_mouse_over_button_id > 0) {
    Button *btn = _wnd->find<Button>(_mouse_over_button_id);
    if (btn != nullptr) {
      auto icon = boost::any_cast<std::shared_ptr<entity_icon>>(&btn->get_data());
      if (icon != nullptr) {
        (*icon)->update();
      }
    }
  }
}

}
