
#include <boost/format.hpp>

#include <framework/debug_view.h>
#include <framework/framework.h>
#include <framework/gui/builder.h>
#include <framework/gui/gui.h>
#include <framework/gui/label.h>
#include <framework/gui/widget.h>
#include <framework/gui/window.h>
#include <framework/particle_manager.h>
#include <framework/settings.h>
#include <framework/timer.h>

namespace fw {
using namespace gui;

enum ids {
  FPS_ID = 308724,
  PARTICLES_ID,
};

debug_view::debug_view() : _wnd(nullptr), _time_to_update(9999.9f) {
}

debug_view::~debug_view() {
}

void debug_view::initialize() {
  settings stg;
  if (stg.is_set("debug-view")) {
    _time_to_update = 1.0f;

    _wnd = Builder<Window>(sum(pct(100), px(-200)), sum(pct(100), px(-50)), px(190), px(40))
      << (Builder<Label>(px(0), px(0), px(190), px(20)) << Label::text_align(Label::Alignment::kRight) << Widget::id(FPS_ID))
      << (Builder<Label>(px(0), px(20), px(190), px(20)) << Label::text_align(Label::Alignment::kRight) << Widget::id(PARTICLES_ID));
    framework::get_instance()->get_gui()->attach_widget(_wnd);
  }
}

void debug_view::destroy() {
  if (_wnd != nullptr) {
    framework::get_instance()->get_gui()->detach_widget(_wnd);
  }
}

void debug_view::update(float dt) {
  if (_wnd == nullptr) {
    return;
  }

  _time_to_update -= dt;
  if (_time_to_update <= 0.0f) {
    fw::framework *frmwrk = fw::framework::get_instance();

    Label *fps = _wnd->find<Label>(FPS_ID);
    fps->set_text((boost::format("%1% fps") % frmwrk->get_timer()->get_fps()).str());

    Label *particles = _wnd->find<Label>(PARTICLES_ID);
    particles->set_text((boost::format("%1% particles") % frmwrk->get_particle_mgr()->get_num_active_particles()).str());

    _time_to_update = 1.0f;
  }
}

}



