#pragma once

#include <memory>

#include <framework/gui/widget.h>

namespace fw::gui {

class LinearLayoutParams : public LayoutParams {
public:
	LinearLayoutParams(
			LayoutParams::Mode width_mode,
			float width,
			LayoutParams::Mode height_mode,
			float height)
		: LayoutParams(width_mode, width, height_mode, height) {
	}

	float weight = 0.f;

	void CopyFrom(LayoutParams const& other) override {
		LayoutParams::CopyFrom(other);

		LinearLayoutParams const *other_linear = dynamic_cast<LinearLayoutParams const*>(&other);
		if (other_linear) {
			weight = other_linear->weight;
		}
	}
};

// LinearLayout is a container layout that aligns children in a row or column, depending on the
// orientation. Similar to LinearLayout in Android.
class LinearLayout : public Widget {
 public:

	enum class Orientation {
		kHorizontal,
		kVertical,
	};

  LinearLayout() = default;
	~LinearLayout() = default;

	static std::unique_ptr<Property> orientation(Orientation orientation);

	// This property sets the "weight" of the widget's layout params. The weight is used to
	// distribute widgets in the direction of the layout (e.g. horizontally for a horizontal layout).
	static std::unique_ptr<Property> weight(float weight);

	void set_orientation(Orientation orientation) {
		orientation_ = orientation;
		RequestLayout();
	}

	float CalculateItemsTotalHeight() const;

protected:
	MeasuredSize OnMeasure(MeasureSpec width_spec, MeasureSpec height_spec) override;
	void OnLayout(float top, float right, float bottom, float left) override;

	std::shared_ptr<LayoutParams> CreateLayoutParams() override {
		return std::make_shared<LinearLayoutParams>(
			LayoutParams::Mode::kFixed, 0.f, LayoutParams::Mode::kFixed, 0.f);
	}
private:
  friend class LinearLayoutOrientationProperty;

	Orientation orientation_ = Orientation::kHorizontal;
};

}  // namespace fw::gui