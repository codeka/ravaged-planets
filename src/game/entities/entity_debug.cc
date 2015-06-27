#include <functional>
#include <boost/foreach.hpp>

#include <framework/framework.h>
#include <framework/input.h>
#include <framework/graphics.h>
#include <framework/scenegraph.h>
#include <framework/gui/gui.h>
#include <framework/gui/window.h>
#include <framework/gui/widget.h>

#include <game/entities/entity.h>
#include <game/entities/entity_debug.h>
#include <game/entities/entity_manager.h>
#include <game/entities/position_component.h>
#include <game/entities/moveable_component.h>

using namespace std::placeholders;

namespace ent {

entity_debug::entity_debug(entity_manager *mgr) :
    _mgr(mgr), _just_shown(false), _wnd(nullptr) {
}

entity_debug::~entity_debug() {
}

void entity_debug::initialize() {/*
  fw::gui::cegui *gui = fw::framework::get_instance()->get_gui();
  _wnd = gui->get_window("DebugOptionsWindow");
  _sel_show_steering = dynamic_cast<CEGUI::Checkbox *>(_wnd->getChildRecursive(
      "DebugOptionsWindow/SelectedEntityOptions/ShowSteering"));
  _sel_position = _wnd->getChildRecursive("DebugOptionsWindow/SelectedEntityOptions/Position");
  _sel_goal = _wnd->getChildRecursive("DebugOptionsWindow/SelectedEntityOptions/Goal");
  _gbl_cam_position = _wnd->getChildRecursive("DebugOptionsWindow/GlobalOptions/CameraPosition");

//		subscribe(_sel_show_steering, CEGUI::Checkbox::EventCheckStateChanged,
//			CEGUI::SubscriberSlot(&entity_debug::on_show_steering_changed, this));
  _sel_show_steering->subscribeEvent(CEGUI::Checkbox::EventCheckStateChanged,
      CEGUI::SubscriberSlot(&entity_debug::on_show_steering_changed, this));
*/
  fw::input *inp = fw::framework::get_instance()->get_input();
  inp->bind_key("Ctrl+D", std::bind(&entity_debug::on_key_press, this, _1, _2));
}

void entity_debug::update() {
  if (_wnd == nullptr)
    initialize();

  std::list<std::weak_ptr<entity> > selection = _mgr->get_selection();
  if (selection.size() > 0) {
    // we display some info about the selected entity (if there's more than one, we just use the first one from the
    // list - essentially a random one)
    std::shared_ptr<entity> entity = selection.front().lock();
    if (!entity) {
      return;
    }

    position_component *pos = entity->get_component<position_component>();

    if (pos != 0) {/*
      _sel_position->setText(
          (boost::format("(%1$.1f, %2$.1f, %3$.1f)") % pos->get_position()[0] % pos->get_position()[1]
              % pos->get_position()[2]).str().c_str());
    */}

    moveable_component *moveable = entity->get_component<moveable_component>();
    if (moveable != nullptr) {/*
      fw::vector goal = moveable->get_goal();
      _sel_goal->setText((boost::format("(%1$.1f, %2$.1f, %3$.1f)") % goal[0] % goal[1] % goal[2]).str().c_str());
    */}
  }

/*  _gbl_cam_position->setText(
      (boost::format("(%1$.1f, %2$.1f, %3$.1f)") % _mgr->get_view_centre()[0] % _mgr->get_view_centre()[1]
          % _mgr->get_view_centre()[2]).str().c_str());*/
}

void entity_debug::on_key_press(std::string /*key*/, bool is_down) {
  if (is_down && !_just_shown) {
    _wnd->set_visible(!_wnd->is_visible());
    _just_shown = true;
  } else if (!is_down) {
    _just_shown = false;
  }
}

bool entity_debug::on_show_steering_changed(fw::gui::widget *w) {
  std::list<std::weak_ptr<entity>> selection = _mgr->get_selection();
  BOOST_FOREACH(std::weak_ptr<entity> const &wp, selection) {
    std::shared_ptr<entity> ent = wp.lock();
    if (!ent) {
      continue;
    }

    entity_debug_flags flags = ent->get_debug_flags();
    //if (_sel_show_steering->isSelected()) {
      flags = static_cast<entity_debug_flags>(flags | debug_show_steering);
    //} else {
    //  flags = static_cast<entity_debug_flags>(flags & ~debug_show_steering);
    //}
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

  fw::vertex::xyz_c *vertices = new fw::vertex::xyz_c[_lines.size() * 2];

  for (int i = 0; i < static_cast<int>(_lines.size()); i++) {
    line const &l = _lines[i];

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
  vb->set_data(_lines.size() * 2, vertices);
  delete[] vertices;

  std::shared_ptr<fw::sg::node> node(new fw::sg::node());
  node->set_world_matrix(transform);
  node->set_vertex_buffer(vb);
  node->set_primitive_type(fw::sg::primitive_linelist);
  node->set_cast_shadows(false);
  scenegraph.add_node(node);

  _lines.clear();
}

}
