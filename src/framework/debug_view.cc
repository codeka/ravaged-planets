
#include <absl/strings/str_cat.h>

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

DebugView::DebugView() : wnd_(nullptr), time_to_update_(9999.9f) {
}

DebugView::~DebugView() {
}

void DebugView::initialize() {
  if (Settings::get<bool>("debug-view")) {
    time_to_update_ = 1.0f;

    wnd_ = Builder<Window>(sum(pct(100), px(-200)), sum(pct(100), px(-50)), px(190), px(40))
      << (Builder<Label>(px(0), px(0), px(190), px(20)) << Label::text_align(Label::Alignment::kRight) << Widget::id(FPS_ID))
      << (Builder<Label>(px(0), px(20), px(190), px(20)) << Label::text_align(Label::Alignment::kRight) << Widget::id(PARTICLES_ID));
    Framework::get_instance()->get_gui()->attach_widget(wnd_);
  }
}

void DebugView::destroy() {
  if (wnd_ != nullptr) {
    Framework::get_instance()->get_gui()->detach_widget(wnd_);
  }
}

void DebugView::update(float dt) {
  if (wnd_ == nullptr) {
    return;
  }

  time_to_update_ -= dt;
  if (time_to_update_ <= 0.0f) {
    fw::Framework *frmwrk = fw::Framework::get_instance();

    Label *fps = wnd_->find<Label>(FPS_ID);
    fps->set_text(absl::StrCat(frmwrk->get_timer()->get_fps(), " fps"));

    Label *particles = wnd_->find<Label>(PARTICLES_ID);
    particles->set_text(
      absl::StrCat(frmwrk->get_particle_mgr()->get_num_active_particles(), " particles"));

    time_to_update_ = 1.0f;
  }
}

}



