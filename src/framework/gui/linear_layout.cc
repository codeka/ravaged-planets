
#include <memory>

#include <framework/gui/builder.h>
#include <framework/gui/linear_layout.h>

namespace fw::gui {

class LinearLayoutOrientationProperty : public Property {
public:
	LinearLayoutOrientationProperty(LinearLayout::Orientation orientation)
		: orientation_(orientation) {}

	void apply(Widget& widget) override {
		LinearLayout& layout = dynamic_cast<LinearLayout&>(widget);
		layout.set_orientation(orientation_);
	}
private:
	LinearLayout::Orientation orientation_;
};

std::unique_ptr<Property> LinearLayout::orientation(Orientation orientation) {
	return std::make_unique<LinearLayoutOrientationProperty>(orientation);
}

MeasuredSize LinearLayout::OnMeasure(MeasureSpec width_spec, MeasureSpec height_spec) {
	// Calculate both the total width/height and the max width/height of the children. Depending on
	// the orientation and the measure specs, we will use one or the other to determine our size.
  float max_width = 0.f;
  float total_width = 0.f;
  float max_height = 0.f;
  float total_height = 0.f;

  for (auto child : children_) {
    if (!child->is_visible()) {
      // Ignore invisible children.
      continue;
    }

    auto lp = child->get_layout_params<LinearLayoutParams>();

    MeasuredSize child_size = child->MeasureChild(width_spec, 0.f, height_spec, 0.f);
    float width =
      child_size.width
      + lp->left_margin
      + lp->right_margin;
    float height =
      child_size.height
      + lp->top_margin
      + lp->bottom_margin;

    max_width = std::max(max_width, width);
		total_width += width;
    max_height = std::max(max_height, height);
		total_height += height;
  }

	if (orientation_ == Orientation::kHorizontal) {
    return ResolveSize(width_spec, total_width, height_spec, max_height);
  } else {
    return ResolveSize(width_spec, max_width, height_spec, total_height);
  }
}

void LinearLayout::OnLayout(float top, float right, float bottom, float left) {
	float current_x = left;
	float current_y = top;

  for (auto child : children_) {
    if (!child->is_visible()) {
      // Ignore invisible children.
      continue;
    }

    auto lp = child->get_layout_params<LinearLayoutParams>();
    auto measured_size = child->get_measured_size();

    float child_left = current_x;
    float child_top = current_y;

    // TODO: gravity?
    child_left += lp->left_margin;
    child_top += lp->top_margin;

    child->PerformLayout(
      child_top,
      measured_size.width + child_left,
      measured_size.height + child_top,
      child_left);

		if (orientation_ == Orientation::kHorizontal) {
      current_x += measured_size.width + lp->left_margin + lp->right_margin;
    } else {
      current_y += measured_size.height + lp->top_margin + lp->bottom_margin;
    }
  }
}


}  // namespace fw::gui