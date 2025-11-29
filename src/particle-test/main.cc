#include <iostream>

#include <framework/bitmap.h>
#include <framework/camera.h>
#include <framework/settings.h>
#include <framework/framework.h>
#include <framework/graphics.h>
#include <framework/font.h>
#include <framework/gui/gui.h>
#include <framework/gui/builder.h>
#include <framework/gui/button.h>
#include <framework/gui/window.h>
#include <framework/logging.h>
#include <framework/particle_manager.h>
#include <framework/particle_effect.h>
#include <framework/math.h>
#include <framework/misc.h>
#include <framework/paths.h>
#include <framework/scenegraph.h>

fw::Status settings_initialize(int argc, char** argv);
void display_exception(std::string const &msg);

static std::shared_ptr<fw::ParticleEffect> g_effect;
static bool is_moving = false;
static float angle;

class Application: public fw::BaseApp {
public:
  bool initialize(fw::Framework *frmwrk);
  void update(float dt);
};

void update_effect_position() {
  fw::Vector pos;
  if (is_moving) {
    fw::Quaternion q = fw::rotate_axis_angle(fw::Vector(0, 1, 0), angle);
    auto res = q * fw::Vector4(10.0f, 0, 0, 1);
    pos = fw::Vector(res[0], res[1], res[2]);
  } else {
    pos = fw::Vector(0.0f, 0.0f, 0.0f);
  }

  fw::Framework::get_instance()->get_scenegraph_manager()->enqueue(
    [pos](fw::sg::Scenegraph& scenegraph) {
      g_effect->set_position(pos);
    });
}

void restart_effect() {
  fw::Framework::get_instance()->get_scenegraph_manager()->enqueue(
    [](fw::sg::Scenegraph& scenegraph) {
      if (g_effect) {
        g_effect->destroy();
      }
      g_effect = fw::Framework::get_instance()->get_particle_mgr()->create_effect(
          fw::Settings::get<std::string>("particle-file"), fw::Vector(0, 0, 0));
    });
  update_effect_position();
}

bool restart_handler(fw::gui::Widget *wdgt) {
  restart_effect();
  return true;
}

bool pause_handler(fw::gui::Widget *wdgt) {
  fw::gui::Button *btn = dynamic_cast<fw::gui::Button *>(wdgt);
  fw::Framework *framework = fw::Framework::get_instance();
  if (framework->is_paused()) {
    framework->unpause();
    btn->set_text("Pause");
  } else {
    framework->pause();
    btn->set_text("Unpause");
  }

  return true;
}

bool movement_handler(fw::gui::Widget *wdgt) {
  fw::gui::Button *btn = dynamic_cast<fw::gui::Button *>(wdgt);
  if (is_moving) {
    is_moving = false;
    btn->set_text("Stationary");
    update_effect_position();
  } else {
    is_moving = true;
    btn->set_text("Moving");
  }

  return true;
}

bool Application::initialize(fw::Framework *frmwrk) {
  fw::TopDownCamera *cam = new fw::TopDownCamera();
  cam->set_mouse_move(false);
  frmwrk->set_camera(cam);

  fw::gui::Window *wnd;
  wnd = fw::gui::Builder<fw::gui::Window>()
      << fw::gui::Widget::position(fw::gui::px(20), fw::gui::px(20))
      << fw::gui::Widget::size(fw::gui::px(150), fw::gui::px(130))
      << fw::gui::Window::background("frame")
      << (fw::gui::Builder<fw::gui::Button>()
          << fw::gui::Widget::position(fw::gui::px(10), fw::gui::px(10))
          << fw::gui::Widget::size(fw::gui::px(130), fw::gui::px(30))
          << fw::gui::Button::text("Restart")
          << fw::gui::Widget::click(std::bind<bool>(restart_handler, std::placeholders::_1)))
      << (fw::gui::Builder<fw::gui::Button>()
          << fw::gui::Widget::position(fw::gui::px(10), fw::gui::px(50))
          << fw::gui::Widget::size(fw::gui::px(130), fw::gui::px(30))
          << fw::gui::Button::text("Pause")
          << fw::gui::Widget::click(std::bind<bool>(pause_handler, std::placeholders::_1)))
      << (fw::gui::Builder<fw::gui::Button>()
          << fw::gui::Widget::position(fw::gui::px(10), fw::gui::px(90))
          << fw::gui::Widget::size(fw::gui::px(130), fw::gui::px(30))
          << fw::gui::Button::text("Stationary")
          << fw::gui::Widget::click(std::bind<bool>(movement_handler, std::placeholders::_1)));
  frmwrk->get_gui()->attach_widget(wnd);

  restart_effect();
  return true;
}

void Application::update(float dt) {
  if (is_moving) {
    angle += 3.1415f * dt;
    update_effect_position();
  }
}

//-----------------------------------------------------------------------------

int main(int argc, char** argv) {
  try {
    auto status = settings_initialize(argc, argv);
    if (!status.ok()) {
      std::cerr << status << std::endl;
      fw::Settings::print_help();
      return 1;
    }

    Application app;
    new fw::Framework(&app);
    fw::Framework::get_instance()->initialize("Particle Test");
    fw::Framework::get_instance()->run();
  } catch (std::exception &e) {
    std::string msg = boost::diagnostic_information(e);
    fw::debug
        << "--------------------------------------------------------------------------------"
        << std::endl;
    fw::debug << "UNHANDLED EXCEPTION!" << std::endl;
    fw::debug << msg << std::endl;

    display_exception(e.what());
  } catch (...) {
    fw::debug
        << "--------------------------------------------------------------------------------"
        << std::endl;
    fw::debug << "UNHANDLED EXCEPTION! (unknown exception)" << std::endl;
  }

  return 0;
}

void display_exception(std::string const &msg) {
  std::stringstream ss;
  ss << "An error has occurred. Please send your log file (below) to dean@codeka.com.au for diagnostics." << std::endl;
  ss << std::endl;
  ss << fw::debug.get_filename() << std::endl;
  ss << std::endl;
  ss << msg;
}

fw::Status settings_initialize(int argc, char** argv) {
  fw::SettingDefinition extra_settings;
  extra_settings.add_group("Additional options", "Particle-test specific settings")
      .add_setting<std::string>(
          "particle-file",
          "Name of the particle file to load, we assume it can be fw::resolve'd.",
          "explosion-01");

  return fw::Settings::initialize(extra_settings, argc, argv, "font-test.conf");
}
