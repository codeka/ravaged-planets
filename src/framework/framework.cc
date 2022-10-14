#include <chrono>
#include <functional>
#include <iostream>
#include <stdint.h>
#include <thread>
#include <vector>

#include <SDL2/SDL.h>

#include <framework/framework.h>
#include <framework/audio.h>
#include <framework/logging.h>
#include <framework/camera.h>
#include <framework/bitmap.h>
#include <framework/exception.h>
#include <framework/settings.h>
#include <framework/graphics.h>
#include <framework/cursor.h>
#include <framework/debug_view.h>
#include <framework/font.h>
#include <framework/particle_manager.h>
#include <framework/model_manager.h>
#include <framework/scenegraph.h>
#include <framework/net.h>
#include <framework/http.h>
#include <framework/timer.h>
#include <framework/texture.h>
#include <framework/lang.h>
#include <framework/misc.h>
#include <framework/input.h>
#include <framework/gui/gui.h>

using namespace std::placeholders;

namespace fw {
struct ScreenshotRequest {
  int width;
  int height;
  bool include_gui;
  std::function<void(std::shared_ptr<fw::Bitmap> bmp)> callback;
  std::shared_ptr<fw::Bitmap> bitmap;
};

static Framework *only_instance = 0;

Framework::Framework(BaseApp* app) :
    app_(app), active_(true), camera_(nullptr), paused_(false), particle_mgr_(nullptr),
    graphics_(nullptr), timer_(nullptr), audio_manager_(nullptr), input_(nullptr), lang_(nullptr),
    gui_(nullptr), font_manager_(nullptr), model_manager_(nullptr), cursor_(nullptr),
    debug_view_(nullptr), scenegraph_manager_(nullptr), running_(true) {
  only_instance = this;
}

Framework::~Framework() {
  if (graphics_ != nullptr)
    delete graphics_;
  if (gui_ != nullptr)
    delete gui_;
  if (lang_ != nullptr)
    delete lang_;
  if (input_ != nullptr)
    delete input_;
  if (timer_ != nullptr)
    delete timer_;
  if (font_manager_ != nullptr)
    delete font_manager_;
  if (particle_mgr_ != nullptr)
    delete particle_mgr_;
  if (cursor_ != nullptr)
    delete cursor_;
  if (model_manager_ != nullptr)
    delete model_manager_;
  if (scenegraph_manager_ != nullptr)
    delete scenegraph_manager_;
  if (debug_view_ != nullptr)
    delete debug_view_;
  if (audio_manager_ != nullptr)
    delete audio_manager_;
}

Framework *Framework::get_instance() {
  return only_instance;
}

bool Framework::initialize(char const *title) {
  Settings stg;
  if (stg.is_set("help")) {
    stg.print_help();
    return false;
  }

  random_initialize();
  logging_initialize();
  language_initialize();

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
    BOOST_THROW_EXCEPTION(fw::Exception() << fw::sdl_error_info(SDL_GetError()));
  }

  timer_ = new Timer();

  // initialize graphics
  if (app_->wants_graphics()) {
    graphics_ = new Graphics();
    graphics_->initialize(title);

    model_manager_ = new ModelManager();
    scenegraph_manager_ = new sg::ScenegraphManager();

    particle_mgr_ = new ParticleManager();
    particle_mgr_->initialize(graphics_);

    cursor_ = new Cursor();
    cursor_->initialize();
  }

  // initialise audio
  audio_manager_ = new AudioManager();
  audio_manager_->initialize();

  input_ = new Input();
  input_->initialize();

  font_manager_ = new FontManager();
  font_manager_->initialize();

  if (app_->wants_graphics()) {
    gui_ = new gui::Gui();
    gui_->initialize(graphics_);
  }

  if (stg.is_set("debug-view") && app_->wants_graphics()) {
    debug_view_ = new DebugView();
    debug_view_->initialize();
  }

  net::initialize();
  Http::initialize();

  debug << "framework initialization complete, application initialization starting..." << std::endl;
  if (!app_->initialize(this))
    return false;

  debug << "application initialization complete, running..." << std::endl;

  // start the game Timer that'll record fps, elapsed time, etc.
  timer_->start();


  input_->bind_function("toggle-fullscreen", std::bind(&Framework::on_fullscreen_toggle, this, _1, _2));

  return true;
}

void Framework::on_fullscreen_toggle(std::string keyname, bool is_down) {
  if (!is_down) {
    graphics_->toggle_fullscreen();
  }
}

void Framework::language_initialize() {
  Settings stg;

  const std::vector<LangDescription> langs = fw::get_languages();
  debug << boost::format("%1% installed language(s):") % langs.size() << std::endl;
  for(const LangDescription &l : langs) {
    debug << boost::format(" %1% (%2%)") % l.name % l.display_name << std::endl;
  }

  std::string lang_name = stg.get_value<std::string>("lang");
  if (!boost::iends_with(lang_name, ".lang")) {
    lang_name += ".lang";
  }

  lang_ = new Lang(lang_name);
}

void Framework::destroy() {
  app_->destroy();

  debug << "framework is shutting down..." << std::endl;
  if (debug_view_ != nullptr) {
    debug_view_->destroy();
  }

  if (graphics_ != nullptr) {
    graphics_->destroy();
  }
  Http::destroy();
  net::destroy();
  if (cursor_ != nullptr) {
    cursor_->destroy();
  }
  audio_manager_->destroy();
}

void Framework::deactivate() {
  active_ = false;
}

void Framework::reactivate() {
  active_ = true;
}

void Framework::run() {
  // kick off the update thread
  std::thread update_thread(std::bind(&Framework::update_proc, this));
  try {
    if (graphics_ == nullptr) {
      wait_events();
      running_ = false;
    } else {
      // do the event/render loop
      while (running_) {
        if (!poll_events()) {
          running_ = false;
          break;
        }

        render();
      }
    }

    // wait for the update thread to exit (once we set running_ to false, it'll stop as well)
    update_thread.join();
  } catch(...) {
    update_thread.detach();
    throw;
  }
}

void Framework::exit() {
  fw::debug << "Exiting." << std::endl;
  running_ = false;
}

void Framework::set_camera(Camera *cam) {
  if (camera_ != 0)
    camera_->disable();

  camera_ = cam;

  if (camera_ != 0)
    camera_->enable();
}

void Framework::update_proc() {
  try {
    int64_t accum_micros = 0;
    int64_t timestep_micros = 1000000 / 40; // 40 frames per second update frequency.
    while (running_) {
      timer_->update();
      accum_micros += std::chrono::duration_cast<std::chrono::microseconds>(
          timer_->get_frame_duration()).count();

      int64_t remaining_micros = timestep_micros - accum_micros;
      if (remaining_micros > 1000) {
        std::this_thread::sleep_for(std::chrono::microseconds(remaining_micros));
        continue;
      }

      while (accum_micros > timestep_micros && running_) {
        float dt = static_cast<float>(timestep_micros) / 1000000.f;
        update(dt);
        accum_micros -= timestep_micros;
      }

      // TODO: should we yield or sleep for a while?
      std::this_thread::yield();
    }
  }catch(std::exception &e) {
    std::string msg = boost::diagnostic_information(e);
    fw::debug << "--------------------------------------------------------------------------------" << std::endl;
    fw::debug << "UNHANDLED EXCEPTION!" << std::endl;
    fw::debug << msg << std::endl;
  } catch (...) {
    fw::debug << "--------------------------------------------------------------------------------" << std::endl;
    fw::debug << "UNHANDLED EXCEPTION! (unknown exception)" << std::endl;
  }
}

void Framework::update(float dt) {
  if (gui_ != nullptr) {
    gui_->update(dt);
  }
  font_manager_->update(dt);
  audio_manager_->update();
  if (!paused_) {
    app_->update(dt);
    particle_mgr_->update(dt);
  }
  if (debug_view_ != nullptr) {
    debug_view_->update(dt);
  }

  if (camera_ != nullptr)
    camera_->update(dt);

  input_->update(dt);
}

void Framework::render() {
  if (graphics_ == nullptr) {
    return;
  }

  timer_->render();

  scenegraph_manager_->before_render();
  
  auto& scenegraph = scenegraph_manager_->get_scenegraph();
  fw::render(scenegraph);

  // if we've been asked for some screenshots, take them after we've done the normal render.
  if (screenshots_.size() > 0)
    take_screenshots(scenegraph);

  graphics_->after_render();
}

bool Framework::poll_events() {
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    input_->process_event(e);
    if (e.type == SDL_QUIT) {
      fw::debug << "Got quit signal, exiting." << std::endl;
      return false;
    }
  }
  return true;
}

bool Framework::wait_events() {
  SDL_Event e;
  while (SDL_WaitEvent(&e)) {
    input_->process_event(e);
    if (e.type == SDL_QUIT) {
      fw::debug << "Got quit signal, exiting." << std::endl;
      return false;
    }
  }
  return true;
}

void Framework::take_screenshot(int width, int height, std::function<void(std::shared_ptr<fw::Bitmap> bmp)> callback_fn,
    bool include_gui /*= true */) {
  if (width == 0 || height == 0) {
    width = graphics_->get_width();
    height = graphics_->get_height();
  }

  ScreenshotRequest *request = new ScreenshotRequest();
  request->width = width;
  request->height = height;
  request->include_gui = include_gui;
  request->callback = callback_fn;

  screenshots_.push_back(request);
}

/* This is called on another thread to call the callbacks that were registered against the "screenshot" request. */
void call_callacks_thread_proc(std::shared_ptr<std::list<ScreenshotRequest *>> requests) {
  for(ScreenshotRequest *request : *requests) {
    request->callback(request->bitmap);
    delete request;
  }
}

/**
 * This is called at the end of a frame if there are pending screenshot callbacks. We'll grab the contents of the,
 * frame buffer, then (on another thread) call the callbacks.
 */
void Framework::take_screenshots(sg::Scenegraph &Scenegraph) {
  if (screenshots_.empty()) {
    return;
  }

  std::shared_ptr<std::list<ScreenshotRequest *>> requests(new std::list<ScreenshotRequest *>());
  for(ScreenshotRequest *request : screenshots_) {
    // render the scene to a separate render target first
    std::shared_ptr<fw::Texture> color_target(new fw::Texture());
    color_target->create(request->width, request->height, false);

    std::shared_ptr<fw::Texture> depth_target(new fw::Texture());
    depth_target->create(request->width, request->height, true);

    std::shared_ptr<fw::Framebuffer> framebuffer(new fw::Framebuffer());
    framebuffer->set_color_buffer(color_target);
    framebuffer->set_depth_buffer(depth_target);

    fw::render(Scenegraph, framebuffer, request->include_gui);

    request->bitmap = std::make_shared<fw::Bitmap>(*color_target.get());
    requests->push_back(request);
  }

  // create a thread to finish the job
  std::thread t(std::bind(&call_callacks_thread_proc, requests));
  t.detach();

  screenshots_.clear();
}

}
