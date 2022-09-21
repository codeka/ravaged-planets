#include <chrono>
#include <functional>
#include <iostream>
#include <stdint.h>
#include <thread>
#include <vector>
#include <boost/foreach.hpp>

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
struct screenshot_request {
  int width;
  int height;
  bool include_gui;
  std::function<void(std::shared_ptr<fw::Bitmap> bmp)> callback;
  std::shared_ptr<fw::Bitmap> bitmap;
};

static framework *only_instance = 0;

framework::framework(base_app* app) :
    _app(app), _active(true), _camera(nullptr), _paused(false), _particle_mgr(nullptr),
    graphics_(nullptr), _timer(nullptr), _audio_manager(nullptr), _input(nullptr), _lang(nullptr),
    _gui(nullptr), _font_manager(nullptr), _model_manager(nullptr), _cursor(nullptr),
    _debug_view(nullptr), _running(true) {
  only_instance = this;
}

framework::~framework() {
  if (graphics_ != nullptr)
    delete graphics_;
  if (_gui != nullptr)
    delete _gui;
  if (_lang != nullptr)
    delete _lang;
  if (_input != nullptr)
    delete _input;
  if (_timer != nullptr)
    delete _timer;
  if (_font_manager != nullptr)
    delete _font_manager;
  if (_particle_mgr != nullptr)
    delete _particle_mgr;
  if (_cursor != nullptr)
    delete _cursor;
  if (_model_manager != nullptr)
    delete _model_manager;
  if (_debug_view != nullptr)
    delete _debug_view;
  if (_audio_manager != nullptr)
    delete _audio_manager;
}

framework *framework::get_instance() {
  return only_instance;
}

bool framework::initialize(char const *title) {
  settings stg;
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

  _timer = new timer();

  // initialize graphics
  if (_app->wants_graphics()) {
    graphics_ = new Graphics();
    graphics_->initialize(title);

    _particle_mgr = new ParticleManager();
    _particle_mgr->initialize(graphics_);

    _model_manager = new ModelManager();

    _cursor = new Cursor();
    _cursor->initialize();
  }

  // initialise audio
  _audio_manager = new AudioManager();
  _audio_manager->initialize();

  _input = new Input();
  _input->initialize();

  _font_manager = new FontManager();
  _font_manager->initialize();

  if (_app->wants_graphics()) {
    _gui = new gui::Gui();
    _gui->initialize(graphics_);
  }

  if (stg.is_set("debug-view") && _app->wants_graphics()) {
    _debug_view = new DebugView();
    _debug_view->initialize();
  }

  net::initialize();
  Http::initialize();

  debug << "framework initialization complete, application initialization starting..." << std::endl;
  if (!_app->initialize(this))
    return false;

  debug << "application initialization complete, running..." << std::endl;

  // start the game timer that'll record fps, elapsed time, etc.
  _timer->start();


  _input->bind_function("toggle-fullscreen", std::bind(&framework::on_fullscreen_toggle, this, _1, _2));

  return true;
}

void framework::on_fullscreen_toggle(std::string keyname, bool is_down) {
  if (!is_down) {
    graphics_->toggle_fullscreen();
  }
}

void framework::language_initialize() {
  settings stg;

  std::vector<LangDescription> langs = fw::get_languages();
  debug << boost::format("%1% installed language(s):") % langs.size() << std::endl;
  BOOST_FOREACH(LangDescription &l, langs) {
    debug << boost::format(" %1% (%2%)") % l.name % l.display_name << std::endl;
  }

  std::string lang_name = stg.get_value<std::string>("lang");
  if (!boost::iends_with(lang_name, ".lang")) {
    lang_name += ".lang";
  }

  _lang = new Lang(lang_name);
}

void framework::destroy() {
  _app->destroy();

  debug << "framework is shutting down..." << std::endl;
  if (_debug_view != nullptr) {
    _debug_view->destroy();
  }

  if (graphics_ != nullptr) {
    graphics_->destroy();
  }
  Http::destroy();
  net::destroy();
  if (_cursor != nullptr) {
    _cursor->destroy();
  }
  _audio_manager->destroy();
}

void framework::deactivate() {
  _active = false;
}

void framework::reactivate() {
  _active = true;
}

void framework::run() {
  // kick off the update thread
  std::thread update_thread(std::bind(&framework::update_proc, this));
  try {
    if (graphics_ == nullptr) {
      wait_events();
      _running = false;
    } else {
      // do the event/render loop
      while (_running) {
        if (!poll_events()) {
          _running = false;
          break;
        }

        render();
      }
    }

    // wait for the update thread to exit (once we set _running to false, it'll
    // stop as well)
    update_thread.join();
  } catch(...) {
    update_thread.detach();
    throw;
  }
}

void framework::exit() {
  fw::debug << "Exiting." << std::endl;
  _running = false;
}

void framework::set_camera(Camera *cam) {
  if (_camera != 0)
    _camera->disable();

  _camera = cam;

  if (_camera != 0)
    _camera->enable();
}

void framework::update_proc() {
  try {
    int64_t accum_micros = 0;
    int64_t timestep_micros = 1000000 / 40; // 40 frames per second update frequency.
    while (_running) {
      _timer->update();
      accum_micros += std::chrono::duration_cast<std::chrono::microseconds>(
          _timer->get_frame_duration()).count();

      int64_t remaining_micros = timestep_micros - accum_micros;
      if (remaining_micros > 1000) {
        std::this_thread::sleep_for(std::chrono::microseconds(remaining_micros));
        continue;
      }

      while (accum_micros > timestep_micros && _running) {
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

void framework::update(float dt) {
  if (_gui != nullptr) {
    _gui->update(dt);
  }
  _font_manager->update(dt);
  _audio_manager->update();
  if (!_paused) {
    _app->update(dt);
    _particle_mgr->update(dt);
  }
  if (_debug_view != nullptr) {
    _debug_view->update(dt);
  }

  if (_camera != nullptr)
    _camera->update(dt);

  _input->update(dt);
}

void framework::render() {
  if (graphics_ == nullptr) {
    return;
  }

  _timer->render();

  // populate the scene graph by calling into the application itself
  sg::scenegraph scenegraph;
  _app->render(scenegraph);
  _particle_mgr->render(scenegraph);

  fw::render(scenegraph);

  // if we've been asked for some screenshots, take them after we've done the normal render.
  if (_screenshots.size() > 0)
    take_screenshots(scenegraph);

  graphics_->after_render();
}

bool framework::poll_events() {
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    _input->process_event(e);
    if (e.type == SDL_QUIT) {
      fw::debug << "Got quit signal, exiting." << std::endl;
      return false;
    }
  }
  return true;
}

bool framework::wait_events() {
  SDL_Event e;
  while (SDL_WaitEvent(&e)) {
    _input->process_event(e);
    if (e.type == SDL_QUIT) {
      fw::debug << "Got quit signal, exiting." << std::endl;
      return false;
    }
  }
  return true;
}

void framework::take_screenshot(int width, int height, std::function<void(std::shared_ptr<fw::Bitmap> bmp)> callback_fn,
    bool include_gui /*= true */) {
  if (width == 0 || height == 0) {
    width = graphics_->get_width();
    height = graphics_->get_height();
  }

  screenshot_request *request = new screenshot_request();
  request->width = width;
  request->height = height;
  request->include_gui = include_gui;
  request->callback = callback_fn;

  _screenshots.push_back(request);
}

/* This is called on another thread to call the callbacks that were registered against the "screenshot" request. */
void call_callacks_thread_proc(std::shared_ptr<std::list<screenshot_request *>> requests) {
  for(screenshot_request *request : *requests) {
    request->callback(request->bitmap);
    delete request;
  }
}

/**
 * This is called at the end of a frame if there are pending screenshot callbacks. We'll grab the contents of the,
 * frame buffer, then (on another thread) call the callbacks.
 */
void framework::take_screenshots(sg::scenegraph &scenegraph) {
  if (_screenshots.empty()) {
    return;
  }

  std::shared_ptr<std::list<screenshot_request *>> requests(new std::list<screenshot_request *>());
  for(screenshot_request *request : _screenshots) {
    // render the scene to a separate render target first
    std::shared_ptr<fw::Texture> color_target(new fw::Texture());
    color_target->create(request->width, request->height, false);

    std::shared_ptr<fw::Texture> depth_target(new fw::Texture());
    depth_target->create(request->width, request->height, true);

    std::shared_ptr<fw::Framebuffer> framebuffer(new fw::Framebuffer());
    framebuffer->set_color_buffer(color_target);
    framebuffer->set_depth_buffer(depth_target);

    fw::render(scenegraph, framebuffer, request->include_gui);

    request->bitmap = std::make_shared<fw::Bitmap>(*color_target.get());
    requests->push_back(request);
  }

  // create a thread to finish the job
  std::thread t(std::bind(&call_callacks_thread_proc, requests));
  t.detach();

  _screenshots.clear();
}

}
