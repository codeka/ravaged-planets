#pragma once

#include <list>

#include <boost/function.hpp>
#include <boost/thread.hpp>

namespace fw {
class graphics;
class framework;
class font;
class timer;
class camera;
class bitmap;
class particle_manager;
struct screenshot_request;
class audio_manager;
class lang;
class input;

namespace gui {
class gui;
}

namespace sg {
class scenegraph;
}

// This is the "interface" that the main game class must implement.
class base_app {
public:
  virtual ~base_app() {
  }

  // This is called to initialise the application
  virtual bool initialize(framework *frmwrk) = 0;

  // This is called when the application is exiting.
  virtual void destroy() = 0;

  // This is called each frame to perform the "update" phase
  virtual void update(float dt) = 0;

  // This is called each frame to perform the "render" phase - we need to basically add
  // nodes to the given scenegraph object
  virtual void render(sg::scenegraph &scenegraph) = 0;
};

// This is the main "framework" class, which contains the interface for working with windows and so on.
class framework {
private:

  void on_create_device();
  void on_destroy_device();
  void update(float dt);
  void render();

  bool _active;

  input * _input;
  graphics * _graphics;
  base_app * _app;
  timer *_timer;
  camera *_camera;
  gui::gui *_gui;
  particle_manager *_particle_mgr;
  bool _paused;
  audio_manager *_audio;
  lang *_lang;
  volatile bool _running;

  // game updates happen (synchronized) on this thread in constant timestep
  void update_proc();

  std::list<screenshot_request *> _screenshots;
  void take_screenshots(sg::scenegraph &scenegraph);

  void language_initialize();

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
  void take_screenshot(int with, int height,
      boost::function<void(fw::bitmap const &bmp)> callback_fn);

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

  // toggle between windowed and fullscreen mode.
  void toggle_fullscreen();

  timer *get_timer() const {
    return _timer;
  }
  input *get_input() const {
    return _input;
  }
  graphics *get_graphics() const {
    return _graphics;
  }
  gui::gui *get_gui() const {
    return _gui;
  }
  base_app *get_app() const {
    return _app;
  }
  particle_manager *get_particle_mgr() const {
    return _particle_mgr;
  }
  audio_manager *get_audio() const {
    return _audio;
  }
  lang *get_lang() const {
    return _lang;
  }
};

}
