
#include <framework/gui/gui.h>
#include <framework/gui/builder.h>
#include <framework/gui/button.h>
#include <framework/gui/drawable.h>
#include <framework/gui/listbox.h>

namespace fw { namespace gui {

/** A special widget that we add children to. This item handles the selection colours and positioning of the item. */
class listbox_item : public widget {
private:
  std::shared_ptr<state_drawable> _background;
public:
  listbox_item(gui *gui);
  virtual ~listbox_item();

  void render();
};

listbox_item::listbox_item(gui *gui) : widget(gui) {
  _background = std::shared_ptr<state_drawable>(new state_drawable());
  _background->add_drawable(state_drawable::normal, _gui->get_drawable_manager()->get_drawable("listbox_item_normal"));
  _background->add_drawable(state_drawable::selected, _gui->get_drawable_manager()->get_drawable("listbox_item_selected"));
}

listbox_item::~listbox_item() {
}

void listbox_item::render() {
  _background->render(get_left(), get_top(), get_width(), get_height());
  widget::render();
}

//-----------------------------------------------------------------------------

listbox::listbox(gui *gui) : widget(gui) {
  _background = gui->get_drawable_manager()->get_drawable("listbox_background");

  std::shared_ptr<state_drawable> bkgnd = std::shared_ptr<state_drawable>(new state_drawable());
  bkgnd->add_drawable(state_drawable::normal, _gui->get_drawable_manager()->get_drawable("listbox_up_normal"));
  bkgnd->add_drawable(state_drawable::hover, _gui->get_drawable_manager()->get_drawable("listbox_up_hover"));
  attach_child(builder<button>(sum(pct(100), px(-19)), px(0), px(19), px(19))
      << button::background(bkgnd));
  bkgnd = std::shared_ptr<state_drawable>(new state_drawable());
  bkgnd->add_drawable(state_drawable::normal, _gui->get_drawable_manager()->get_drawable("listbox_down_normal"));
  bkgnd->add_drawable(state_drawable::hover, _gui->get_drawable_manager()->get_drawable("listbox_down_hover"));
  attach_child(builder<button>(sum(pct(100), px(-19)), sum(pct(100), px(-19)), px(19), px(19))
      << button::background(bkgnd));
  bkgnd = std::shared_ptr<state_drawable>(new state_drawable());
  bkgnd->add_drawable(state_drawable::normal, _gui->get_drawable_manager()->get_drawable("listbox_thumb_normal"));
  bkgnd->add_drawable(state_drawable::hover, _gui->get_drawable_manager()->get_drawable("listbox_thumb_hover"));
  attach_child(builder<button>(sum(pct(100), px(-19)), px(19), px(19), sum(pct(100), px(-38)))
      << button::background(bkgnd));
}

listbox::~listbox() {
}

void listbox::add_item(widget *w) {
  int top = 0;
  if (_items.size() > 0) {
    listbox_item *last_item = _items[_items.size() - 1];
    top = last_item->get_top() + last_item->get_height() - _items[0]->get_top();
  }
  listbox_item *item = builder<listbox_item>(px(0), px(top), sum(pct(100), px(-20)), px(w->get_height()));
  item->attach_child(w);
  attach_child(item);
  _items.push_back(item);
}


void listbox::render() {
  _background->render(get_left(), get_top(), get_width(), get_height());

  widget::render();
}

} }
