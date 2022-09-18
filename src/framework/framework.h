#pragma once

#include <functional>
#include <list>
#include <memory>

namespace fw {
class graphics;
class framework;
class font_manager;
class debug_view;
class model_manager;
class timer;
class camera;
class bitmap;
class particle_manager;
struct screenshot_request;
class audio_manager;
class lang;
class cursor;
class input;

namespace gui {
class Gui;
}

namespace sg {
class scenegraph;
}

// This is the "interface" that the main game class must implement.
class base_app {
public:
  virtual ~base_app() {
  }

  /** Override and return false if you don't want graphics to be initialized. */
  virtual bool wants_graphics() {
    return true;
  }

  // This is called to initialize the application
  virtual bool initialize(framework *frmwrk) {
    return false;
  }

  // This is called when the application is exiting.
  virtual void destroy() {
  }

  // This is called each frame to perform the "update" phase
  virtual void update(float dt) {
  }

  // This is called each frame to perform the "render" phase - we need to basically add
  // nodes to the given scenegraph object
  virtual void render(sg::scenegraph &scenegraph) {
  }
};

/** A \ref base_app class that you can use when you don't want graphics. */
class tool_application: public fw::base_app {
public:
  tool_application() {
  }
  ~tool_application() {
  }

  virtual bool wants_graphics() {
    return false;
  }
};

// This is the main "framework" class, which contains the interface for working with windows and so on.
class framework {
private:

  void on_create_device();
  void on_destroy_device();
  void update(float dt);
  void render();
  bool wait_events();
  bool poll_events();

  bool _active;

  input * _input;
  graphics * _graphics;
  base_app * _app;
  timer *_timer;
  camera *_camera;
  gui::Gui *_gui;
  particle_manager *_particle_mgr;
  bool _paused;
  audio_manager *_audio_manager;
  model_manager *_model_manager;
  cursor *_cursor;
  lang *_lang;
  font_manager *_font_manager;
  debug_view *_debug_view;
  volatile bool _running;

  // game updates happen (synchronized) on this thread in constant timestep
  void update_proc();

  std::list<screenshot_request *> _screenshots;
  void take_screenshots(sg::scenegraph &scenegraph);

  void language_initialize();

  void on_fullscreen_toggle(std::string keyname, bool is_down);

public:
  // construct a new framework that'll call the methods of the given base_app
  framework(base_app* app);

  // this is called when the framework shuts down.
  ~framework();

  static framework *get_instance();

  void deactivate();
  void reactivate();

  void pause() {
    _paused = true;
  }
  void unpause() {
    _paused = false;
  }
  bool is_paused() const {
    return _paused;
  }

  // takes a screenshot at the end of the next frame, saves it to an fw::bitmap and
  // calls the given callback with the bitmap, specify 0 for width/height to take a
  // screenshot at the current resolution
  void take_screenshot(int with, int height, std::function<void(std::shared_ptr<fw::bitmap> bmp)> callback_fn,
      bool include_gui = true);

  // gets or sets the camera we'll use for camera control
  void set_camera(camera *cam);
  camera *get_camera() {
    return _camera;
  }

  // Initializes the game and gets everything ready to go.
  bool initialize(char const *title);

  // shutdown the framework, this is called automatically when the main window
  // is closed/destroyed.
  void destroy();

  // runs the main game message loop
  void run();

  // exit the game
  void exit();

  timer *get_timer() const {
    return _timer;
  }
  input *get_input() const {
    return _input;
  }
  graphics *get_graphics() const {
    return _graphics;
  }
  gui::Gui *get_gui() const {
    return _gui;
  }
  base_app *get_app() const {
    return _app;
  }
  font_manager *get_font_manager() const {
    return _font_manager;
  }
  model_manager *get_model_manager() const {
    return _model_manager;
  }
  particle_manager *get_particle_mgr() const {
    return _particle_mgr;
  }
  cursor *get_cursor() const {
    return _cursor;
  }
  audio_manager *get_audio_manager() const {
    return _audio_manager;
  }
  lang *get_lang() const {
    return _lang;
  }
};

}
