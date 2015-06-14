
#include <framework/gui/gui.h>
#include <framework/gui/builder.h>
#include <framework/gui/button.h>
#include <framework/gui/drawable.h>
#include <framework/gui/listbox.h>

namespace fw { namespace gui {

/** A special widget that we add children to. This item handles the selection colours and positioning of the item. */
class listbox_item : public widget {
private:
  listbox *_listbox;
  int _index;
  std::shared_ptr<state_drawable> _background;
public:
  listbox_item(gui *gui);
  virtual ~listbox_item();

  virtual bool on_mouse_down(float x, float y);

  void setup(listbox *listbox, int index);
  int get_index() const;
  void set_selected(bool selected);

  void render();
};

listbox_item::listbox_item(gui *gui) : widget(gui), _listbox(nullptr), _index(-1) {
  _background = std::shared_ptr<state_drawable>(new state_drawable());
  _background->add_drawable(state_drawable::normal, _gui->get_drawable_manager()->get_drawable("listbox_item_normal"));
  _background->add_drawable(state_drawable::selected, _gui->get_drawable_manager()->get_drawable("listbox_item_selected"));
}

listbox_item::~listbox_item() {
}

/** When you click a listbox item, we want to make sure it's the selected one. */
bool listbox_item::on_mouse_down(float x, float y) {
  _listbox->select_item(_index);
  return true;
}

void listbox_item::setup(listbox *listbox, int index) {
  _listbox = listbox;
  _index = index;
}

int listbox_item::get_index() const {
  return _index;
}

void listbox_item::set_selected(bool selected) {
  if (selected) {
    _background->set_current_state(state_drawable::selected);
  } else {
    _background->set_current_state(state_drawable::normal);
  }
}

void listbox_item::render() {
  _background->render(get_left(), get_top(), get_width(), get_height());
  widget::render();
}

//-----------------------------------------------------------------------------

listbox::listbox(gui *gui) : widget(gui), _selected_item(nullptr) {
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

  _item_container = builder<widget>(px(0), px(0), sum(pct(100), px(-20)), px(0));
  attach_child(_item_container);
}

listbox::~listbox() {
}

void listbox::add_item(widget *w) {
  int top = _item_container->get_height();
  listbox_item *item = builder<listbox_item>(px(0), px(top), pct(100), px(w->get_height()));
  item->attach_child(w);
  item->setup(this, _items.size());
  _item_container->attach_child(item);
  _item_container->set_height(px(_item_container->get_height() + w->get_height()));
  _items.push_back(item);
}

void listbox::select_item(int index) {
  if (_selected_item != nullptr) {
    _selected_item->set_selected(false);
  }
  _selected_item = _items[index];
  _selected_item->set_selected(true);
  sig_item_selected(index);
}

void listbox::render() {
  _background->render(get_left(), get_top(), get_width(), get_height());

  widget::render();
}

} }
