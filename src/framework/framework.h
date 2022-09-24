#pragma once

#include <functional>
#include <list>
#include <memory>

namespace fw {
class Graphics;
class Framework;
class FontManager;
class DebugView;
class ModelManager;
class Timer;
class Camera;
class Bitmap;
class ParticleManager;
struct ScreenshotRequest;
class AudioManager;
class Lang;
class Cursor;
class Input;

namespace gui {
class Gui;
}

namespace sg {
class Scenegraph;
}

// This is the "interface" that the main game class must implement.
class BaseApp {
public:
  virtual ~BaseApp() {
  }

  // Override and return false if you don't want graphics to be initialized.
  virtual bool wants_graphics() {
    return true;
  }

  // This is called to initialize the Application
  virtual bool initialize(Framework *frmwrk) {
    return false;
  }

  // This is called when the Application is exiting.
  virtual void destroy() {
  }

  // This is called each frame to perform the "update" phase
  virtual void update(float dt) {
  }

  // This is called each frame to perform the "render" phase - we need to basically add
  // nodes to the given Scenegraph object
  virtual void render(sg::Scenegraph &Scenegraph) {
  }
};

/** A \ref BaseApp class that you can use when you don't want graphics. */
class ToolApplication: public fw::BaseApp {
public:
  ToolApplication() {
  }
  ~ToolApplication() {
  }

  virtual bool wants_graphics() {
    return false;
  }
};

// This is the main "Framework" class, which contains the interface for working with windows and so on.
class Framework {
private:

  void on_create_device();
  void on_destroy_device();
  void update(float dt);
  void render();
  bool wait_events();
  bool poll_events();

  bool active_;

  Input * input_;
  Graphics * graphics_;
  BaseApp * app_;
  Timer *timer_;
  Camera *camera_;
  gui::Gui *gui_;
  ParticleManager *particle_mgr_;
  bool paused_;
  AudioManager *audio_manager_;
  ModelManager *model_manager_;
  Cursor *cursor_;
  Lang *lang_;
  FontManager *font_manager_;
  DebugView *debug_view_;
  volatile bool running_;

  // game updates happen (synchronized) on this thread in constant timestep
  void update_proc();

  std::list<ScreenshotRequest *> screenshots_;
  void take_screenshots(sg::Scenegraph &Scenegraph);

  void language_initialize();

  void on_fullscreen_toggle(std::string keyname, bool is_down);

public:
  // construct a new Framework that'll call the methods of the given BaseApp
  Framework(BaseApp* app);

  // this is called when the Framework shuts down.
  ~Framework();

  static Framework *get_instance();

  void deactivate();
  void reactivate();

  void pause() {
    paused_ = true;
  }
  void unpause() {
    paused_ = false;
  }
  bool is_paused() const {
    return paused_;
  }

  // takes a screenshot at the end of the next frame, saves it to an fw::Bitmap and
  // calls the given callback with the bitmap, specify 0 for width/height to take a
  // screenshot at the current resolution
  void take_screenshot(int with, int height, std::function<void(std::shared_ptr<fw::Bitmap> bmp)> callback_fn,
      bool include_gui = true);

  // gets or sets the camera we'll use for camera control
  void set_camera(Camera *cam);
  Camera *get_camera() {
    return camera_;
  }

  // Initializes the game and gets everything ready to go.
  bool initialize(char const *title);

  // shutdown the Framework, this is called automatically when the main window
  // is closed/destroyed.
  void destroy();

  // runs the main game message loop
  void run();

  // exit the game
  void exit();

  Timer *get_timer() const {
    return timer_;
  }
  Input *get_input() const {
    return input_;
  }
  Graphics *get_graphics() const {
    return graphics_;
  }
  gui::Gui *get_gui() const {
    return gui_;
  }
  BaseApp *get_app() const {
    return app_;
  }
  FontManager *get_font_manager() const {
    return font_manager_;
  }
  ModelManager *get_model_manager() const {
    return model_manager_;
  }
  ParticleManager *get_particle_mgr() const {
    return particle_mgr_;
  }
  Cursor *get_cursor() const {
    return cursor_;
  }
  AudioManager *get_audio_manager() const {
    return audio_manager_;
  }
  Lang *get_lang() const {
    return lang_;
  }
};

}
