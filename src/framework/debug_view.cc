
#include <absl/strings/str_cat.h>

#include <framework/debug_view.h>
#include <framework/framework.h>
#include <framework/gui/builder.h>
#include <framework/gui/gui.h>
#include <framework/gui/label.h>
#include <framework/gui/widget.h>
#include <framework/gui/window.h>
#include <framework/particle_manager.h>
#include <framework/service_locator.h>
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

    wnd_ = Builder<Window>()
			<< Widget::width(LayoutParams::Mode::kFixed, 190)
      << Widget::height(LayoutParams::Mode::kFixed, 40)
      << (Builder<Label>()
				  << Widget::width(LayoutParams::Mode::kMatchParent, 0)
				  << Widget::height(LayoutParams::Mode::kFixed, 20)
          << Label::text_align(Label::Alignment::kRight)
          << Widget::id(FPS_ID))
      << (Builder<Label>()
          << Widget::width(LayoutParams::Mode::kMatchParent, 0)
          << Widget::height(LayoutParams::Mode::kFixed, 20)
          << Label::text_align(Label::Alignment::kRight)
          << Widget::id(PARTICLES_ID));
    fw::Get<Gui>().AttachWindow(wnd_);
  }
}

void DebugView::destroy() {
  if (wnd_) {
    fw::Get<Gui>().DetachWindow(wnd_);
  }
}

void DebugView::update(float dt) {
  if (!wnd_) {
    return;
  }

  time_to_update_ -= dt;
  if (time_to_update_ <= 0.0f) {
    fw::Framework *frmwrk = fw::Framework::get_instance();

    auto fps = wnd_->Find<Label>(FPS_ID);
    fps->set_text(absl::StrCat(frmwrk->get_timer()->get_fps(), " fps"));

    auto particles = wnd_->Find<Label>(PARTICLES_ID);
    particles->set_text(
      absl::StrCat(frmwrk->get_particle_mgr()->get_num_active_particles(), " particles"));

    time_to_update_ = 1.0f;
  }
}

}



