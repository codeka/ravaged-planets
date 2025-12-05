#include <iostream>

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
#include <framework/math.h>
#include <framework/model_manager.h>
#include <framework/model.h>
#include <framework/model_node.h>
#include <framework/misc.h>
#include <framework/paths.h>
#include <framework/scenegraph.h>
#include <framework/status.h>

fw::Status settings_initialize(int argc, char** argv);
void display_exception(std::string const &msg);
void initialize_ground(std::shared_ptr<fw::sg::Node> Node);

class Application: public fw::BaseApp {
public:
  fw::Status initialize(fw::Framework *frmwrk);
  void update(float dt);
};

static std::shared_ptr<fw::Model> g_model;
static std::shared_ptr<fw::ModelNode> g_model_node;
static bool g_show_ground = false;
static std::shared_ptr<fw::sg::Node> g_ground;
static bool g_rotating = false;
static float g_rotate_angle = 0.0f;

bool restart_handler(fw::gui::Widget *wdgt) {
  auto model = fw::Framework::get_instance()->get_model_manager()->get_model(
      fw::Settings::get<std::string>("mesh-file"));
  if (!model.ok()) {
    LOG(ERR) << "error loading model: " << model.status();
  } else {
    g_model = *model;
  }
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

  fw::Framework::get_instance()->get_scenegraph_manager()->enqueue(
    [](fw::sg::Scenegraph& sg) {
      g_ground->set_enabled(g_show_ground);
    });

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

fw::Status Application::initialize(fw::Framework *frmwrk) {
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

  fw::Framework::get_instance()->get_scenegraph_manager()->enqueue(
    [frmwrk](fw::sg::Scenegraph& scenegraph) {
      scenegraph.set_clear_color(fw::Color(1, 0, 0, 0));

      // set up the properties of the sun that we'll use to Light and also cast shadows
      fw::Vector sun(0.485f, 0.485f, 0.727f);
      fw::Vector lookat(0, 0, 0);
      auto light = std::make_shared<fw::sg::Light>(lookat + sun * 300.0f, sun * -1, true);
      scenegraph.add_light(light);

      auto model =
          frmwrk->get_model_manager()->get_model(fw::Settings::get<std::string>("mesh-file"));
      if (!model.ok()) {
        LOG(ERR) << "error loading model: " << model.status();
      } else {
        g_model = *model;
        g_model_node = g_model->create_node(fw::Color::from_rgba(0xff0000ff));
        scenegraph.add_node(g_model_node);
      }
      g_ground = std::shared_ptr<fw::sg::Node>(new fw::sg::Node());
      g_ground->set_enabled(false);
      scenegraph.add_node(g_ground);
      initialize_ground(g_ground);
    });
  return fw::OkStatus();
}

void Application::update(float dt) {
  if (g_rotating) {
    g_rotate_angle += 3.14159f * dt;

    fw::Framework::get_instance()->get_scenegraph_manager()->enqueue(
      [](fw::sg::Scenegraph& sg) {
        g_model_node->set_world_matrix(fw::rotate_axis_angle(fw::Vector(0, 1, 0), g_rotate_angle).to_matrix());
      });
  }
}

//-----------------------------------------------------------------------------

void initialize_ground(std::shared_ptr<fw::sg::Node> node) {
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

  auto shader = fw::Shader::CreateOrEmpty("basic.shader");
  std::shared_ptr<fw::Texture> texture(new fw::Texture());
  texture->create(fw::resolve("terrain/grass-01.jpg"));

  auto params = shader->CreateParameters();
  params->set_texture("tex_sampler", texture);

  node->set_vertex_buffer(vb);
  node->set_index_buffer(ib);
  node->set_shader(shader);
  node->set_shader_parameters(params);
  node->set_cast_shadows(false);
  node->set_primitive_type(fw::sg::PrimitiveType::kTriangleList);
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
    auto continue_or_status = fw::Framework::get_instance()->initialize("Mesh Test");
    if (!continue_or_status.ok()) {
      LOG(ERR) << continue_or_status.status();
      return 1;
    }
    if (!continue_or_status.value()) {
      return 0;
    }

    fw::Framework::get_instance()->run();
  } catch (std::exception &e) {
    LOG(ERR) << "--------------------------------------------------------------------------------";
    LOG(ERR) << "UNHANDLED EXCEPTION!";
    LOG(ERR) << e.what();

    display_exception(e.what());
  } catch (...) {
    LOG(ERR) << "--------------------------------------------------------------------------------";
    LOG(ERR) << "UNHANDLED EXCEPTION! (unknown exception)";
  }

  return 0;
}

void display_exception(std::string const &msg) {
  std::stringstream ss;
  ss << "An error has occurred. Please send your log file (below) to dean@codeka.com.au for diagnostics." << std::endl;
  ss << std::endl;
  ss << fw::LogFileName() << std::endl;
  ss << std::endl;
  ss << msg;
}

fw::Status settings_initialize(int argc, char** argv) {
  fw::SettingDefinition extra_settings;
  extra_settings.add_group("Additional options", "Mesh-test specific settings")
      .add_setting<std::string>(
          "mesh-file",
          "Name of the mesh file to load, we assume it can be fw::resolve'd.",
          "tank-tracks");

  return fw::Settings::initialize(extra_settings, argc, argv, "font-test.conf");
}
