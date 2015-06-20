#include <functional>
#include <iostream>
#include <stdint.h>
#include <thread>
#include <vector>
#include <boost/foreach.hpp>

#include <SDL.h>

#include <framework/framework.h>
#include <framework/logging.h>
#include <framework/camera.h>
#include <framework/exception.h>
#include <framework/settings.h>
#include <framework/graphics.h>
#include <framework/font.h>
#include <framework/particle_manager.h>
#include <framework/model_manager.h>
#include <framework/scenegraph.h>
#include <framework/timer.h>
#include <framework/lang.h>
#include <framework/misc.h>
#include <framework/input.h>
#include <framework/gui/gui.h>

namespace fw {
struct screenshot_request {
  int width;
  int height;
  std::function<void(fw::bitmap const &)> callback;
  fw::bitmap *bitmap;
};

static framework *only_instance = 0;

framework::framework(base_app* app) :
    _app(app), _active(true), _camera(nullptr), _paused(false), _particle_mgr(nullptr),
    _graphics(nullptr), _timer(nullptr), _audio(nullptr), _input(nullptr), _lang(nullptr),
    _gui(nullptr), _font_manager(nullptr), _model_manager(nullptr), _running(true) {
  only_instance = this;
}

framework::~framework() {
  if (_graphics != nullptr)
    delete _graphics;
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
  if (_model_manager != nullptr)
    delete _model_manager;

  /*
   if (_audio != 0) delete _audio;
   */
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

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    BOOST_THROW_EXCEPTION(fw::exception() << fw::sdl_error_info(SDL_GetError()));
  }

  _timer = new timer();

  // initialize graphics
  if (_app->wants_graphics()) {
    _graphics = new graphics();
    _graphics->initialize(title);

    _particle_mgr = new particle_manager();
    _particle_mgr->initialise(_graphics);

    _model_manager = new model_manager();
  }

  /*
   // initialise audio
   _audio->initialise();
   */

  _input = new input();
  _input->initialize();

  _font_manager = new font_manager();
  _font_manager->initialize();

  if (_app->wants_graphics()) {
    _gui = new gui::gui();
    _gui->initialize(_graphics);
  }

  /*
   // initialise the host (ENet) and HTTP module (libcurl)
   net::initialise();
   http::initialise();
   */
  debug << "framework initialization complete, application initialization starting..." << std::endl;
  if (!_app->initialize(this))
    return false;

  debug << "application initialization complete, running..." << std::endl;

  // start the game timer that'll record fps, elapsed time, etc.
  _timer->start();

  return true;
}

void framework::language_initialize() {
  settings stg;

  std::vector<lang_description> langs = fw::get_languages();
  debug << boost::format("%1% installed language(s):") % langs.size() << std::endl;
  BOOST_FOREACH(lang_description &l, langs) {
    debug << boost::format(" %1% (%2%)") % l.name % l.display_name << std::endl;
  }

  std::string lang_name = stg.get_value<std::string>("lang");
  if (!boost::iends_with(lang_name, ".lang")) {
    lang_name += ".lang";
  }

  _lang = new lang(lang_name);
}

void framework::destroy() {
  _app->destroy();

  debug << "framework is shutting down..." << std::endl;

  if (_graphics != nullptr) {
    _graphics->destroy();
  }
  /*
   http::destroy();
   net::destroy();
   _audio->destroy();*/
}

void framework::deactivate() {
  _active = false;
}

void framework::reactivate() {/*
 if (_graphics->get_device() != 0)
 {
 _graphics->sig_device_lost();
 _graphics->reset();
 _graphics->sig_device_reset();
 }
 */
  _active = true;
}

void framework::run() {
  // kick off the update thread
  std::thread update_thread(std::bind(&framework::update_proc, this));
  try {
    if (_graphics == nullptr) {
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

void framework::set_camera(camera *cam) {
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
        usleep(remaining_micros);
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
  /*
   _audio->update();

   */
  if (!_paused) {
    _app->update(dt);
    _particle_mgr->update(dt);
  }

  if (_camera != nullptr)
    _camera->update(dt);

  _input->update(dt);
}

void framework::render() {
  if (_graphics == nullptr) {
    return;
  }

  // populate the scene graph by calling into the application itself
  sg::scenegraph scenegraph;
  _app->render(scenegraph);
  _particle_mgr->render(scenegraph);

  // if we've been asked for some screenshots, take them before we do the
  // normal render.
  // if (_screenshots.size() > 0)
  //   take_screenshots(scenegraph);

  fw::render(scenegraph);
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

void framework::take_screenshot(int width, int height, std::function<void(fw::bitmap const &)> callback_fn) {
  if (width == 0 || height == 0) {
    width = 640; // TODO
    height = 480; // TODO
  }

  screenshot_request *request = new screenshot_request();
  request->width = width;
  request->height = height;
  request->callback = callback_fn;

  _screenshots.push_back(request);
}

// this is called on another thread to call the callbacks that were registered against
// the "screenshot" request.
void call_callacks_thread_proc(
    boost::shared_ptr<std::list<boost::shared_ptr<screenshot_request> > > requests) {
  BOOST_FOREACH(boost::shared_ptr<screenshot_request> request, *requests) {
    request->callback(*request->bitmap);
  }
}

// This is called at the beginning of a frame if there are pending screenshot
// callbacks. We'll take the screenshot, then (on another thread) get the data
// and call the callbacks.
void framework::take_screenshots(sg::scenegraph &scenegraph) {
  boost::shared_ptr<std::list<boost::shared_ptr<screenshot_request> > > requests(
      new std::list<boost::shared_ptr<screenshot_request> >());

  BOOST_FOREACH(screenshot_request *request, _screenshots) {
    // render the scene to a separate render target first
//			boost::shared_ptr<fw::texture> render_target(new fw::texture());
//			render_target->create_rendertarget(request->width, request->height, D3DFMT_X8R8G8B8);

//			_graphics->set_render_target(render_target.get());
//			fw::render(scenegraph);

    // create a local-memory texture that'll hold the actual data
//			fw::texture local_texture;
//			local_texture.create(request->width, request->height, D3DUSAGE_DYNAMIC, D3DFMT_X8R8G8B8, 1, D3DPOOL_SYSTEMMEM);

//			IDirect3DSurface9 *surface = 0;
//			HRESULT hr = local_texture.get_d3dtexture()->GetSurfaceLevel(0, &surface);
//			if (FAILED(hr))
//			{
//				BOOST_THROW_EXCEPTION(fw::exception(hr));
//			}

//			hr = _graphics->get_device()->GetRenderTargetData(render_target->get_render_target(), surface);
//			if (FAILED(hr))
//			{
//				BOOST_THROW_EXCEPTION(fw::exception(hr));
//			}

//			request->bitmap = new fw::bitmap(surface);
//			surface->Release();

//			_graphics->set_render_target(0);

//			requests->push_back(request);
  }

  // create a thread to finish the job
  std::thread t(std::bind(&call_callacks_thread_proc, requests));

  _screenshots.clear();
}

void framework::toggle_fullscreen() {/*
 if (_graphics == 0 || !_active)
 return;

 debug << boost::format("switching to %1%...") % (_graphics->windowed ? "full-screen" : "windowed") << std::endl;
 _graphics->windowed = !_graphics->windowed;

 // Set new window style
 if (_graphics->windowed)
 {
 // Going to windowed mode
 ::SetWindowLongPtr(_hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
 }
 else
 {
 // Save current location/size
 ::ZeroMemory(&_wp, sizeof(WINDOWPLACEMENT));
 _wp.length = sizeof(WINDOWPLACEMENT);
 ::GetWindowPlacement(_hwnd, &_wp);

 // Going to fullscreen mode
 ::SetWindowLongPtr(_hwnd, GWL_STYLE, WS_EX_TOPMOST);

 // Hide the window to avoid animation of blank windows
 ::ShowWindow(_hwnd, SW_HIDE);
 }

 // Reset the Device
 on_lost_device();
 _graphics->reset();
 on_reset_device();

 if (_graphics->windowed)
 {
 // Restore the window location/size
 ::SetWindowPlacement(_wnd->get_handle(), &_wp);
 }

 // Make the window visible
 if (!::IsWindowVisible(_hwnd))
 {
 ::ShowWindow(_hwnd, SW_SHOW);
 }*/
}

}
