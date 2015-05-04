
#include <memory>

#include <framework/framework.h>
#include <framework/graphics.h>
#include <framework/timer.h>
#include <framework/input.h>
#include <framework/camera.h>
#include <framework/logging.h>
#include <framework/index_buffer.h>
#include <framework/vertex_buffer.h>
#include <framework/vertex_formats.h>

#include <game/application.h>
#include <game/world/terrain.h>
//#include <warworlds/screens/screen.h>
//#include <warworlds/session/session.h>
//#include <warworlds/simulation/simulation_thread.h>

namespace rp {

application::application()
  : _framework(nullptr), _screen(nullptr) {
}

application::~application() {
}

ww::terrain *terrain = nullptr;
std::shared_ptr<fw::index_buffer> idx_buffer;
std::shared_ptr<fw::vertex_buffer> vtx_buffer;

bool application::initialize(fw::framework *frmwrk) {
  _framework = frmwrk;
//  _framework->get_input()->show_cursor();

  // set up the camera
  fw::top_down_camera *cam = new fw::top_down_camera();
  cam->set_mouse_move(false);
  _framework->set_camera(cam);

 // terrain = new ww::terrain();
//  terrain->create(64, 64, 1);
 // terrain->initialize();

  vtx_buffer = std::shared_ptr<fw::vertex_buffer>(new fw::vertex_buffer());
  vtx_buffer->create_buffer<fw::vertex::xyz_n>(4, false);
  fw::vertex::xyz_n data[4];
  data[0] = fw::vertex::xyz_n(-1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
  data[1] = fw::vertex::xyz_n( 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
  data[2] = fw::vertex::xyz_n( 1.0f, 1.0f,  0.0f, 0.0f, 1.0f, 0.0f);
  data[3] = fw::vertex::xyz_n(-1.0f, 1.0f,  0.0f, 0.0f, 1.0f, 0.0f);
  vtx_buffer->set_data(4, reinterpret_cast<void *>(&data));

  idx_buffer = std::shared_ptr<fw::index_buffer>(new fw::index_buffer());
  idx_buffer->create_buffer(6, false);
  unsigned short indices[6];
  indices[0] = 0;
  indices[1] = 1;
  indices[2] = 2;
  indices[3] = 0;
  indices[4] = 3;
  indices[5] = 2;
  idx_buffer->set_data(6, indices);

  // start the simulation thread now, it'll always run even if there's
  // no actual game running....
//  simulation_thread::get_instance()->initialise();
  /*
   // attach our GUI sounds to the various events
   fw::audio_manager *audio_mgr = _framework->get_audio();
   fw::gui::system_sounds *sounds = _framework->get_gui()->get_system_sounds();
   sounds->attach_sound(fw::gui::system_sounds::button_hover,
   audio_mgr->load_file("ui/button-hover.ogg"));
   sounds->attach_sound(fw::gui::system_sounds::button_click,
   audio_mgr->load_file("ui/button-click.ogg"));
   */
//  _screen = new screen_stack();
//  _screen->set_active_screen("title");

  return true;
}

void application::destroy() {
  fw::debug << "application shutting down." << std::endl;
//  simulation_thread::get_instance()->destroy();
}

void application::update(float dt) {
//  terrain->update();
//  _screen->get_active_screen()->update(dt);
//  session::get_instance()->update(dt);
}

void application::render(fw::sg::scenegraph &scenegraph) {
 // terrain->render(scenegraph);
//  _screen->get_active_screen()->render(scenegraph);
  std::shared_ptr<fw::sg::node> node(new fw::sg::node());
  node->set_index_buffer(idx_buffer);
  node->set_vertex_buffer(vtx_buffer);
  node->set_primitive_type(fw::sg::primitive_trianglelist);
  scenegraph.add_node(node);
}

}
