#pragma once

#include <memory>
#include <string_view>

#include <framework/gui/drawable.h>
#include <framework/gui/widget.h>

namespace fw::gui {

class WindowInitialPosition {
 public:
  enum class Type {
    kCenter,
    kAbsolute,
  };
  Type type = Type::kCenter;
  float x = 0.f;
  float y = 0.f;

  static WindowInitialPosition Center() {
    return WindowInitialPosition{.type = Type::kCenter, .x = 0, .y = 0};
  }
  static WindowInitialPosition CenterOffset(float x, float y) {
    return WindowInitialPosition{ .type = Type::kCenter, .x = x, .y = y };
  }
  static WindowInitialPosition Absolute(float x, float y) {
    return WindowInitialPosition{.type = Type::kAbsolute, .x = x, .y = y};
  }
};

// Represents a top-level window, complete with controls for moving and so on. All rendering
// happens inside a window.
class Window : public Widget {
private:
  friend class WindowBackgroundProperty;
  friend class WindowInitialPositionProperty;

  std::shared_ptr<Drawable> background_;
	WindowInitialPosition initial_position_;
	bool need_layout_ = true;
	bool need_initial_position_ = false;

public:
  Window();
  ~Window();

	static std::unique_ptr<Property> initial_position(WindowInitialPosition initial_position);
  static std::unique_ptr<Property> background(std::string_view drawable_name);

	void update(float dt) override;
  void render() override;

  // Called by the Gui system when this window is attached.
	void OnAttachedToGui();
  // Called by the Gui system when this window is detached.
  void OnDetachedFromGui();

	void RequestLayout() override;

  void set_visible(bool visible) override;
};

}
