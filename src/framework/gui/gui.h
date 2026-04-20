#pragma once

#include <mutex>
#include <optional>
#include <vector>

#include <framework/audio.h>
#include <framework/status.h>
#include <framework/signals.h>
#include <framework/gui/drawable.h>
#include <framework/gui/window.h>

namespace fw::gui {
class Widget;

// This is the main entry point into the GUI subsystem. You get an instance of this class from
// \ref fw::Framework.
class Gui {
public:
	static std::string service_name;

public:
  Gui() = default;
  ~Gui() = default;

  // Called to initialize the GUI system.
  fw::Status Initialize(fw::AudioManager* audio_manager);

  // Called on the update thread to update the GUI.
  void update(float dt);

  // Called on the render thread to actually render the GUI.
  void render();

  // Register a new top-level widget.
  void AttachWindow(std::shared_ptr<Window> window);

  // Destroys the given top-level widget, unhooks any signals and removes it from the Screen.
  void DetachWindow(std::shared_ptr<Window> window);

  // Returns true if the given widget is attached to the view heriarchy. Once it is, all operations
  // on the widget should be on the render thread.
  bool IsAttached(Widget const &widget);

  // If the given widget is attached to the view heriarchy, ensures that we're running on the
  // render thread. Once attached to the heriarchy, all operations on widgets should be done on the
  // render thread (you can perform operations in the view heriarchy while it's not attached).
  void EnsureThread(Widget const &widget);

  // Bring the given window to the top.
  void BringToTop(std::shared_ptr<Window> window);

  // Give the specified widget Input focus. Keystrokes will be sent to this widget only.
  void Focus(std::shared_ptr<Widget> widget);

  // Gets the one-and-only AudioSource that all GUI element should use to play audio.
  std::shared_ptr<AudioSource> get_audio_source() { return audio_source_;  }

  int get_width() const;
  int get_height() const;

  // Injects a mouse button up/down event, returns true if we handled it or false if it should be
  // passed through.
  bool InjectMouse(int button, bool is_down, float x, float y);

  // Injects a key press, returns true if we handled it or false if it should be passed through.
  bool InjectKey(int key, bool is_down);

  // Global 'click' signal, fired whenever you click the mouse. sig_click is fired if you click a
  // widget, and sig_click_away is fired if you click outside of any widgets.
  fw::Signal<int /*button*/, bool /*is_down*/, Widget const &/*widget*/> sig_click;
  fw::Signal<int /*button*/, bool /*is_down*/> sig_click_away;

  bool is_mouse_over_widget() const {
    return !widget_under_mouse_.expired();
  }

  inline DrawableManager &get_drawable_manager() {
    return drawable_manager_;
  }

private:
  bool enabled_ = false;
  DrawableManager drawable_manager_;
  std::mutex window_mutex_;
  std::vector<std::shared_ptr<Window>> windows_;
  std::vector<std::shared_ptr<Window>> pending_remove_;
  std::weak_ptr<Widget> widget_under_mouse_;
  std::weak_ptr<Widget> widget_mouse_down_;
  std::weak_ptr<Widget> focused_;
  std::shared_ptr<AudioSource> audio_source_;

  // Gets the leaf-most widget at the given (x, y) coordinates, or null if there's no widget.
  std::shared_ptr<Widget> GetWidgetAt(float x, float y);

  void PropagateMouseEvent(std::shared_ptr<Widget> w, bool is_down, float x, float y);
};

} 

