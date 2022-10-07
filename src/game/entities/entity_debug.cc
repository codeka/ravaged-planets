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

EntityDebug::EntityDebug(EntityManager *mgr) :
    mgr_(mgr), just_shown_(false), wnd_(nullptr) {
}

EntityDebug::~EntityDebug() {
  fw::Framework::get_instance()->get_gui()->detach_widget(wnd_);
}

void EntityDebug::initialize() {
  wnd_ = Builder<Window>(px(10), px(10), px(200), px(106))
      << Window::background("frame") << Widget::visible(false)
      << (Builder<Checkbox>(px(10), px(10), sum(pct(100), px(-20)), px(26))
          << Checkbox::text("Show steering") << Widget::id(SHOW_STEERING_ID)
          << Widget::click(std::bind(&EntityDebug::on_show_steering_changed, this, _1)))
      << (Builder<Label>(px(10), px(46), sum(pct(100), px(-20)), px(20))
          << Label::text("Pos: ") << Widget::id(POSITION_ID))
      << (Builder<Label>(px(10), px(76), sum(pct(100), px(-20)), px(20))
          << Label::text("Goal: ") << Widget::id(GOAL_ID))
      ;
  fw::Framework::get_instance()->get_gui()->attach_widget(wnd_);

  fw::Input *inp = fw::Framework::get_instance()->get_input();
  inp->bind_key("Ctrl+D", std::bind(&EntityDebug::on_key_press, this, _1, _2));
}

void EntityDebug::update() {
  if (wnd_ == nullptr)
    initialize();

  std::string new_pos_value;
  std::string new_goal_value;

  std::list<std::weak_ptr<Entity> > selection = mgr_->get_selection();
  if (selection.size() > 0) {
    // we display some info about the selected Entity (if there's more than one, we just use the first one from the
    // list - essentially a random one)
    std::shared_ptr<Entity> entity = selection.front().lock();
    if (!entity) {
      return;
    }

    PositionComponent *pos = entity->get_component<PositionComponent>();

    if (pos != 0) {
      new_pos_value = (boost::format("Pos: (%1$.1f, %2$.1f, %3$.1f)")
          % pos->get_position()[0] % pos->get_position()[1] % pos->get_position()[2]).str();
    }

    MoveableComponent *moveable = entity->get_component<MoveableComponent>();
    if (moveable != nullptr) {
      fw::Vector goal = moveable->get_goal();
      new_goal_value = (boost::format("Goal: (%1$.1f, %2$.1f, %3$.1f)") % goal[0] % goal[1] % goal[2]).str();
    }
  }

  wnd_->find<Label>(POSITION_ID)->set_text(new_pos_value);
  wnd_->find<Label>(GOAL_ID)->set_text(new_goal_value);
}

void EntityDebug::on_key_press(std::string /*key*/, bool is_down) {
  if (is_down && !just_shown_) {
    wnd_->set_visible(!wnd_->is_visible());
    just_shown_ = true;
  } else if (!is_down) {
    just_shown_ = false;
  }
}

bool EntityDebug::on_show_steering_changed(Widget *w) {
  Checkbox *cbx = dynamic_cast<Checkbox *>(w);

  for(std::weak_ptr<Entity> const &wp : mgr_->get_selection()) {
    std::shared_ptr<Entity> ent = wp.lock();
    if (!ent) {
      continue;
    }

    EntityDebugFlags flags = ent->get_debug_flags();
    if (cbx->is_checked()) {
      flags = static_cast<EntityDebugFlags>(flags | kDebugShowSteering);
    } else {
      flags = static_cast<EntityDebugFlags>(flags & ~kDebugShowSteering);
    }
    ent->set_debug_flags(flags);
  }

  return true;
}

//-------------------------------------------------------------------------

EntityDebugView::EntityDebugView() {
}

EntityDebugView::~EntityDebugView() {
}

void EntityDebugView::add_line(fw::Vector const &from, fw::Vector const &to, fw::Color const &col) {
  Line l;
  l.from = from;
  l.to = to;
  l.col = col;

  lines_.push_back(l);
}

void EntityDebugView::add_circle(fw::Vector const &center, float radius, fw::Color const &col) {
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

void EntityDebugView::render(fw::sg::Scenegraph &scenegraph, fw::Matrix const &transform) {
  if (lines_.size() == 0)
    return;

  std::vector<Line> lines_copy = lines_;
  lines_.clear();

  const int vertices_size = static_cast<int>(lines_copy.size() * 2);
  fw::vertex::xyz_c *vertices = new fw::vertex::xyz_c[vertices_size];

  for (int i = 0; i < vertices_size; i += 2) {
    Line const &l = lines_copy[i / 2];

    vertices[i].x = l.from[0];
    vertices[i].y = l.from[1];
    vertices[i].z = l.from[2];
    vertices[i].color = l.col.to_rgba();

    vertices[i + 1].x = l.to[0];
    vertices[i + 1].y = l.to[1];
    vertices[i + 1].z = l.to[2];
    vertices[i + 1].color = l.col.to_rgba();
  }

  std::shared_ptr<fw::VertexBuffer> vb = fw::VertexBuffer::create<fw::vertex::xyz_c>();
  vb->set_data(vertices_size, vertices);
  delete[] vertices;

  std::shared_ptr<fw::Shader> shader(fw::Shader::create("basic.shader"));
  std::shared_ptr<fw::ShaderParameters> shader_params = shader->create_parameters();
  shader_params->set_program_name("notexture");

  std::shared_ptr<fw::sg::Node> node(new fw::sg::Node());
  node->set_world_matrix(transform);
  node->set_vertex_buffer(vb);
  node->set_primitive_type(fw::sg::PrimitiveType::kLineList);
  node->set_shader(shader);
  node->set_shader_parameters(shader_params);
  node->set_cast_shadows(false);
  scenegraph.add_node(node);
}

}
