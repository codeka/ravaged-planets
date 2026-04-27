
#include <memory>

#include <framework/gui/builder.h>
#include <framework/gui/linear_layout.h>

namespace fw::gui {

class OrientationProperty : public Property {
public:
  OrientationProperty(LinearLayout::Orientation orientation)
		: orientation_(orientation) {}

	void apply(Widget& widget) override {
		LinearLayout* layout = dynamic_cast<LinearLayout*>(&widget);
    if (!layout) {
      LOG(WARN) << "LinearLayout::orientation set on non-LinearLayout widget " << widget.get_name();
			return;
    }
		layout->set_orientation(orientation_);
	}
private:
	LinearLayout::Orientation orientation_;
};

class WeightProperty : public Property {
public:
  WeightProperty(float weight)
    : weight_(weight) {}

  void apply(Widget& widget) override {
    auto lp = widget.get_layout_params<LinearLayoutParams>();
    if (!lp) {
      LOG(WARN) << "LinearLayout::weight set on widget "
                << widget.get_name()
                << " with non-LinearLayout layout params";
			return;
    }
   	lp->weight = weight_;
  }
private:
  float weight_;
};

std::unique_ptr<Property> LinearLayout::orientation(Orientation orientation) {
	return std::make_unique<OrientationProperty>(orientation);
}

std::unique_ptr<Property> LinearLayout::weight(float weight) {
	return std::make_unique<WeightProperty>(weight);
}

MeasuredSize LinearLayout::OnMeasure(MeasureSpec width_spec, MeasureSpec height_spec) {
  // Calculate both the total width/height and the max width/height of the children. Depending on
  // the orientation and the measure specs, we will use one or the other to determine our size.
  float max_width = 0.f;
  float total_width = 0.f;
  float max_height = 0.f;
  float total_height = 0.f;
  float total_weight = 0.f;

  for (auto child : children_) {
    if (!child->is_visible()) {
      // Ignore invisible children.
      continue;
    }

    auto lp = child->get_layout_params<LinearLayoutParams>();
    float width = lp->left_margin + lp->right_margin;
    float height = lp->top_margin + lp->bottom_margin;

    MeasuredSize child_size = child->MeasureChild(width_spec,  0.f, height_spec, 0.f);
    if (lp->weight > 0.f) {
      // Skip normal measurement (in the direction of the orientation) when there's a non-zero 
      // weight.
      if (orientation_ == Orientation::kHorizontal) {
        height += child_size.height;
        width = 0.f;
      } else {
        width += child_size.width;
        height = 0.f;
      }
      total_weight += lp->weight;
    } else {
      width += child_size.width;
      height += child_size.height;
    }

    max_width = std::max(max_width, width);
    total_width += width;
    max_height = std::max(max_height, height);
    total_height += height;
  }

	// Note: weighted views assume the parent has some kind of fixed size. If the parent is
  // wrap_content, then the weighted views will not get any extra space.
  float remaining_space = 0.f;
  if (orientation_ == Orientation::kHorizontal) {
    remaining_space = width_spec.size - total_width;
  } else {
    remaining_space = height_spec.size - total_height;
	}
  
  // Second pass to calculate the weighted views.
	for (auto child : children_) {
    if (!child->is_visible()) {
      // Ignore invisible children.
      continue;
    }
    auto lp = child->get_layout_params<LinearLayoutParams>();
    if (lp->weight > 0.f) {
      float extra_space = (lp->weight / total_weight) * remaining_space;

      // If the child has a weight, then we need to give it all the remaining space in the
      // direction of the orientation.
      MeasureSpec new_width_spec = width_spec;
      float width_used = 0.f;
			MeasureSpec new_height_spec = height_spec;
			float height_used = 0.f;
			if (orientation_ == Orientation::kHorizontal) {
        new_width_spec = MeasureSpec::Exactly(extra_space);
				width_used = width_spec.size - extra_space;
				lp->width_mode = LayoutParams::Mode::kMatchParent;
      } else {
        new_height_spec = MeasureSpec::Exactly(extra_space);
        height_used = height_spec.size - extra_space;
        lp->height_mode = LayoutParams::Mode::kMatchParent;
      }

      // Re-measure the size with the new remaining space.
      MeasuredSize child_size = child->MeasureChild(
          new_width_spec,
          0.f,
          new_height_spec,
          0.f);

      if (orientation_ == Orientation::kHorizontal) {
        total_width += extra_space;
      } else {
        total_height += extra_space;
      }
      max_width = std::max(max_width, child_size.width);
      max_height = std::max(max_height, child_size.height);
    }
  }

  if (orientation_ == Orientation::kHorizontal) {
    return ResolveSize(width_spec, total_width, height_spec, max_height);
  } else {
    return ResolveSize(width_spec, max_width, height_spec, total_height);
  }
}

void LinearLayout::OnLayout(float top, float right, float bottom, float left) {
	float current_x = 0.f;
	float current_y = 0.f;

  for (auto child : children_) {
    if (!child->is_visible()) {
      // Ignore invisible children.
      continue;
    }

    auto lp = child->get_layout_params<LinearLayoutParams>();
    auto measured_size = child->get_measured_size();

    float child_left = current_x;
    float child_top = current_y;

    if (lp->gravity & LayoutParams::Gravity::kRight) {
      child_left += width_ - measured_size.width - lp->right_margin;
    } else if (lp->gravity & LayoutParams::Gravity::kCenterHorizontal) {
      child_left += (width_ - measured_size.width) / 2.f + lp->left_margin - lp->right_margin;
    } else {
      // kLeft
			child_left += lp->left_margin;
    }
    if (lp->gravity & LayoutParams::Gravity::kBottom) {
      child_top += height_ - measured_size.height - lp->bottom_margin;
    } else if (lp->gravity & LayoutParams::Gravity::kCenterVertical) {
      child_top += (height_ - measured_size.height) / 2.f + lp->top_margin - lp->bottom_margin;
    } else {
      // kTop
      child_top += lp->top_margin;
    }

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

float LinearLayout::CalculateItemsTotalHeight() const {
  float total_height = 0.f;
  for (auto const& child : children_) {
    total_height += child->get_height();
  }
  return total_height;
}

}  // namespace fw::gui