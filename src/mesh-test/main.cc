#include <iostream>

#include <boost/program_options.hpp>

#include <framework/bitmap.h>
#include <framework/camera.h>
#include <framework/settings.h>
#include <framework/framework.h>
#include <framework/graphics.h>
#include <framework/shader.h>
#include <framework/font.h>
#include <framework/gui/gui.h>
#include <framework/gui/builder.h>
#include <framework/gui/button.h>
#include <framework/gui/window.h>
#include <framework/logging.h>
#include <framework/texture.h>
#include <framework/model_manager.h>
#include <framework/model.h>
#include <framework/misc.h>
#include <framework/paths.h>
#include <framework/scenegraph.h>
#include <framework/vector.h>

namespace po = boost::program_options;

void settings_initialize(int argc, char** argv);
void display_exception(std::string const &msg);
void initialize_ground(std::shared_ptr<fw::sg::node> node);

class application: public fw::base_app {
public:
  bool initialize(fw::framework *frmwrk);
  void update(float dt);
  void render(fw::sg::scenegraph &scenegraph);
};

static std::shared_ptr<fw::model> g_model;
static bool g_show_ground = false;
static std::shared_ptr<fw::sg::node> g_ground;
static bool g_rotating = false;
static float g_rotate_angle = 0.0f;

bool restart_handler(fw::gui::widget *wdgt) {
  fw::settings stg;
  g_model = fw::framework::get_instance()->get_model_manager()->get_model(stg.get_value<std::string>("mesh-file"));
  return true;
}

bool ground_handler(fw::gui::widget *wdgt) {
  if (g_show_ground) {
    g_show_ground = false;
    dynamic_cast<fw::gui::button *>(wdgt)->set_text("Show ground");
  } else {
    g_show_ground = true;
    dynamic_cast<fw::gui::button *>(wdgt)->set_text("Hide ground");
  }
  return true;
}

bool rotate_handler(fw::gui::widget *wdgt) {
  if (g_rotating) {
    g_rotating = false;
    dynamic_cast<fw::gui::button *>(wdgt)->set_text("Rotate");
  } else {
    g_rotating = true;
    dynamic_cast<fw::gui::button *>(wdgt)->set_text("Stop rotating");
  }
  return true;
}

bool application::initialize(fw::framework *frmwrk) {
  fw::top_down_camera *cam = new fw::top_down_camera();
  cam->set_mouse_move(false);
  frmwrk->set_camera(cam);

  fw::gui::window *wnd;
  wnd = fw::gui::builder<fw::gui::window>(fw::gui::px(20), fw::gui::px(20), fw::gui::px(150), fw::gui::px(130))
      << fw::gui::window::background("frame")
      << (fw::gui::builder<fw::gui::button>(fw::gui::px(10), fw::gui::px(10), fw::gui::px(130), fw::gui::px(30))
          << fw::gui::button::text("Restart")
          << fw::gui::widget::click(std::bind<bool>(restart_handler, std::placeholders::_1)))
      << (fw::gui::builder<fw::gui::button>(fw::gui::px(10), fw::gui::px(50), fw::gui::px(130), fw::gui::px(30))
          << fw::gui::button::text("Show ground")
          << fw::gui::widget::click(std::bind<bool>(ground_handler, std::placeholders::_1)))
      << (fw::gui::builder<fw::gui::button>(fw::gui::px(10), fw::gui::px(90), fw::gui::px(130), fw::gui::px(30))
          << fw::gui::button::text("Rotate")
          << fw::gui::widget::click(std::bind<bool>(rotate_handler, std::placeholders::_1)));
  frmwrk->get_gui()->attach_widget(wnd);

  fw::settings stg;
  g_model = frmwrk->get_model_manager()->get_model(stg.get_value<std::string>("mesh-file"));
  g_ground = std::shared_ptr<fw::sg::node>(new fw::sg::node());
  initialize_ground(g_ground);
  return true;
}

void application::update(float dt) {
  if (g_rotating) {
    g_rotate_angle += 3.14159f * dt;
  }
}

void application::render(fw::sg::scenegraph &scenegraph) {
  scenegraph.set_clear_colour(fw::colour(1, 0, 1, 0));

  // set up the properties of the sun that we'll use to light and also cast shadows
  fw::vector sun(0.485f, 0.485f, 0.727f);
  fw::vector lookat(0, 0, 0);
  std::shared_ptr <fw::sg::light> light(new fw::sg::light(lookat + sun * 300.0f, sun * -1, true));
  scenegraph.add_light(light);

  if (g_show_ground) {
    scenegraph.add_node(g_ground);
  }

  g_model->set_colour(fw::colour(1.0f, 1.0f, 0.0f, 0.0f));
  g_model->render(scenegraph, fw::rotate_axis_angle(fw::vector(0, 1, 0), g_rotate_angle));
}

//-----------------------------------------------------------------------------

void initialize_ground(std::shared_ptr<fw::sg::node> node) {
  fw::vertex::xyz_n_uv vertices[] = {
      fw::vertex::xyz_n_uv(-100.0f, 0.0f, -100.0f,      0.0f, 1.0f, 0.0f,       0.0f,  0.0f),
      fw::vertex::xyz_n_uv(-100.0f, 0.0f,  100.0f,      0.0f, 1.0f, 0.0f,       0.0f,  8.0f),
      fw::vertex::xyz_n_uv( 100.0f, 0.0f,  100.0f,      0.0f, 1.0f, 0.0f,       8.0f,  8.0f),
      fw::vertex::xyz_n_uv( 100.0f, 0.0f, -100.0f,      0.0f, 1.0f, 0.0f,       8.0f,  0.0f)
    };
  uint16_t indices[] = { 0, 1, 2, 0, 2, 3 };

  std::shared_ptr<fw::vertex_buffer> vb = fw::vertex_buffer::create<fw::vertex::xyz_n_uv>();
  vb->set_data(4, vertices);

  std::shared_ptr<fw::index_buffer> ib(new fw::index_buffer());
  ib->set_data(6, indices);

  std::shared_ptr<fw::shader> shader = fw::shader::create("basic.shader");

  std::shared_ptr<fw::texture> texture(new fw::texture());
  texture->create(fw::resolve("terrain/grass-01.jpg"));

  std::shared_ptr<fw::shader_parameters> params = shader->create_parameters();
  params->set_texture("tex_sampler", texture);

  node->set_vertex_buffer(vb);
  node->set_index_buffer(ib);
  node->set_shader(shader);
  node->set_shader_parameters(params);
  node->set_cast_shadows(false);
  node->set_primitive_type(fw::sg::primitive_trianglelist);
}

//-----------------------------------------------------------------------------

int main(int argc, char** argv) {
  try {
    settings_initialize(argc, argv);

    application app;
    new fw::framework(&app);
    fw::framework::get_instance()->initialize("Mesh Test");
    fw::framework::get_instance()->run();
  } catch (std::exception &e) {
    std::string msg = boost::diagnostic_information(e);
    fw::debug << "--------------------------------------------------------------------------------" << std::endl;
    fw::debug << "UNHANDLED EXCEPTION!" << std::endl;
    fw::debug << msg << std::endl;

    display_exception(e.what());
  } catch (...) {
    fw::debug << "--------------------------------------------------------------------------------" << std::endl;
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

void settings_initialize(int argc, char** argv) {
  po::options_description options("Additional options");
  options.add_options()("mesh-file",
      po::value<std::string>()->default_value("factory"),
      "Name of the mesh file to load, we assume it can be fw::resolve'd.");

  fw::settings::initialize(options, argc, argv, "font-test.conf");
}
