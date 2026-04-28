#include <framework/gui/grid_layout.h>

#include <memory>

#include <framework/gui/widget.h>

namespace fw::gui {

class RowProperty : public Property {
public:
	RowProperty(int row)
		: row_(row) {}

	void apply(Widget& widget) override {
		auto lp = widget.get_layout_params<GridLayoutParams>();
		if (!lp) {
			LOG(WARN) << "GridLayoutParams::row set on widget "
				<< widget.get_name()
				<< " with non-GridLayout layout params";
			return;
		}
		lp->row = row_;
	}
private:
	int row_;
};

class ColumnProperty : public Property {
public:
	ColumnProperty(int column)
		: column_(column) {}

	void apply(Widget& widget) override {
		auto lp = widget.get_layout_params<GridLayoutParams>();
		if (!lp) {
			LOG(WARN) << "GridLayoutParams::column set on widget "
				<< widget.get_name()
				<< " with non-GridLayout layout params";
			return;
		}
		lp->column = column_;
	}
private:
	int column_;
};

class GridSizeProperty : public Property {
public:
	GridSizeProperty(int num_columns)
		: num_columns_(num_columns) {}

	void apply(Widget& widget) override {
		GridLayout* layout = dynamic_cast<GridLayout*>(&widget);
		if (!layout) {
			LOG(WARN) << "GridLayout::grid_size set on non-GridLayout widget " << widget.get_name();
			return;
		}
		layout->set_num_columns(num_columns_);
	}
private:
	int num_columns_;
};

std::unique_ptr<Property> GridLayout::row(int row) {
	return std::make_unique<RowProperty>(row);
}

std::unique_ptr<Property> GridLayout::column(int column) {
	return std::make_unique<ColumnProperty>(column);
}

std::unique_ptr<Property> GridLayout::grid_size(int num_columns) {
	return std::make_unique<GridSizeProperty>(num_columns);
}

MeasuredSize GridLayout::OnMeasure(MeasureSpec width_spec, MeasureSpec height_spec) {
	const int num_cols = num_columns_;
	const int num_rows = CountNumRows();

	// For slightly faster performance, just use a dynamic 1D array, access as
	// "row * num_columns + col"
	auto cell_sizes = new fw::Point[num_rows * num_cols];
	auto row_sizes = new float[num_rows] { 0.f };
	auto col_sizes = new float[num_cols] { 0.f };

	auto row_match_parent = new bool[num_rows] { false };
	auto col_match_parent = new bool[num_cols] { false };
	int num_row_match_parent = 0;
	int num_col_match_parent = 0;

	for (int row = 0; row < num_rows; row++) {
		for (int col = 0; col < num_cols; col++) {
			auto child = FindChildAt(row, col);
			if (!child || !child->is_visible()) {
				continue;
			}

			MeasuredSize child_size = child->MeasureChild(width_spec, 0.0f, height_spec, 0.0f);
			// If the child is set to "match_parent" in either direction, ignore that. We'll distribute
			// the sizes in a second pass later.
			auto lp = child->get_layout_params<GridLayoutParams>();
			float width = lp->left_margin + lp->right_margin + child_size.width;
			float height = lp->top_margin + lp->bottom_margin + child_size.height;
			if (lp->width_mode == LayoutParams::kMatchParent) {
				width = 0.0f;
				if (!col_match_parent[col]) {
					col_match_parent[col] = true;
					num_col_match_parent++;
				}
			}
			if (lp->height_mode == LayoutParams::kMatchParent) {
				height = 0.0f;
				if (!row_match_parent[row]) {
					row_match_parent[col] = true;
					num_row_match_parent++;
				}
			}

			cell_sizes[row * num_cols + col] = fw::Point(width, height);
			row_sizes[row] = std::max(row_sizes[row], height);
			col_sizes[col] = std::max(col_sizes[col], width);
		}
	}

	// Calculate the total width/height of all the children, once they're layed out, not counting any
	// match_parent children.
	float total_width = 0.0f;
	for (int col = 0; col < num_cols; col++) {
		total_width += col_sizes[col];
	}
	float total_height = 0.0f;
	for (int row = 0; row < num_rows; row++) {
		total_height += row_sizes[row];
	}

  float remaining_width = width_spec.size - total_width;
	float remaining_height = height_spec.size - total_height;

	// TODO: allow for width/height weights in row/cols?
	float allowed_width = remaining_width / static_cast<float>(num_col_match_parent);
	float allowed_height = remaining_height / static_cast<float>(num_row_match_parent);

	// Go through and re-layout all the children with match_parent specs.
	if (num_col_match_parent > 0 || num_row_match_parent > 0) {
		for (int row = 0; row < num_rows; row++) {
			for (int col = 0; col < num_cols; col++) {
				auto child = FindChildAt(row, col);
				if (!child || !child->is_visible()) {
					continue;
				}
				auto lp = child->get_layout_params<GridLayoutParams>();

				MeasureSpec new_width_spec = width_spec;
				MeasureSpec new_height_spec = height_spec;
				if (lp->width_mode == LayoutParams::kMatchParent) {
					new_width_spec = MeasureSpec::Exactly(allowed_width);
				}
				if (lp->height_mode == LayoutParams::kMatchParent) {
					new_height_spec = MeasureSpec::Exactly(allowed_height);
				}

				// Re-measure the size with the new remaining space.
				MeasuredSize child_size = child->MeasureChild(new_width_spec, 0.f, new_height_spec, 0.f);
				const float width = lp->left_margin + lp->right_margin + child_size.width;
				const float height = lp->top_margin + lp->bottom_margin + child_size.height;
				col_sizes[col] = std::max(col_sizes[col], width);
				row_sizes[col] = std::max(row_sizes[col], height);
			}
		}

		total_width = 0.0f;
		for (int col = 0; col < num_cols; col++) {
			total_width += col_sizes[col];
		}
		total_height = 0.0f;
		for (int row = 0; row < num_rows; row++) {
			total_height += row_sizes[row];
		}
	}

	// Finally, go through each row/column and calculate the top/left of each 'cell'.
	measured_cell_top_left_.resize(num_rows * num_cols);
	float row_offset = 0.0f;
	for (int row = 0; row < num_rows; row++) {
		float col_offset = 0.0f;
		for (int col = 0; col < num_cols; col++) {
			measured_cell_top_left_[row * num_cols + col] = fw::Point(col_offset, row_offset);

			col_offset += col_sizes[col];
		}
		row_offset += row_sizes[row];
	}

	return ResolveSize(width_spec, total_width, height_spec, total_height);
}

void GridLayout::OnLayout(float top, float right, float bottom, float left) {
	const int num_cols = num_columns_;
	const int num_rows = CountNumRows();

	for (int row = 0; row < num_rows; row++) {
		for (int col = 0; col < num_cols; col++) {
			auto child = FindChildAt(row, col);
			if (!child || !child->is_visible()) {
				continue;
			}

			auto offset = measured_cell_top_left_[row * num_cols + col];
			auto child_size = child->get_measured_size();
			auto lp = child->get_layout_params<GridLayoutParams>();

			// TODO: gravity

			child->PerformLayout(
				offset[1] + lp->top_margin,
				offset[0] + child_size.width,
				offset[1] + child_size.height,
				offset[0] + lp->left_margin);
		}
	}

}

int GridLayout::CountNumRows() const {
	int max_row = 0;
	for (auto child : children_) {
		auto lp = child->get_layout_params<GridLayoutParams>();
		max_row = std::max(max_row, lp->row);
	}

	int implicit_row_count =
			static_cast<int>(std::ceil(static_cast<float>(children_.size()) / num_columns_));

	return std::max(max_row + 1, implicit_row_count);
}

std::shared_ptr<Widget> GridLayout::FindChildAt(int row, int col) const {
	int index = 0;
	for (int curr_row = 0; index < children_.size(); curr_row++) {
		for (int curr_col = 0; curr_col < num_columns_ && index < children_.size(); curr_col++) {
			auto child = children_[index];
			auto lp = child->get_layout_params<GridLayoutParams>();
			if (lp->row > curr_row) {
				curr_row = lp->row;
			}
			if (lp->column > curr_col) {
				curr_col = lp->column;
			}

			if (curr_row == row && curr_col == col) {
				return child;
			}

			index++;
		}
	}

	return nullptr;
}

}