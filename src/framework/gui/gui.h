#pragma once

#include <mutex>
#include <vector>

#define BOOST_BIND_NO_PLACEHOLDERS // so it doesn't auto-include _1, _2 etc.
#include <boost/signals2.hpp>

#include <framework/audio.h>
#include <framework/graphics.h>
#include <framework/gui/drawable.h>

namespace fw::gui {
class Widget;

// This is the main entry point into the GUI subsystem. You get an instance of this class from \ref fw::Framework.
class Gui {
private:
  fw::Graphics *graphics_;
  DrawableManager *drawable_manager_;
  std::mutex top_level_widget_mutex_;
  std::vector<Widget *> top_level_widgets_;
  std::vector<Widget *> pending_remove_;
  Widget *widget_under_mouse_;
  Widget *widget_mouse_down_;
  Widget *focused_;
  std::shared_ptr<AudioSource> audio_source_;

  // Gets the leaf-most widget at the given (x, y) coordinates, or null if there's no widget.
  Widget *get_widget_at(float x, float y);

  void propagate_mouse_event(Widget *w, bool is_down, float x, float y);

public:
  Gui();
  ~Gui();

  // Called to initialize the GUI system.
  void initialize(fw::Graphics *graphics, fw::AudioManager* audio_manager);

  // Called on the update thread to update the GUI.
  void update(float dt);

  // Called on the render thread to actually render the GUI.
  void render();

  // Register a new top-level widget.
  void attach_widget(Widget *widget);

  // Destroys the given top-level widget, unhooks any signals and removes it from the Screen.
  void detach_widget(Widget *widget);

  // Bring the given widget to the top.
  void bring_to_top(Widget *widget);

  // Give the specified widget Input focus. Keystrokes will be sent to this widget only.
  void focus(Widget *widget);

  // Gets the one-and-only AudioSource that all GUI element should use to play audio.
  std::shared_ptr<AudioSource> get_audio_source() { return audio_source_;  }

  int get_width() const;
  int get_height() const;

  // Injects a mouse button up/down event, returns true if we handled it or false if it should be passed through.
  bool inject_mouse(int button, bool is_down, float x, float y);

  // Injects a key press, returns true if we handled it or false if it should be passed through.
  bool inject_key(int key, bool is_down);

  // Global 'click' signal, fired whenever you click the mouse. Widget may be null if you clicked on no widget.
  boost::signals2::signal<void(int button, bool is_down, Widget *widget)> sig_click;

  bool is_mouse_over_widget() const {
    return widget_under_mouse_ != nullptr;
  }

  inline DrawableManager *get_drawable_manager() const {
    return drawable_manager_;
  }
};

} 

