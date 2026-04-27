#include <framework/gui/listbox.h>

#include <memory>

#include <framework/framework.h>
#include <framework/gui/gui.h>
#include <framework/gui/builder.h>
#include <framework/gui/button.h>
#include <framework/gui/drawable.h>
#include <framework/timer.h>

using namespace std::placeholders;

namespace fw::gui {

enum ids {
  THUMB = 56756,
  UP_BUTTON = 56757,
  DOWN_BUTTON = 56758
};

/**
 * If you click again within this amount of time, then we count your click as "activate" (i.e.
 * double-click)
 */
static const float ACTIVATE_TIME_MS = 600.0f;

//-----------------------------------------------------------------------------

class ListboxItemSelectedProperty : public Property {
private:
  std::function<void(int index)> on_selected_;
public:
  ListboxItemSelectedProperty(std::function<void(int index)> on_selected)
      : on_selected_(on_selected) {
  }

  void apply(Widget &widget) override {
    dynamic_cast<Listbox &>(widget).sig_item_selected.Connect(on_selected_);
  }
};

class ListboxItemActivatedProperty : public Property {
private:
  std::function<void(int index)> on_activated_;
public:
  ListboxItemActivatedProperty(std::function<void(int index)> on_activated)
      : on_activated_(on_activated) {
  }

  void apply(Widget &widget) override {
    dynamic_cast<Listbox &>(widget).sig_item_activated.Connect(on_activated_);
  }
};

//-----------------------------------------------------------------------------

// A special widget that we add children to. This item handles the selection colors and positioning
// of the item.
class ListboxItem : public Widget {
public:
  ListboxItem();
  virtual ~ListboxItem();

  virtual bool on_mouse_down(float x, float y);

  void Setup(std::shared_ptr<Listbox> listbox, int index);
  int GetIndex() const;
  void SetSelected(bool selected);
  std::shared_ptr<Widget> GetWidget();

  void render() override;
private:
  std::weak_ptr<Listbox> listbox_;
  int index_;
  std::shared_ptr<StateDrawable> background_;
  float last_down_time_;
};

ListboxItem::ListboxItem()
    : Widget(), index_(-1), last_down_time_(0) {
  background_ = std::shared_ptr<StateDrawable>(new StateDrawable());
  background_->add_drawable(
      StateDrawable::kNormal,
    fw::Get<Gui>().get_drawable_manager().get_drawable("listbox_item_normal"));
  background_->add_drawable(
      StateDrawable::kSelected,
    fw::Get<Gui>().get_drawable_manager().get_drawable("listbox_item_selected"));
}

ListboxItem::~ListboxItem() {
}

// When you click a listbox item, we want to make sure it's the selected one.
bool ListboxItem::on_mouse_down(float x, float y) {
  auto listbox = listbox_.lock();
  listbox->SelectItem(index_);
  float now = fw::Framework::get_instance()->get_timer()->get_total_time();
  if (now - last_down_time_ < (ACTIVATE_TIME_MS / 1000.0f)) {
    listbox->ActivateItem(index_);
  }
  last_down_time_ = now;
  return true;
}

void ListboxItem::Setup(std::shared_ptr<Listbox> listbox, int index) {
  listbox_ = listbox;
  index_ = index;
}

int ListboxItem::GetIndex() const {
  return index_;
}

std::shared_ptr<Widget> ListboxItem::GetWidget() {
  return children_[0];
}

void ListboxItem::SetSelected(bool selected) {
  if (selected) {
    background_->set_current_state(StateDrawable::kSelected);
  } else {
    background_->set_current_state(StateDrawable::kNormal);
  }
}

void ListboxItem::render() {
  auto rect = GetScreenRect();
  background_->render(rect.left, rect.top, rect.width, rect.height);
  Widget::render();
}

//-----------------------------------------------------------------------------

Listbox::Listbox() : Widget(), scrollbar_visible_(false) {
  background_ = fw::Get<Gui>().get_drawable_manager().get_drawable("listbox_background");
}

Listbox::~Listbox() {
}

void Listbox::OnAttachedToParent(Widget &parent) {
  Widget::OnAttachedToParent(parent);

  std::shared_ptr<StateDrawable> bkgnd = std::shared_ptr<StateDrawable>(new StateDrawable());
  bkgnd->add_drawable(
      StateDrawable::kNormal,
      fw::Get<Gui>().get_drawable_manager().get_drawable("listbox_up_normal"));
  bkgnd->add_drawable(
      StateDrawable::kHover,
      fw::Get<Gui>().get_drawable_manager().get_drawable("listbox_up_hover"));
  AttachChild(Builder<Button>()
		  << Widget::width(LayoutParams::kFixed, 19.0f)
      << Widget::height(LayoutParams::kFixed, 19.0f)
		  << Widget::gravity(LayoutParams::Gravity::kRight | LayoutParams::Gravity::kTop)
      << Button::background(bkgnd)
      << Widget::id(UP_BUTTON)
      //<< Widget::visible(false)
      << Button::click(std::bind(&Listbox::OnUpButtonClick, this, _1)));
  bkgnd = std::shared_ptr<StateDrawable>(new StateDrawable());
  bkgnd->add_drawable(
      StateDrawable::kNormal,
      fw::Get<Gui>().get_drawable_manager().get_drawable("listbox_down_normal"));
  bkgnd->add_drawable(
      StateDrawable::kHover,
      fw::Get<Gui>().get_drawable_manager().get_drawable("listbox_down_hover"));
  AttachChild(Builder<Button>()
      << Widget::width(LayoutParams::kFixed, 19.0f)
      << Widget::height(LayoutParams::kFixed, 19.0f)
      << Widget::gravity(LayoutParams::Gravity::kRight | LayoutParams::Gravity::kBottom)
      << Button::background(bkgnd)
      << Widget::id(DOWN_BUTTON)
      //<< Widget::visible(false)
      << Button::click(std::bind(&Listbox::OnDownButtonClick, this, _1)));
  bkgnd = std::shared_ptr<StateDrawable>(new StateDrawable());
  bkgnd->add_drawable(
      StateDrawable::kNormal,
      fw::Get<Gui>().get_drawable_manager().get_drawable("listbox_thumb_normal"));
  bkgnd->add_drawable(
      StateDrawable::kHover,
      fw::Get<Gui>().get_drawable_manager().get_drawable("listbox_thumb_hover"));
  AttachChild(Builder<Button>()
      << Widget::width(LayoutParams::kFixed, 19.0f)
      << Widget::height(LayoutParams::kFixed, 19.0f)
      << Widget::gravity(LayoutParams::Gravity::kRight | LayoutParams::Gravity::kTop)
		  << Widget::margin(19.0f, 0.0f, 0.0f, 0.0f)
      << Button::background(bkgnd)
      << Widget::id(THUMB)
      /*<< Widget::visible(false)*/);

  item_container_ = Builder<LinearLayout>()
      << Widget::width(LayoutParams::kMatchParent, 0.f)
      << Widget::height(LayoutParams::kMatchParent, 0.f)
      << Widget::margin(0.f, 19.f, 0.f, 0.f)
	    << LinearLayout::orientation(LinearLayout::Orientation::kVertical);
  AttachChild(item_container_);
}

std::unique_ptr<Property> Listbox::item_selected(std::function<void(int index)> on_selected) {
  return std::make_unique<ListboxItemSelectedProperty>(on_selected);
}

std::unique_ptr<Property> Listbox::item_activated(std::function<void(int index)> on_activated) {
  return std::make_unique<ListboxItemActivatedProperty>(on_activated);
}

void Listbox::AddItem(std::shared_ptr<Widget> w) {
  auto item = std::shared_ptr<ListboxItem>(
      Builder<ListboxItem>()
          << Widget::width(LayoutParams::kMatchParent, 0.f)
          << Widget::height(LayoutParams::kWrapContent, 0.f));
  item->AttachChild(w);
  item->Setup(std::dynamic_pointer_cast<Listbox>(this->shared_from_this()), items_.size());
  item_container_->AttachChild(item);
  items_.push_back(item);
}

void Listbox::Clear() {
  item_container_->ClearChildren();
  items_.clear();
  UpdateThumbButton(true);
}

MeasuredSize Listbox::OnMeasure(MeasureSpec width_spec, MeasureSpec height_spec) {
	UpdateThumbButton(true);

  return Widget::OnMeasure(width_spec, height_spec);
}

void Listbox::UpdateThumbButton(bool adjust_height) {
  const float widget_height = get_height();
  const float content_height = item_container_->CalculateItemsTotalHeight();
  const float thumb_max_height = widget_height - 38.0f; // the up/down buttons
  float thumb_height;
  auto thumb = Find<Button>(THUMB);

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
    thumb->get_layout_params()->height = std::floor(thumb_height);
  } else {
    thumb_height = thumb->get_height();
  }

  auto up_button = Find<Button>(UP_BUTTON);
  auto down_button = Find<Button>(DOWN_BUTTON);
  if (scrollbar_visible_) {
    item_container_->get_layout_params()->right_margin = 20.f;
    thumb->set_visible(true);
    up_button->set_visible(true);
    down_button->set_visible(true);

    float top_margin = item_container_->get_layout_params()->top_margin;
    float max_top_margin = content_height - get_height();
    float offset_ratio = -top_margin / max_top_margin;

    float max_thumb_offset = thumb_max_height - thumb_height;
    float thumb_offset = max_thumb_offset * offset_ratio;
    thumb->get_layout_params()->top_margin = std::floor(19.f + thumb_offset);
  } else {
    item_container_->get_layout_params()->right_margin = 0.f;
    thumb->set_visible(false);
    up_button->set_visible(false);
    down_button->set_visible(false);
  }
}

bool Listbox::OnDownButtonClick(Widget &w) {
  float top_margin = item_container_->get_layout_params()->top_margin;
  top_margin -= get_height() / 4.0f;

  float item_container_height = item_container_->CalculateItemsTotalHeight();
  float max_top_margin = item_container_height - get_height();
  if (top_margin < -max_top_margin) {
    top_margin = -max_top_margin;
  }
  item_container_->get_layout_params()->top_margin = top_margin;
  UpdateThumbButton(false);
  RequestLayout();
  return true;
}

bool Listbox::OnUpButtonClick(Widget &w) {
  float top_margin = item_container_->get_layout_params()->top_margin;
  top_margin += get_height() / 4.0f;
  if (top_margin > 0) {
    top_margin = 0.0f;
  }
  item_container_->get_layout_params()->top_margin = top_margin;
  UpdateThumbButton(false);
  RequestLayout();
  return true;
}

void Listbox::SelectItem(int index) {
  auto selected_item = selected_item_.lock();
  if (selected_item) {
    selected_item->SetSelected(false);
  }
  selected_item = items_[index];
  selected_item->SetSelected(true);
  selected_item_ = selected_item;
  sig_item_selected.Emit(index);
}

void Listbox::ActivateItem(int index) {
  sig_item_activated.Emit(index);
}

int Listbox::GetSelectedIndex() {
  auto selected_item = selected_item_.lock();
  if (!selected_item) {
    return -1;
  }
  return selected_item->GetIndex();
}

std::shared_ptr<Widget> Listbox::GetItem(int index) {
  if (index < 0 || index >= items_.size()) {
    return nullptr;
  }
  return items_[index]->GetWidget();
}

std::shared_ptr<Widget> Listbox::GetSelectedItem() {
  auto selected_item = selected_item_.lock();
  return selected_item ? selected_item->GetWidget() : nullptr;
}

void Listbox::render() {
	auto rect = GetScreenRect();
  background_->render(rect.left, rect.top, rect.width, rect.height);

  Widget::render();
}

}
