
#include <framework/framework.h>
#include <framework/gui/gui.h>
#include <framework/gui/builder.h>
#include <framework/gui/button.h>
#include <framework/gui/drawable.h>
#include <framework/gui/listbox.h>
#include <framework/timer.h>

using namespace std::placeholders;

namespace fw::gui {

enum ids {
  THUMB = 56756,
  UP_BUTTON = 56757,
  DOWN_BUTTON = 56758
};

/** If you click again within this amount of time, then we count your click as "activate" (i.e. double-click) */
static const float ACTIVATE_TIME_MS = 600.0f;

//-----------------------------------------------------------------------------

class ListboxItemSelectedProperty : public Property {
private:
  std::function<void(int index)> on_selected_;
public:
  ListboxItemSelectedProperty(std::function<void(int index)> on_selected)
      : on_selected_(on_selected) {
  }

  void apply(Widget *widget) {
    dynamic_cast<Listbox *>(widget)->sig_item_selected.connect(on_selected_);
  }
};

class ListboxItemActivatedProperty : public Property {
private:
  std::function<void(int index)> on_activated_;
public:
  ListboxItemActivatedProperty(std::function<void(int index)> on_activated)
      : on_activated_(on_activated) {
  }

  void apply(Widget *widget) {
    dynamic_cast<Listbox *>(widget)->sig_item_activated.connect(on_activated_);
  }
};

//-----------------------------------------------------------------------------

// A special widget that we add children to. This item handles the selection colours and positioning of the item.
class ListboxItem : public Widget {
private:
  Listbox *listbox_;
  int index_;
  std::shared_ptr<StateDrawable> background_;
  float last_down_time_;
public:
  ListboxItem(Gui *gui);
  virtual ~ListboxItem();

  virtual bool on_mouse_down(float x, float y);

  void setup(Listbox *listbox, int index);
  int get_index() const;
  void set_selected(bool selected);
  Widget *get_widget();

  void render();
};

ListboxItem::ListboxItem(Gui *gui) : Widget(gui), listbox_(nullptr), index_(-1), last_down_time_(0) {
  background_ = std::shared_ptr<StateDrawable>(new StateDrawable());
  background_->add_drawable(StateDrawable::kNormal, gui_->get_drawable_manager()->get_drawable("listbox_item_normal"));
  background_->add_drawable(StateDrawable::kSelected, gui_->get_drawable_manager()->get_drawable("listbox_item_selected"));
}

ListboxItem::~ListboxItem() {
}

// When you click a listbox item, we want to make sure it's the selected one.
bool ListboxItem::on_mouse_down(float x, float y) {
  listbox_->select_item(index_);
  float now = fw::framework::get_instance()->get_timer()->get_total_time();
  if (now - last_down_time_ < (ACTIVATE_TIME_MS / 1000.0f)) {
    listbox_->activate_item(index_);
  }
  last_down_time_ = now;
  return true;
}

void ListboxItem::setup(Listbox *listbox, int index) {
  listbox_ = listbox;
  index_ = index;
}

int ListboxItem::get_index() const {
  return index_;
}

Widget *ListboxItem::get_widget() {
  return children_[0];
}

void ListboxItem::set_selected(bool selected) {
  if (selected) {
    background_->set_current_state(StateDrawable::kSelected);
  } else {
    background_->set_current_state(StateDrawable::kNormal);
  }
}

void ListboxItem::render() {
  background_->render(get_left(), get_top(), get_width(), get_height());
  Widget::render();
}

//-----------------------------------------------------------------------------

Listbox::Listbox(Gui *gui) : Widget(gui), selected_item_(nullptr), scrollbar_visible_(false) {
  background_ = gui->get_drawable_manager()->get_drawable("listbox_background");

  std::shared_ptr<StateDrawable> bkgnd = std::shared_ptr<StateDrawable>(new StateDrawable());
  bkgnd->add_drawable(StateDrawable::kNormal, gui_->get_drawable_manager()->get_drawable("listbox_up_normal"));
  bkgnd->add_drawable(StateDrawable::kHover, gui_->get_drawable_manager()->get_drawable("listbox_up_hover"));
  attach_child(Builder<Button>(sum(pct(100), px(-19)), px(0), px(19), px(19))
      << Button::background(bkgnd)
      << Widget::id(UP_BUTTON)
      << Widget::visible(false)
      << Button::click(std::bind(&Listbox::on_up_button_click, this, _1)));
  bkgnd = std::shared_ptr<StateDrawable>(new StateDrawable());
  bkgnd->add_drawable(StateDrawable::kNormal, gui_->get_drawable_manager()->get_drawable("listbox_down_normal"));
  bkgnd->add_drawable(StateDrawable::kHover, gui_->get_drawable_manager()->get_drawable("listbox_down_hover"));
  attach_child(Builder<Button>(sum(pct(100), px(-19)), sum(pct(100), px(-19)), px(19), px(19))
      << Button::background(bkgnd)
      << Widget::id(DOWN_BUTTON)
      << Widget::visible(false)
      << Button::click(std::bind(&Listbox::on_down_button_click, this, _1)));
  bkgnd = std::shared_ptr<StateDrawable>(new StateDrawable());
  bkgnd->add_drawable(StateDrawable::kNormal, gui_->get_drawable_manager()->get_drawable("listbox_thumb_normal"));
  bkgnd->add_drawable(StateDrawable::kHover, gui_->get_drawable_manager()->get_drawable("listbox_thumb_hover"));
  attach_child(Builder<Button>(sum(pct(100), px(-19)), px(19), px(19), sum(pct(100), px(-38)))
      << Button::background(bkgnd)
      << Widget::id(THUMB)
      << Widget::visible(false));

  item_container_ = Builder<Widget>(px(0), px(0), pct(100), px(0));
  attach_child(item_container_);
}

Listbox::~Listbox() {
}

Property * Listbox::item_selected(std::function<void(int index)> on_selected) {
  return new ListboxItemSelectedProperty(on_selected);
}

Property * Listbox::item_activated(std::function<void(int index)> on_activated) {
  return new ListboxItemActivatedProperty(on_activated);
}

void Listbox::add_item(Widget *w) {
  int top = item_container_->get_height();
  ListboxItem *item = Builder<ListboxItem>(px(0), px(top), pct(100), px(w->get_height()));
  item->attach_child(w);
  item->setup(this, items_.size());
  item_container_->attach_child(item);
  item_container_->set_height(px(item_container_->get_height() + w->get_height()));
  items_.push_back(item);
  update_thumb_button(true);
}

void Listbox::clear() {
  item_container_->clear_children();
  item_container_->set_height(px(0));
  item_container_->set_top(px(0));
  items_.clear();
  update_thumb_button(true);
}

void Listbox::update_thumb_button(bool adjust_height) {
  float widget_height = get_height();
  float content_height = item_container_->get_height();
  float thumb_max_height = widget_height - 38.0f; // the up/down buttons
  float thumb_height;
  Button *thumb = find<Button>(THUMB);

  if (adjust_height) {
    if (content_height <= widget_height) {
      thumb_height = thumb_max_height;
      scrollbar_visible_ = false;
    } else {
      // This will be 0.5 when content_height is twice widget height,
      float ratio = widget_height / content_height;
      thumb_height = thumb_max_height * ratio;
      if (thumb_height < 30) {
        thumb_height = 30;
      }
      if (thumb_height >= thumb_max_height) {
        thumb_height = thumb_max_height;
        scrollbar_visible_ = false;
      } else {
        scrollbar_visible_ = true;
      }
    }
    thumb->set_height(px(thumb_height));
  } else {
    thumb_height = thumb->get_height();
  }

  float offset = get_top() - item_container_->get_top();
  float max_offset = item_container_->get_height() - get_height();
  float offset_ratio = offset / max_offset;

  float max_thumb_offset = thumb_max_height - thumb_height;
  float thumb_offset = max_thumb_offset * offset_ratio;
  thumb->set_top(px(19 + thumb_offset));

  Button *up_button = find<Button>(UP_BUTTON);
  Button *down_button = find<Button>(DOWN_BUTTON);
  if (scrollbar_visible_) {
    item_container_->set_width(sum(pct(100), px(-20)));
    thumb->set_visible(true);
    up_button->set_visible(true);
    down_button->set_visible(true);
  } else {
    item_container_->set_width(pct(100));
    thumb->set_visible(false);
    up_button->set_visible(false);
    down_button->set_visible(false);
  }
}

bool Listbox::on_down_button_click(Widget *w) {
  float current_top = item_container_->get_top() - get_top();
  current_top -= get_height() / 4.0f;
  float max_offset = item_container_->get_height() - get_height();
  if (current_top < -max_offset) {
    current_top = -max_offset;
  }
  item_container_->set_top(px(current_top));
  update_thumb_button(false);
  return true;
}

bool Listbox::on_up_button_click(Widget *w) {
  float current_top = item_container_->get_top() - get_top();
  current_top += get_height() / 4.0f;
  if (current_top > 0) {
    current_top = 0.0f;
  }
  item_container_->set_top(px(current_top));
  update_thumb_button(false);
  return true;
}

void Listbox::select_item(int index) {
  if (selected_item_ != nullptr) {
    selected_item_->set_selected(false);
  }
  selected_item_ = items_[index];
  selected_item_->set_selected(true);
  sig_item_selected(index);
}

void Listbox::activate_item(int index) {
  sig_item_activated(index);
}

int Listbox::get_selected_index() {
  if (selected_item_ == nullptr) {
    return -1;
  }
  return selected_item_->get_index();
}

Widget * Listbox::get_item(int index) {
  if (index < 0 || index >= items_.size()) {
    return nullptr;
  }
  return items_[index]->get_widget();
}

Widget * Listbox::get_selected_item() {
  return selected_item_ == nullptr ? nullptr : selected_item_->get_widget();
}

void Listbox::render() {
  background_->render(get_left(), get_top(), get_width(), get_height());

  Widget::render();
}

}
