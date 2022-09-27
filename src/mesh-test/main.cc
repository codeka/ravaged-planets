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
void initialize_ground(std::shared_ptr<fw::sg::Node> Node);

class Application: public fw::BaseApp {
public:
  bool initialize(fw::Framework *frmwrk);
  void update(float dt);
  void render(fw::sg::Scenegraph &Scenegraph);
};

static std::shared_ptr<fw::Model> g_model;
static bool g_show_ground = false;
static std::shared_ptr<fw::sg::Node> g_ground;
static bool g_rotating = false;
static float g_rotate_angle = 0.0f;

bool restart_handler(fw::gui::Widget *wdgt) {
  fw::Settings stg;
  g_model = fw::Framework::get_instance()->get_model_manager()->get_model(stg.get_value<std::string>("mesh-file"));
  return true;
}

bool ground_handler(fw::gui::Widget *wdgt) {
  if (g_show_ground) {
    g_show_ground = false;
    dynamic_cast<fw::gui::Button *>(wdgt)->set_text("Show ground");
  } else {
    g_show_ground = true;
    dynamic_cast<fw::gui::Button *>(wdgt)->set_text("Hide ground");
  }
  return true;
}

bool rotate_handler(fw::gui::Widget *wdgt) {
  if (g_rotating) {
    g_rotating = false;
    dynamic_cast<fw::gui::Button *>(wdgt)->set_text("Rotate");
  } else {
    g_rotating = true;
    dynamic_cast<fw::gui::Button *>(wdgt)->set_text("Stop rotating");
  }
  return true;
}

bool Application::initialize(fw::Framework *frmwrk) {
  fw::TopDownCamera *cam = new fw::TopDownCamera();
  cam->set_mouse_move(false);
  frmwrk->set_camera(cam);

  fw::gui::Window *wnd;
  wnd = fw::gui::Builder<fw::gui::Window>(fw::gui::px(20), fw::gui::px(20), fw::gui::px(150), fw::gui::px(130))
      << fw::gui::Window::background("frame")
      << (fw::gui::Builder<fw::gui::Button>(fw::gui::px(10), fw::gui::px(10), fw::gui::px(130), fw::gui::px(30))
          << fw::gui::Button::text("Restart")
          << fw::gui::Widget::click(std::bind<bool>(restart_handler, std::placeholders::_1)))
      << (fw::gui::Builder<fw::gui::Button>(fw::gui::px(10), fw::gui::px(50), fw::gui::px(130), fw::gui::px(30))
          << fw::gui::Button::text("Show ground")
          << fw::gui::Widget::click(std::bind<bool>(ground_handler, std::placeholders::_1)))
      << (fw::gui::Builder<fw::gui::Button>(fw::gui::px(10), fw::gui::px(90), fw::gui::px(130), fw::gui::px(30))
          << fw::gui::Button::text("Rotate")
          << fw::gui::Widget::click(std::bind<bool>(rotate_handler, std::placeholders::_1)));
  frmwrk->get_gui()->attach_widget(wnd);

  fw::Settings stg;
  g_model = frmwrk->get_model_manager()->get_model(stg.get_value<std::string>("mesh-file"));
  g_ground = std::shared_ptr<fw::sg::Node>(new fw::sg::Node());
  initialize_ground(g_ground);
  return true;
}

void Application::update(float dt) {
  if (g_rotating) {
    g_rotate_angle += 3.14159f * dt;
  }
}

void Application::render(fw::sg::Scenegraph &Scenegraph) {
  Scenegraph.set_clear_color(fw::Color(1, 0, 1, 0));

  // set up the properties of the sun that we'll use to Light and also cast shadows
  fw::Vector sun(0.485f, 0.485f, 0.727f);
  fw::Vector lookat(0, 0, 0);
  std::shared_ptr <fw::sg::Light> Light(new fw::sg::Light(lookat + sun * 300.0f, sun * -1, true));
  Scenegraph.add_light(Light);

  if (g_show_ground) {
    Scenegraph.add_node(g_ground);
  }

  g_model->set_color(fw::Color(1.0f, 1.0f, 0.0f, 0.0f));
  g_model->render(Scenegraph, fw::rotate_axis_angle(fw::Vector(0, 1, 0), g_rotate_angle));
}

//-----------------------------------------------------------------------------

void initialize_ground(std::shared_ptr<fw::sg::Node> Node) {
  fw::vertex::xyz_n_uv vertices[] = {
      fw::vertex::xyz_n_uv(-100.0f, 0.0f, -100.0f,      0.0f, 1.0f, 0.0f,       0.0f,  0.0f),
      fw::vertex::xyz_n_uv(-100.0f, 0.0f,  100.0f,      0.0f, 1.0f, 0.0f,       0.0f,  8.0f),
      fw::vertex::xyz_n_uv( 100.0f, 0.0f,  100.0f,      0.0f, 1.0f, 0.0f,       8.0f,  8.0f),
      fw::vertex::xyz_n_uv( 100.0f, 0.0f, -100.0f,      0.0f, 1.0f, 0.0f,       8.0f,  0.0f)
    };
  uint16_t indices[] = { 0, 1, 2, 0, 2, 3 };

  std::shared_ptr<fw::VertexBuffer> vb = fw::VertexBuffer::create<fw::vertex::xyz_n_uv>();
  vb->set_data(4, vertices);

  std::shared_ptr<fw::IndexBuffer> ib(new fw::IndexBuffer());
  ib->set_data(6, indices);

  std::shared_ptr<fw::Shader> Shader = fw::Shader::create("basic.shader");

  std::shared_ptr<fw::Texture> texture(new fw::Texture());
  texture->create(fw::resolve("terrain/grass-01.jpg"));

  std::shared_ptr<fw::ShaderParameters> params = Shader->create_parameters();
  params->set_texture("tex_sampler", texture);

  Node->set_vertex_buffer(vb);
  Node->set_index_buffer(ib);
  Node->set_shader(Shader);
  Node->set_shader_parameters(params);
  Node->set_cast_shadows(false);
  Node->set_primitive_type(fw::sg::PrimitiveType::kTriangleList);
}

//-----------------------------------------------------------------------------

int main(int argc, char** argv) {
  try {
    settings_initialize(argc, argv);

    Application app;
    new fw::Framework(&app);
    fw::Framework::get_instance()->initialize("Mesh Test");
    fw::Framework::get_instance()->run();
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
      po::value<std::string>()->default_value("tank-tracks"),
      "Name of the mesh file to load, we assume it can be fw::resolve'd.");

  fw::Settings::initialize(options, argc, argv, "font-test.conf");
}
