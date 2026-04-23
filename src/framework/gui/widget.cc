#include <framework/gui/widget.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <stack>
#include <vector>

#include <framework/graphics.h>
#include <framework/gui/gui.h>
#include <framework/misc.h>

namespace fw::gui {

class WidgetWidthHeightProperty : public Property {
private:
  bool is_width_;
  LayoutParams::Mode mode_;
  float value_;

public:
  WidgetWidthHeightProperty(bool is_width, LayoutParams::Mode mode, float value)
    : is_width_(is_width), mode_(mode), value_(value) {}

  void apply(Widget& widget) override {
    if (is_width_) {
      widget.get_layout_params()->width_mode = mode_;
      widget.get_layout_params()->width = value_;
    } else {
      widget.get_layout_params()->height_mode = mode_;
      widget.get_layout_params()->height = value_;
    }
  }
};

class WidgetMarginProperty : public Property {
private:
  float top_;
  float right_;
  float bottom_;
  float left_;

public:
  WidgetMarginProperty(float top, float right, float bottom, float left)
    : top_(top), right_(right), bottom_(bottom), left_(left) {}
  void apply(Widget& widget) override {
    widget.get_layout_params()->top_margin = top_;
    widget.get_layout_params()->right_margin = right_;
    widget.get_layout_params()->bottom_margin = bottom_;
    widget.get_layout_params()->left_margin = left_;
  }
};

class WidgetClickProperty : public Property {
private:
  std::function<bool(Widget&)> on_click_;
public:
  WidgetClickProperty(std::function<bool(Widget&)> on_click)
    : on_click_(on_click) {}

  void apply(Widget& widget) override {
    widget.on_click_ = on_click_;
  }
};

class WidgetIdProperty : public Property {
private:
  int id_;
public:
  WidgetIdProperty(int id)
    : id_(id) {}

  void apply(Widget& widget) override {
    widget.id_ = id_;
  }
};

class WidgetNameProperty : public Property {
private:
  std::string name_;
public:
  WidgetNameProperty(std::string_view name)
    : name_(name) {}

  void apply(Widget& widget) override {
    widget.name_ = name_;
  }
};

class WidgetVisibleProperty : public Property {
private:
  bool visible_;
public:
  WidgetVisibleProperty(bool visible)
    : visible_(visible) {}

  void apply(Widget& widget) override {
    widget.visible_ = visible_;
  }
};

class WidgetDataProperty : public Property {
private:
  std::any data_;
public:
  WidgetDataProperty(std::any const& data)
    : data_(data) {}

  void apply(Widget& widget) override {
    widget.data_ = data_;
  }
};

class WidgetEnabledProperty : public Property {
private:
  bool enabled_;
public:
  WidgetEnabledProperty(bool enabled)
    : enabled_(enabled) {}

  void apply(Widget& widget) override {
    widget.set_enabled(enabled_);
  }
};

class WidgetGravityProperty : public Property {
private:
  int gravity_;
public:
  WidgetGravityProperty(int gravity)
    : gravity_(gravity) {}

  void apply(Widget& widget) override {
		auto lp = widget.get_layout_params();
    if (lp) {
      lp->gravity = gravity_;
		}
  }
};

class WidgetPaddingProperty : public Property {
private:
  float top_; 
  float right_;
  float bottom_;
  float left_;

public:
  WidgetPaddingProperty(float top, float right, float bottom, float left)
    : top_(top), right_(right), bottom_(bottom), left_(left) {}

  void apply(Widget& widget) override {
    widget.padding_top_= top_;
    widget.padding_right_ = right_;
    widget.padding_bottom_ = bottom_;
    widget.padding_left_ = left_;
  }
};

//-----------------------------------------------------------------------------

Widget::Widget() :
  measured_size_(0, 0) {}

Widget::~Widget() {}

std::unique_ptr<Property> Widget::width(LayoutParams::Mode mode, float width) {
  return std::make_unique<WidgetWidthHeightProperty>(true, mode, width);
}

std::unique_ptr<Property> Widget::height(LayoutParams::Mode mode, float height) {
  return std::make_unique<WidgetWidthHeightProperty>(false, mode, height);
}

std::unique_ptr<Property> Widget::margin(float top, float right, float bottom, float left) {
  return std::make_unique<WidgetMarginProperty>(top, right, bottom, left);
}

std::unique_ptr<Property> Widget::click(std::function<bool(Widget&)> on_click) {
  return std::make_unique<WidgetClickProperty>(on_click);
}

std::unique_ptr<Property> Widget::visible(bool visible) {
  return std::make_unique<WidgetVisibleProperty>(visible);
}

std::unique_ptr<Property> Widget::id(int id) {
  return std::make_unique<WidgetIdProperty>(id);
}

std::unique_ptr<Property> Widget::name(std::string_view name) {
  return std::make_unique<WidgetNameProperty>(name);
}

std::unique_ptr<Property> Widget::data(std::any const& data) {
  return std::make_unique<WidgetDataProperty>(data);
}

std::unique_ptr<Property> Widget::enabled(bool enabled) {
  return std::make_unique<WidgetEnabledProperty>(enabled);
}

std::unique_ptr<Property> Widget::gravity(int gravity) {
  return std::make_unique<WidgetGravityProperty>(gravity);
}

std::unique_ptr<Property> Widget::padding(float top, float right, float bottom, float left) {
  return std::make_unique<WidgetPaddingProperty>(top, right, bottom, left);
}

void Widget::AttachChild(std::shared_ptr<Widget> child) {
  fw::Get<Gui>().EnsureThread(*this);

  std::shared_ptr<Widget> old_parent = child->parent_.lock();
  if (old_parent) {
    old_parent->DetachChild(child);
  }
  child->parent_ = this->shared_from_this();
  children_.push_back(child);
  child->OnAttachedToParent(*this);
  RequestLayout();
}

void Widget::DetachChild(std::shared_ptr<Widget> child) {
  fw::Get<Gui>().EnsureThread(*this);

  children_.erase(std::find(children_.begin(), children_.end(), child));
  child->parent_.reset();
  RequestLayout();
}

void Widget::ClearChildren() {
  fw::Get<Gui>().EnsureThread(*this);

  for (auto& child : children_) {
    child->parent_.reset();
  }
  children_.clear();
  RequestLayout();
}

void Widget::OnAttachedToParent(Widget& parent) {
  layout_params_ = parent.CreateLayoutParams();
  RequestLayout();
}

MeasuredSize Widget::ResolveSize(
    MeasureSpec width_spec,
    float measured_width,
    MeasureSpec height_spec,
    float measured_height) {
	float width = measured_width;
	float height = measured_height;

  switch (width_spec.mode) {
  case MeasureSpec::Mode::kExactly:
    width = width_spec.size;
    break;
  case MeasureSpec::Mode::kAtMost:
    if (width > width_spec.size) {
      width = width_spec.size;
    }
    break;
  }

  switch (height_spec.mode) {
  case MeasureSpec::Mode::kExactly:
    height = height_spec.size;
    break;
  case MeasureSpec::Mode::kAtMost:
    if (height > height_spec.size) {
      height = height_spec.size;
    }
    break;
  }

	return MeasuredSize(width, height);
}

MeasureSpec GetChildMeasureSpec(
    MeasureSpec parent_spec,
    float padding_and_margins,
    LayoutParams::Mode layout_mode,
    float layout_size) {

  MeasureSpec result_spec = MeasureSpec::Unspecified();
  if (parent_spec.mode == MeasureSpec::Mode::kExactly) {
    float available_size = std::max(0.f, parent_spec.size - padding_and_margins);
    if (layout_mode == LayoutParams::Mode::kFixed) {
      result_spec = MeasureSpec::Exactly(layout_size);
    } else if (layout_mode == LayoutParams::Mode::kMatchParent) {
      result_spec = MeasureSpec::Exactly(available_size);
    } else if (layout_mode == LayoutParams::Mode::kWrapContent) {
      result_spec = MeasureSpec::AtMost(available_size);
    }
  } else if (parent_spec.mode == MeasureSpec::Mode::kAtMost) {
    float available_size = std::max(0.f, parent_spec.size - padding_and_margins);
    if (layout_mode == LayoutParams::Mode::kFixed) {
      result_spec = MeasureSpec::Exactly(layout_size);
    } else if (layout_mode == LayoutParams::Mode::kMatchParent) {
      result_spec = MeasureSpec::AtMost(available_size);
    } else if (layout_mode == LayoutParams::Mode::kWrapContent) {
      result_spec = MeasureSpec::AtMost(available_size);
    }
  } else if (parent_spec.mode == MeasureSpec::Mode::kUnspecified) {
    if (layout_mode == LayoutParams::Mode::kFixed) {
      result_spec = MeasureSpec::Exactly(layout_size);
    } else {
      // Match parent doesn't make sense when the parent has not specified a size. Fall back to
      // wrap content.
      result_spec = MeasureSpec::Unspecified();
    }
  }
  return result_spec;
}

MeasuredSize Widget::MeasureChild(
    MeasureSpec parent_width_spec,
    float width_used,
    MeasureSpec parent_height_spec,
    float height_used) {

  MeasureSpec width_spec = GetChildMeasureSpec(
    parent_width_spec,
    layout_params_->left_margin + layout_params_->right_margin + width_used,
    layout_params_->width_mode,
    layout_params_->width);
  MeasureSpec height_spec = GetChildMeasureSpec(
    parent_height_spec,
    layout_params_->top_margin + layout_params_->bottom_margin + height_used,
    layout_params_->height_mode,
    layout_params_->height);

  measured_size_ = OnMeasure(width_spec, height_spec);
  return measured_size_;
}

MeasuredSize Widget::Measure(MeasureSpec width_spec, MeasureSpec height_spec) {
  measured_size_ = OnMeasure(width_spec, height_spec);
	return measured_size_;
}

MeasuredSize Widget::OnMeasure(MeasureSpec width_spec, MeasureSpec height_spec) {
	auto self_size = OnMeasureSelf();
  float max_width = self_size[0];
  float max_height = self_size[1];

	for (auto child : children_) {
    if (!child->is_visible()) {
      // Ignore invisible children.
      continue;
		}

    MeasuredSize child_size = child->MeasureChild(width_spec, 0.f, height_spec, 0.f);
    float width =
        child_size.width
        + child->layout_params_->left_margin
        + child->layout_params_->right_margin;
		float height =
        child_size.height
        + child->layout_params_->top_margin
        + child->layout_params_->bottom_margin;

    max_width = std::max(max_width, width);
    max_height = std::max(max_height, height);
  }

	return ResolveSize(width_spec, max_width, height_spec, max_height);
}

Point Widget::OnMeasureSelf() {
  return Point(0.f, 0.f);
}

void Widget::PerformLayout(float top, float right, float bottom, float left) {
  this->x_ = left;
  this->y_ = top;
  this->width_ = right - left;
  this->height_ = bottom - top;

	OnLayout(top, right, bottom, left);
}

void Widget::OnLayout(float top, float right, float bottom, float left) {
  for (auto child : children_) {
    if (!child->is_visible()) {
      // Ignore invisible children.
      continue;
    }

    auto lp = child->layout_params_;
    auto measured_size = child->get_measured_size();

    float child_left = 0;
    float child_top = 0;

		if (lp->gravity & LayoutParams::Gravity::kRight) {
      child_left = width_ - measured_size.width - lp->right_margin;
    } else if (lp->gravity & LayoutParams::Gravity::kCenterHorizontal) {
      child_left = (width_ - measured_size.width) / 2.f + lp->left_margin - lp->right_margin;
    } else {
      // kLeft
      child_left = lp->left_margin;
    }
		if (lp->gravity & LayoutParams::Gravity::kBottom) {
      child_top = height_ - measured_size.height - lp->bottom_margin;
    } else if (lp->gravity & LayoutParams::Gravity::kCenterVertical) {
      child_top = (height_ - measured_size.height) / 2.f + lp->top_margin - lp->bottom_margin;
    } else {
      // kTop
      child_top = lp->top_margin;
    }

    child->PerformLayout(
      child_top,
      measured_size.width + child_left,
      measured_size.height + child_top,
      child_left);
  }
}

std::shared_ptr<Widget> Widget::get_parent() {
  return parent_.lock();
}

std::shared_ptr<Widget> Widget::get_root() {
  std::shared_ptr<Widget> root = this->shared_from_this();
  std::shared_ptr<Widget> parent = root->parent_.lock();
  while (parent) {
    root = parent;
    parent = root->parent_.lock();
  }
  return root;
}

std::string Widget::get_cursor_name() const {
  return "arrow";
}

void Widget::on_focus_gained() {
  focused_ = true;
}

void Widget::on_focus_lost() {
  focused_ = false;
}

void Widget::update(float dt) {
  for(auto &child : children_) {
    child->update(dt);
  }
}

std::stack<fw::Rectangle<float>> scissor_rectangles;

bool Widget::prerender() {
  fw::Rectangle<float> rect = GetScreenRect();
  if (!scissor_rectangles.empty()) {
    fw::Rectangle<float> const &top = scissor_rectangles.top();
    rect = fw::Rectangle<float>::intersect(top, rect);
  }
  if (rect.width <= 0 || rect.height <= 0) {
    return false;
  }

  scissor_rectangles.push(rect);
  glScissor(
      rect.left,
      fw::Get<Gui>().get_height() - rect.top - rect.height,
      rect.width,
      rect.height);
  return true;
}

void Widget::render() {
  for(auto &child : children_) {
    if (child->is_visible() && child->prerender()) {
      child->render();
      child->postrender();
    }
  }
}

void Widget::postrender() {
  scissor_rectangles.pop();
  if (!scissor_rectangles.empty()) {
    fw::Rectangle<float> const &top = scissor_rectangles.top();
    glScissor(
        top.left,
        fw::Get<Gui>().get_height() - top.top - top.height,
        top.width,
        top.height);
  }
}

bool Widget::on_mouse_down(float x, float y) {
  return enabled_ && on_click_ != nullptr;
}

bool Widget::on_mouse_up(float x, float y) {
  if (enabled_ && on_click_) {
    return on_click_(*this);
  }
  return false;
}

std::shared_ptr<Widget> Widget::GetChildAt(float x, float y) {
  // If we're outside the given (x,y) then return null.
  auto rect = GetScreenRect();
  if (x < rect.left || y < rect.top  || x >= rect.right() || y >= rect.bottom()) {
    return nullptr;
  }

  // If one of our children is within the (x,y) then return that.
  for(auto &child : children_) {
    auto found = child->GetChildAt(x, y);
    if (found) {
      return found;
    }
  }

  // Otherwise, return ourselves.
  return this->shared_from_this();
}

std::shared_ptr<Widget> Widget::Find(int id) {
  if (id_ == id) {
    return this->shared_from_this();
  }
  for(auto &child : children_) {
    auto found = child->Find(id);
    if (found != nullptr) {
      return found;
    }
  }
  return nullptr;
}

std::any const &Widget::get_data() const {
  return data_;
}

void Widget::set_data(std::any const &data) {
  data_ = data;
}

bool Widget::IsChild(std::shared_ptr<Widget const> w) const {
  if (w == nullptr) {
    return false;
  }

  for(auto child : children_) {
    if (w == child) {
      return true;
    } else if (child->IsChild(w)) {
      return true;
    }
  }

  return false;
}

bool Widget::IsChild(Widget const &w) const {
  for(auto child : children_) {
    if (&w == child.get()) {
      return true;
    } else if (child->IsChild(w)) {
      return true;
    }
  }

  return false;
}

void Widget::set_visible(bool visible) {
  visible_ = visible;
  RequestLayout();
}

void Widget::set_enabled(bool enabled) {
  enabled_ = enabled;
}

fw::Point Widget::GetScreenPosition() {
	fw::Point pos(x_, y_);

  auto parent = parent_.lock();
  if (parent) {
    pos += parent->GetScreenPosition();
	}

  return pos;
}

fw::Rectangle<float> Widget::GetScreenRect() {
  auto pos = GetScreenPosition();
	auto size = get_measured_size();
	return fw::Rectangle<float>(pos[0], pos[1], size.width, size.height);
}

}
