#pragma once

#include <memory>

#include <framework/gui/widget.h>

namespace fw::gui {

class GridLayoutParams : public LayoutParams {
public:
	GridLayoutParams(
		LayoutParams::Mode width_mode,
		float width,
		LayoutParams::Mode height_mode,
		float height)
		: LayoutParams(width_mode, width, height_mode, height) {}

	int row = 0;
	int column = 0;

	void CopyFrom(LayoutParams const& other) override {
		LayoutParams::CopyFrom(other);

		GridLayoutParams const* other_linear = dynamic_cast<GridLayoutParams const*>(&other);
		if (other_linear) {
			row = other_linear->row;
			column = other_linear->column;
		}
	}
};

// GridLayout is a container layout that aligns children in a grid -- rows and columns.
class GridLayout : public Widget {
public:
	GridLayout() = default;
	~GridLayout() = default;

	// Sets the row/column of the child widget. By default, the row/column will be automatically
	// assigned so that views occupy each row/colum in sequence.
	static std::unique_ptr<Property> row(int row);
	static std::unique_ptr<Property> column(int column);

	// Sets the number of colums of this grid layout. The number of rows is automatically determined
	// based on the number children (e.g. if they specifically have a row property set). If this is
	// not specified, the default is 2 colums.
	static std::unique_ptr<Property> grid_size(int num_columns);

	void set_num_columns(int num_columns) {
		num_columns_ = num_columns;
		RequestLayout();
	}

protected:
	MeasuredSize OnMeasure(MeasureSpec width_spec, MeasureSpec height_spec) override;
	void OnLayout(float top, float right, float bottom, float left) override;

	std::shared_ptr<LayoutParams> CreateLayoutParams() override {
		return std::make_shared<GridLayoutParams>(
			LayoutParams::Mode::kFixed, 0.f, LayoutParams::Mode::kFixed, 0.f);
	}
private:
	int num_columns_ = 2;

	// Count the number of rows in this grid layout. We look for the child with the largest "row"
	// layout property, or num_children/num_columns, whichever is larger.
	int CountNumRows() const;

	// After an onMeasure, the final "top, left" of every cell. This is a 1D array for performance,
	// access as measured_cell_top_left_[row * num_columns_ + column].
	std::vector<fw::Point> measured_cell_top_left_;

	std::shared_ptr<Widget> FindChildAt(int row, int col) const;
};

}  // namespace fw::gui