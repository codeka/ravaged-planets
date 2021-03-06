#include <functional>
#include <boost/foreach.hpp>

#include <framework/framework.h>
#include <framework/input.h>
#include <framework/graphics.h>
#include <framework/scenegraph.h>
#include <framework/shader.h>
#include <framework/gui/builder.h>
#include <framework/gui/checkbox.h>
#include <framework/gui/gui.h>
#include <framework/gui/label.h>
#include <framework/gui/window.h>
#include <framework/gui/widget.h>

#include <game/entities/entity.h>
#include <game/entities/entity_debug.h>
#include <game/entities/entity_manager.h>
#include <game/entities/position_component.h>
#include <game/entities/moveable_component.h>

using namespace std::placeholders;
using namespace fw::gui;

namespace ent {

enum ids {
  SHOW_STEERING_ID = 3642,
  POSITION_ID,
  GOAL_ID,
};

entity_debug::entity_debug(entity_manager *mgr) :
    _mgr(mgr), _just_shown(false), _wnd(nullptr) {
}

entity_debug::~entity_debug() {
  fw::framework::get_instance()->get_gui()->detach_widget(_wnd);
}

void entity_debug::initialize() {
  _wnd = builder<window>(px(10), px(10), px(200), px(106))
      << window::background("frame") << widget::visible(false)
      << (builder<checkbox>(px(10), px(10), sum(pct(100), px(-20)), px(26))
          << checkbox::text("Show steering") << widget::id(SHOW_STEERING_ID)
          << widget::click(std::bind(&entity_debug::on_show_steering_changed, this, _1)))
      << (builder<label>(px(10), px(46), sum(pct(100), px(-20)), px(20))
          << label::text("Pos: ") << widget::id(POSITION_ID))
      << (builder<label>(px(10), px(76), sum(pct(100), px(-20)), px(20))
          << label::text("Goal: ") << widget::id(GOAL_ID))
      ;
  fw::framework::get_instance()->get_gui()->attach_widget(_wnd);

  fw::input *inp = fw::framework::get_instance()->get_input();
  inp->bind_key("Ctrl+D", std::bind(&entity_debug::on_key_press, this, _1, _2));
}

void entity_debug::update() {
  if (_wnd == nullptr)
    initialize();

  std::string new_pos_value;
  std::string new_goal_value;

  std::list<std::weak_ptr<entity> > selection = _mgr->get_selection();
  if (selection.size() > 0) {
    // we display some info about the selected entity (if there's more than one, we just use the first one from the
    // list - essentially a random one)
    std::shared_ptr<entity> entity = selection.front().lock();
    if (!entity) {
      return;
    }

    position_component *pos = entity->get_component<position_component>();

    if (pos != 0) {
      new_pos_value = (boost::format("Pos: (%1$.1f, %2$.1f, %3$.1f)")
          % pos->get_position()[0] % pos->get_position()[1] % pos->get_position()[2]).str();
    }

    moveable_component *moveable = entity->get_component<moveable_component>();
    if (moveable != nullptr) {
      fw::vector goal = moveable->get_goal();
      new_goal_value = (boost::format("Goal: (%1$.1f, %2$.1f, %3$.1f)") % goal[0] % goal[1] % goal[2]).str();
    }
  }

  _wnd->find<label>(POSITION_ID)->set_text(new_pos_value);
  _wnd->find<label>(GOAL_ID)->set_text(new_goal_value);
}

void entity_debug::on_key_press(std::string /*key*/, bool is_down) {
  if (is_down && !_just_shown) {
    _wnd->set_visible(!_wnd->is_visible());
    _just_shown = true;
  } else if (!is_down) {
    _just_shown = false;
  }
}

bool entity_debug::on_show_steering_changed(widget *w) {
  checkbox *cbx = dynamic_cast<checkbox *>(w);

  BOOST_FOREACH(std::weak_ptr<entity> const &wp, _mgr->get_selection()) {
    std::shared_ptr<entity> ent = wp.lock();
    if (!ent) {
      continue;
    }

    entity_debug_flags flags = ent->get_debug_flags();
    if (cbx->is_checked()) {
      flags = static_cast<entity_debug_flags>(flags | debug_show_steering);
    } else {
      flags = static_cast<entity_debug_flags>(flags & ~debug_show_steering);
    }
    ent->set_debug_flags(flags);
  }

  return true;
}

//-------------------------------------------------------------------------

entity_debug_view::entity_debug_view() {
}

entity_debug_view::~entity_debug_view() {
}

void entity_debug_view::add_line(fw::vector const &from, fw::vector const &to, fw::colour const &col) {
  line l;
  l.from = from;
  l.to = to;
  l.col = col;

  _lines.push_back(l);
}

void entity_debug_view::add_circle(fw::vector const &centre, float radius, fw::colour const &col) {
  // the number of segments is basically the diameter of our circle. That means
  // we'll have one segment per unit, approximately.
  int num_segments = (int) (2.0f * M_PI * radius);

  // at least 8 segments, though...
  if (num_segments < 8)
    num_segments = 8;

  fw::vector last_point;
  for (int i = 0; i < num_segments; i++) {
    float factor = 2.0f * (float) M_PI * (i / (float) num_segments);

    fw::vector point(centre[0] + radius * sin(factor), centre[1], centre[2] + radius * cos(factor));
    if (i > 0) {
      add_line(last_point, point, col);
    }
    last_point = point;
  }

  fw::vector first_point(centre[0] + radius * sin(0.0f), centre[1], centre[2] + radius * cos(0.0f));
  add_line(last_point, first_point, col);
}

void entity_debug_view::render(fw::sg::scenegraph &scenegraph, fw::matrix const &transform) {
  if (_lines.size() == 0)
    return;

  std::vector<line> lines_copy = _lines;
  _lines.clear();

  fw::vertex::xyz_c *vertices = new fw::vertex::xyz_c[lines_copy.size() * 2];

  for (int i = 0; i < static_cast<int>(lines_copy.size()); i++) {
    line const &l = lines_copy[i];

    vertices[i * 2].x = l.from[0];
    vertices[i * 2].y = l.from[1];
    vertices[i * 2].z = l.from[2];
    vertices[i * 2].colour = l.col.to_rgba();

    vertices[(i * 2) + 1].x = l.to[0];
    vertices[(i * 2) + 1].y = l.to[1];
    vertices[(i * 2) + 1].z = l.to[2];
    vertices[(i * 2) + 1].colour = l.col.to_rgba();
  }

  std::shared_ptr<fw::vertex_buffer> vb = fw::vertex_buffer::create<fw::vertex::xyz_c>();
  vb->set_data(lines_copy.size() * 2, vertices);
  delete[] vertices;

  std::shared_ptr<fw::shader> shader(fw::shader::create("basic.shader"));
  std::shared_ptr<fw::shader_parameters> shader_params = shader->create_parameters();
  shader_params->set_program_name("notexture");

  std::shared_ptr<fw::sg::node> node(new fw::sg::node());
  node->set_world_matrix(transform);
  node->set_vertex_buffer(vb);
  node->set_primitive_type(fw::sg::primitive_linelist);
  node->set_shader(shader);
  node->set_shader_parameters(shader_params);
  node->set_cast_shadows(false);
  scenegraph.add_node(node);
}

}
