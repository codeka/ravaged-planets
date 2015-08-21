
#include <memory>
#include <boost/foreach.hpp>

#include <framework/framework.h>
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
#include <framework/scenegraph.h>
#include <framework/texture.h>
#include <framework/timer.h>

#include <game/entities/entity.h>
#include <game/entities/entity_factory.h>
#include <game/entities/mesh_component.h>
#include <game/screens/hud/build_window.h>

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
  std::shared_ptr<fw::model> _model;
  std::shared_ptr<fw::framebuffer> _framebuffer;
  std::shared_ptr<drawable> _drawable;
  bool _render_queued;

  void queue_render();
public:
  entity_icon();

  void initialize();
  void set_model(std::shared_ptr<fw::model> mdl);
  void render();

  std::shared_ptr<drawable> get_drawable();
};

entity_icon::entity_icon() : _rotation(0.0f), _render_queued(false) {
}

void entity_icon::initialize() {
  std::shared_ptr<fw::texture> colour_target(new fw::texture());
  colour_target->create(54, 54, false);

  std::shared_ptr<fw::texture> depth_target(new fw::texture());
  depth_target->create(54, 54, true);

  _framebuffer = std::shared_ptr<fw::framebuffer>(new fw::framebuffer());
  _framebuffer->set_colour_buffer(colour_target);
  _framebuffer->set_depth_buffer(depth_target);

  _drawable = fw::framework::get_instance()->get_gui()->get_drawable_manager()
      ->build_drawable(colour_target, 0, 0, 54, 54);
  render();
}

void entity_icon::set_model(std::shared_ptr<fw::model> mdl) {
  _model = mdl;
  _rotation = 0.0f;
  render();
}

void entity_icon::render() {
  _render_queued = false;
  if (!_model) {
    return;
  }

  _rotation += 3.14159f * fw::framework::get_instance()->get_timer()->get_frame_time();

  fw::lookat_camera cam;
  cam.set_distance(2.0f);
  cam.update(1.0f / 30.0f);

  fw::camera *old_cam = fw::framework::get_instance()->get_camera();
  fw::framework::get_instance()->set_camera(&cam);

  fw::sg::scenegraph sg;
  _model->render(sg, fw::rotate_axis_angle(fw::vector(0, 1, 0), _rotation));

  fw::render(sg, _framebuffer, false);
  fw::framework::get_instance()->set_camera(old_cam);
}

void entity_icon::queue_render() {
  if (_render_queued) {
    return;
  }

  fw::framework::get_instance()->get_graphics()->run_on_render_thread([=]() {
    render();
  });
}

std::shared_ptr<drawable> entity_icon::get_drawable() {
  return _drawable;
}

//-----------------------------------------------------------------------------

build_window::build_window() : _wnd(nullptr), _require_refresh(false) {
}

build_window::~build_window() {
  fw::framework::get_instance()->get_gui()->detach_widget(_wnd);
}

void build_window::initialize() {
  _wnd = builder<window>(sum(pct(100), px(-210)), px(220), px(200), px(200))
      << window::background("frame") << widget::visible(false)
      << (builder<button>(px(10), px(10), px(54), px(54)) << widget::id(FIRST_BUILD_BUTTON_ID + 0))
      << (builder<button>(px(73), px(10), px(54), px(54)) << widget::id(FIRST_BUILD_BUTTON_ID + 1))
      << (builder<button>(px(136), px(10), px(54), px(54)) << widget::id(FIRST_BUILD_BUTTON_ID + 2))
      << (builder<button>(px(10), px(73), px(54), px(54)) << widget::id(FIRST_BUILD_BUTTON_ID + 3))
      << (builder<button>(px(73), px(73), px(54), px(54)) << widget::id(FIRST_BUILD_BUTTON_ID + 4))
      << (builder<button>(px(136), px(73), px(54), px(54)) << widget::id(FIRST_BUILD_BUTTON_ID + 5))
      << (builder<button>(px(10), px(136), px(54), px(54)) << widget::id(FIRST_BUILD_BUTTON_ID + 6))
      << (builder<button>(px(73), px(136), px(54), px(54)) << widget::id(FIRST_BUILD_BUTTON_ID + 7))
      << (builder<button>(px(136), px(136), px(54), px(54)) << widget::id(FIRST_BUILD_BUTTON_ID + 8));
  fw::framework::get_instance()->get_gui()->attach_widget(_wnd);
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

void build_window::do_refresh() {
  std::vector<std::shared_ptr<ent::entity_template>> templates;
  ent::entity_factory ent_factory;
  ent_factory.get_buildable_templates(_build_group, templates);

  int index = 0;
  BOOST_FOREACH(std::shared_ptr<ent::entity_template> tmpl, templates) {
    button *btn = _wnd->find<button>(FIRST_BUILD_BUTTON_ID + index);
    if (btn == nullptr) {
      continue; // TODO
    }

    std::shared_ptr<entity_icon> const *iconp = boost::any_cast<std::shared_ptr<entity_icon>>(&btn->get_data());
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

    BOOST_FOREACH(auto comp_tmpl, tmpl->components) {
      if (comp_tmpl->identifier == ent::mesh_component::identifier) {
        fw::framework::get_instance()->get_graphics()->run_on_render_thread([=]() {
          ent::mesh_component mesh_comp;
          mesh_comp.apply_template(comp_tmpl);
          icon->set_model(fw::framework::get_instance()->get_model_manager()->get_model(mesh_comp.get_model_name()));
        });
        break;
      }
    }

    index ++;
  }
}

void build_window::update() {
  if (_require_refresh) {
    do_refresh();
    _require_refresh = false;
  }
}

}
