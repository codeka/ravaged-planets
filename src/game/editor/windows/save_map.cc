#include <framework/bitmap.h>
#include <framework/framework.h>
#include <framework/graphics.h>
#include <framework/gui/gui.h>
#include <framework/gui/builder.h>
#include <framework/gui/label.h>
#include <framework/gui/textedit.h>
#include <framework/gui/window.h>
#include <framework/gui/button.h>
#include <framework/misc.h>
#include <framework/texture.h>

#include <game/world/world.h>
#include <game/editor/editor_screen.h>
#include <game/editor/editor_world.h>
#include <game/editor/world_writer.h>
#include <game/editor/windows/save_map.h>

using namespace fw::gui;
using namespace std::placeholders;

namespace ed {

SaveMapWindow *save_map = nullptr;

enum ids {
  NAME_ID,
  AUTHOR_ID,
  DESCRIPTION_ID,
  SCREENSHOT_ID
};

SaveMapWindow::SaveMapWindow() : wnd_(nullptr) {
}

SaveMapWindow::~SaveMapWindow() {
}

void SaveMapWindow::initialize() {
  wnd_ = Builder<Window>(sum(pct(50), px(-250)), sum(pct(50), px(-150)), px(500), px(232))
      << Window::background("frame") << Widget::visible(false)
      << (Builder<Label>(px(10), px(10), px(80), px(20)) << Label::text("Name:"))
      << (Builder<TextEdit>(px(90), px(10), px(160), px(20)) << Widget::id(NAME_ID))
      << (Builder<Label>(px(10), px(40), px(80), px(20)) << Label::text("Author:"))
      << (Builder<TextEdit>(px(90), px(40), px(160), px(20)) << Widget::id(AUTHOR_ID))
      << (Builder<Label>(px(10), px(70), px(80), px(20)) << Label::text("Description:"))
      << (Builder<TextEdit>(px(90), px(70), px(160), px(20)) << Widget::id(DESCRIPTION_ID))
      << (Builder<Label>(px(260), px(10), px(230), px(172)) << Widget::id(SCREENSHOT_ID))
      << (Builder<Button>(px(90), px(152), px(160), px(30)) << Button::text("Update screenshot")
          << Widget::click(std::bind(&SaveMapWindow::screenshot_clicked, this, _1)))
      << (Builder<Button>(sum(pct(100), px(-180)), sum(pct(100), px(-38)), px(80), px(30)) << Button::text("Save")
          << Widget::click(std::bind(&SaveMapWindow::save_clicked, this, _1)))
      << (Builder<Button>(sum(pct(100), px(-90)), sum(pct(100), px(-38)), px(80), px(30)) << Button::text("Cancel")
          << Widget::click(std::bind(&SaveMapWindow::cancel_clicked, this, _1)));
  fw::Framework::get_instance()->get_gui()->attach_widget(wnd_);
}

// when we go to show, we have to update our controls with what we currently know about the map we're editing.
void SaveMapWindow::show() {
  wnd_->set_visible(true);

  auto world = dynamic_cast<EditorWorld *>(game::World::get_instance());

  wnd_->find<TextEdit>(NAME_ID)->set_text(world->get_name());
  wnd_->find<TextEdit>(DESCRIPTION_ID)->set_text(world->get_description());
  if (world->get_author() == "") {
    wnd_->find<TextEdit>(AUTHOR_ID)->set_text(fw::get_user_name());
  } else {
    wnd_->find<TextEdit>(AUTHOR_ID)->set_text(world->get_author());
  }
  update_screenshot();
}

// updates the screenshot that we're displaying whenever it changes.
void SaveMapWindow::update_screenshot() {
  auto world = dynamic_cast<EditorWorld *>(game::World::get_instance());
  if (world->get_screenshot().get_width() == 0)
    return;

  fw::Bitmap const &bmp = world->get_screenshot();
  wnd_->find<Label>(SCREENSHOT_ID)->set_background(bmp);
}

bool SaveMapWindow::screenshot_clicked(Widget *w) {
  auto world = dynamic_cast<EditorWorld *>(game::World::get_instance());
  fw::Framework::get_instance()->take_screenshot(
      1024, 768, std::bind(&SaveMapWindow::screenshot_complete, this, _1), false);
  return true;
}

void SaveMapWindow::screenshot_complete(fw::Bitmap const &bitmap) {
  fw::Bitmap resized = bitmap;
  resized.resize(640, 480);

  auto world = dynamic_cast<EditorWorld *>(game::World::get_instance());
  world->set_screenshot(resized);
  fw::Framework::get_instance()->get_graphics()->run_on_render_thread([this]() {
    update_screenshot();
  });
}

bool SaveMapWindow::save_clicked(Widget *w) {
  auto world = dynamic_cast<EditorWorld *>(game::World::get_instance());
  world->set_name(wnd_->find<TextEdit>(NAME_ID)->get_text());
  world->set_author(wnd_->find<TextEdit>(AUTHOR_ID)->get_text());
  world->set_description(wnd_->find<TextEdit>(DESCRIPTION_ID)->get_text());

  WorldWriter writer(world);
  auto status = writer.write(world->get_name());
  if (!status.ok()) {
    // TODO: show error to user?
    LOG(ERR) << "error saving map: " << status << std::endl;
    return true;
  }

  wnd_->set_visible(false);
  return true;
}

bool SaveMapWindow::cancel_clicked(Widget *w) {
  wnd_->set_visible(false);
  return true;
}

}
