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
#include <game/entities/mesh_component.h>
#include <game/entities/moveable_component.h>
#include <game/world/terrain.h>
#include <game/world/world.h>

using namespace std::placeholders;
using namespace fw::gui;

namespace ent {

enum ids {
  SHOW_STEERING_ID = 3642,
  POSITION_ID,
  GOAL_ID,
  CURSOR_ID,
};

EntityDebug::EntityDebug(EntityManager *mgr) :
    mgr_(mgr), just_shown_(false), wnd_(nullptr) {
}

EntityDebug::~EntityDebug() {
  fw::Framework::get_instance()->get_gui()->detach_widget(wnd_);
}

void EntityDebug::initialize() {
  wnd_ = Builder<Window>(px(10), px(10), px(200), px(136))
      << Window::background("frame") << Widget::visible(false)
      << (Builder<Checkbox>(px(10), px(10), sum(pct(100), px(-20)), px(26))
          << Checkbox::text("Show steering") << Widget::id(SHOW_STEERING_ID)
          << Widget::click(std::bind(&EntityDebug::on_show_steering_changed, this, _1)))
      << (Builder<Label>(px(10), px(46), sum(pct(100), px(-20)), px(20))
          << Label::text("Pos: ") << Widget::id(POSITION_ID))
      << (Builder<Label>(px(10), px(76), sum(pct(100), px(-20)), px(20))
          << Label::text("Goal: ") << Widget::id(GOAL_ID))
      << (Builder<Label>(px(10), px(106), sum(pct(100), px(-20)), px(20))
          << Label::text("Cursor: ") << Window::id(CURSOR_ID))
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
  std::string new_cursor_value;

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

  auto cursor = game::World::get_instance()->get_terrain()->get_cursor_location();
  new_cursor_value = (boost::format("Cursor: (%1$.1f, %2$.1f, %3$.1f)") % cursor[0] % cursor[1] % cursor[2]).str();

  wnd_->find<Label>(POSITION_ID)->set_text(new_pos_value);
  wnd_->find<Label>(GOAL_ID)->set_text(new_goal_value);
  wnd_->find<Label>(CURSOR_ID)->set_text(new_cursor_value);
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

EntityDebugView::EntityDebugView(Entity* entity) {
  mesh_component_ = entity->get_component<MeshComponent>();
}

EntityDebugView::~EntityDebugView() {
  if (sg_node_) {
    fw::Framework::get_instance()->get_scenegraph_manager()->enqueue(
      [sg_node = sg_node_](fw::sg::Scenegraph& scenegraph) {
        scenegraph.remove_node(sg_node);
      });
  }
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
  // we'll have one segment per unit, approximately. At least 8, though.
  const int num_segments = std::max(8, (int) (2.0f * M_PI * radius));

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

void EntityDebugView::update(float dt) {
  if (lines_.size() == 0) {
    if (sg_node_) {
      sg_node_->set_enabled(false);
    }
    return;
  }

  const int vertices_size = static_cast<int>(lines_.size() * 2);
  fw::vertex::xyz_c* vertices = new fw::vertex::xyz_c[vertices_size];
  for (int i = 0; i < vertices_size; i += 2) {
    Line const& l = lines_[i / 2];

    vertices[i].x = l.from[0];
    vertices[i].y = l.from[1];
    vertices[i].z = l.from[2];
    vertices[i].color = l.col.to_rgba();

    vertices[i + 1].x = l.to[0];
    vertices[i + 1].y = l.to[1];
    vertices[i + 1].z = l.to[2];
    vertices[i + 1].color = l.col.to_rgba();
  }
  lines_.clear();

  fw::Framework::get_instance()->get_scenegraph_manager()->enqueue(
    [&sg_node = sg_node_, mesh_component = mesh_component_, vertices, vertices_size](fw::sg::Scenegraph& scenegraph) {
      if (!sg_node) {
        sg_node = std::make_shared<fw::sg::Node>();

        std::shared_ptr<fw::Shader> shader(fw::Shader::create("basic.shader"));
        std::shared_ptr<fw::ShaderParameters> shader_params = shader->create_parameters();
        shader_params->set_program_name("notexture");

        sg_node->set_primitive_type(fw::sg::PrimitiveType::kLineList);
        sg_node->set_shader(shader);
        sg_node->set_shader_parameters(shader_params);
        sg_node->set_cast_shadows(false);
        sg_node->set_world_matrix(fw::translation(0.0f, 0.2f, 0.0f));

        scenegraph.add_node(sg_node);
      }
      
      std::shared_ptr<fw::VertexBuffer> vb = fw::VertexBuffer::create<fw::vertex::xyz_c>();
      vb->set_data(vertices_size, vertices);
      delete[] vertices;

      sg_node->set_vertex_buffer(vb);
    });
}

}
