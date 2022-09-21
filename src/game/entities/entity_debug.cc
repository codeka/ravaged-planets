#include <functional>

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
    mgr_(mgr), _just_shown(false), _wnd(nullptr) {
}

entity_debug::~entity_debug() {
  fw::framework::get_instance()->get_gui()->detach_widget(_wnd);
}

void entity_debug::initialize() {
  _wnd = Builder<Window>(px(10), px(10), px(200), px(106))
      << Window::background("frame") << Widget::visible(false)
      << (Builder<Checkbox>(px(10), px(10), sum(pct(100), px(-20)), px(26))
          << Checkbox::text("Show steering") << Widget::id(SHOW_STEERING_ID)
          << Widget::click(std::bind(&entity_debug::on_show_steering_changed, this, _1)))
      << (Builder<Label>(px(10), px(46), sum(pct(100), px(-20)), px(20))
          << Label::text("Pos: ") << Widget::id(POSITION_ID))
      << (Builder<Label>(px(10), px(76), sum(pct(100), px(-20)), px(20))
          << Label::text("Goal: ") << Widget::id(GOAL_ID))
      ;
  fw::framework::get_instance()->get_gui()->attach_widget(_wnd);

  fw::Input *inp = fw::framework::get_instance()->get_input();
  inp->bind_key("Ctrl+D", std::bind(&entity_debug::on_key_press, this, _1, _2));
}

void entity_debug::update() {
  if (_wnd == nullptr)
    initialize();

  std::string new_pos_value;
  std::string new_goal_value;

  std::list<std::weak_ptr<entity> > selection = mgr_->get_selection();
  if (selection.size() > 0) {
    // we display some info about the selected entity (if there's more than one, we just use the first one from the
    // list - essentially a Random one)
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
      fw::Vector goal = moveable->get_goal();
      new_goal_value = (boost::format("Goal: (%1$.1f, %2$.1f, %3$.1f)") % goal[0] % goal[1] % goal[2]).str();
    }
  }

  _wnd->find<Label>(POSITION_ID)->set_text(new_pos_value);
  _wnd->find<Label>(GOAL_ID)->set_text(new_goal_value);
}

void entity_debug::on_key_press(std::string /*key*/, bool is_down) {
  if (is_down && !_just_shown) {
    _wnd->set_visible(!_wnd->is_visible());
    _just_shown = true;
  } else if (!is_down) {
    _just_shown = false;
  }
}

bool entity_debug::on_show_steering_changed(Widget *w) {
  Checkbox *cbx = dynamic_cast<Checkbox *>(w);

  for(std::weak_ptr<entity> const &wp : mgr_->get_selection()) {
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

void entity_debug_view::add_line(fw::Vector const &from, fw::Vector const &to, fw::Color const &col) {
  line l;
  l.from = from;
  l.to = to;
  l.col = col;

  _lines.push_back(l);
}

void entity_debug_view::add_circle(fw::Vector const &center, float radius, fw::Color const &col) {
  // the number of segments is basically the diameter of our circle. That means
  // we'll have one segment per unit, approximately.
  int num_segments = (int) (2.0f * M_PI * radius);

  // at least 8 segments, though...
  if (num_segments < 8)
    num_segments = 8;

  fw::Vector last_point;
  for (int i = 0; i < num_segments; i++) {
    float factor = 2.0f * (float) M_PI * (i / (float) num_segments);

    fw::Vector point(center[0] + radius * sin(factor), center[1], center[2] + radius * cos(factor));
    if (i > 0) {
      add_line(last_point, point, col);
    }
    last_point = point;
  }

  fw::Vector first_point(center[0] + radius * sin(0.0f), center[1], center[2] + radius * cos(0.0f));
  add_line(last_point, first_point, col);
}

void entity_debug_view::render(fw::sg::Scenegraph &Scenegraph, fw::Matrix const &transform) {
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
    vertices[i * 2].color = l.col.to_rgba();

    vertices[(i * 2) + 1].x = l.to[0];
    vertices[(i * 2) + 1].y = l.to[1];
    vertices[(i * 2) + 1].z = l.to[2];
    vertices[(i * 2) + 1].color = l.col.to_rgba();
  }

  std::shared_ptr<fw::VertexBuffer> vb = fw::VertexBuffer::create<fw::vertex::xyz_c>();
  vb->set_data(lines_copy.size() * 2, vertices);
  delete[] vertices;

  std::shared_ptr<fw::Shader> Shader(fw::Shader::create("basic.shader"));
  std::shared_ptr<fw::ShaderParameters> shader_params = Shader->create_parameters();
  shader_params->set_program_name("notexture");

  std::shared_ptr<fw::sg::Node> Node(new fw::sg::Node());
  Node->set_world_matrix(transform);
  Node->set_vertex_buffer(vb);
  Node->set_primitive_type(fw::sg::PrimitiveType::kLineList);
  Node->set_shader(Shader);
  Node->set_shader_parameters(shader_params);
  Node->set_cast_shadows(false);
  Scenegraph.add_node(Node);
}

}
