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

	void set_orientation(Orientation orientation) {
		orientation_ = orientation;
		RequestLayout();
	}

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