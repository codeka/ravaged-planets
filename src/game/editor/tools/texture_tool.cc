#include <framework/framework.h>
#include <framework/graphics.h>
#include <framework/bitmap.h>
#include <framework/texture.h>
#include <framework/input.h>
#include <framework/timer.h>
#include <framework/scenegraph.h>
#include <framework/gui/gui.h>
#include <framework/gui/builder.h>
#include <framework/gui/slider.h>
#include <framework/gui/window.h>
#include <framework/gui/button.h>

#include <game/editor/editor_terrain.h>
#include <game/editor/editor_world.h>
#include <game/editor/tools/texture_tool.h>

using namespace fw::gui;
using namespace std::placeholders;

//-----------------------------------------------------------------------------
class texture_tool_window {
private:
  ed::texture_tool *_tool;
  window *_wnd;

  bool radius_slider_value_changed(fw::gui::widget *widget);
  void refresh_texture_icon(fw::gui::window *wnd, int layer_num);

public:
  texture_tool_window(ed::texture_tool *tool);
  virtual ~texture_tool_window();

  // get the index of the layer we're currently painting (0 through 3)
  int get_paint_layer();

  void show();
  void hide();
};

texture_tool_window::texture_tool_window(ed::texture_tool *tool) : _tool(tool) {
  _wnd = builder<window>(px(10), px(30), px(80), px(100)) << window::background("frame")
      << builder<slider>(px(4), px(4), sum(pct(100), px(-8)), px(18));
  fw::framework::get_instance()->get_gui()->attach_widget(_wnd);
}

texture_tool_window::~texture_tool_window() {
  fw::framework::get_instance()->get_gui()->detach_widget(_wnd);
}

void texture_tool_window::show() {
  _wnd->set_visible(true);
/*
  _radius_slider->setCurrentValue((float) (_tool->get_radius() - 1) / ed::texture_tool::max_radius);

  _layer1_image = get_child("ToolTexture/Layer1Image");
  _layer1_paint = get_child < CEGUI::RadioButton > ("ToolTexture/Layer1Select");
  _layer1_paint->setSelected(true);
  refresh_texture_icon(_layer1_image, 0);
  _layer2_image = get_child("ToolTexture/Layer2Image");
  _layer2_paint = get_child < CEGUI::RadioButton > ("ToolTexture/Layer2Select");
  refresh_texture_icon(_layer2_image, 1);
  _layer3_image = get_child("ToolTexture/Layer3Image");
  _layer3_paint = get_child < CEGUI::RadioButton > ("ToolTexture/Layer3Select");
  refresh_texture_icon(_layer3_image, 2);
  _layer4_image = get_child("ToolTexture/Layer4Image");
  _layer4_paint = get_child < CEGUI::RadioButton > ("ToolTexture/Layer4Select");
  refresh_texture_icon(_layer4_image, 3);
*/
}

void texture_tool_window::hide() {
  _wnd->set_visible(false);
}

int texture_tool_window::get_paint_layer() {
/*  if (_layer1_paint->isSelected())
    return 0;
  if (_layer2_paint->isSelected())
    return 1;
  if (_layer3_paint->isSelected())
    return 2;
  if (_layer4_paint->isSelected())
    return 3;

  // huh?*/
  return 0;
}

bool texture_tool_window::radius_slider_value_changed(fw::gui::widget *widget) {
//  int radius = (int) (_radius_slider->getCurrentValue() * ed::texture_tool::max_radius);
//  _tool->set_radius(radius + 1);
  return true;
}

// refreshes the icon of the given window which represents the given
// terrain texture layer
void texture_tool_window::refresh_texture_icon(fw::gui::window *wnd, int layer_num) {
}

namespace ed {
REGISTER_TOOL("texture", texture_tool);

float texture_tool::max_radius = 10;

texture_tool::texture_tool(editor_world *wrld) :
    tool(wrld), _radius(4), _is_painting(false) {
  _wnd = new texture_tool_window(this);
}

texture_tool::~texture_tool() {
  delete _wnd;
}

void texture_tool::activate() {
  tool::activate();

  fw::input *inp = fw::framework::get_instance()->get_input();
  _keybind_tokens.push_back(
      inp->bind_key("Left-Mouse", fw::input_binding(std::bind(&texture_tool::on_key, this, _1, _2))));

  _wnd->show();
}

void texture_tool::deactivate() {
  tool::deactivate();

  _wnd->hide();
}

void texture_tool::update() {
  tool::update();

  if (_is_painting) {
    fw::vector cursor_loc = _terrain->get_cursor_location();

    // patch_x and patch_z are the patch number(s) we're inside of
    int patch_x = static_cast<int>(cursor_loc[0] / game::terrain::PATCH_SIZE);
    int patch_z = static_cast<int>(cursor_loc[2] / game::terrain::PATCH_SIZE);

    // get the splatt texture at the current cursor location
    fw::bitmap &splatt = _terrain->get_splatt(patch_x, patch_z);

    // scale_x and scale_y represent the number of pixels in the splatt texture
    // per game unit of the terrain
    float scale_x = static_cast<float>(splatt.get_width()) / game::terrain::PATCH_SIZE;
    float scale_y = static_cast<float>(splatt.get_height()) / game::terrain::PATCH_SIZE;

    // centre_u and centre_v are the texture coordinates (in the range [0..1])
    // of what the cursor is currently pointing at
    float centre_u = (cursor_loc[0] - (patch_x * game::terrain::PATCH_SIZE))
        / static_cast<float>(game::terrain::PATCH_SIZE);
    float centre_v = (cursor_loc[2] - (patch_z * game::terrain::PATCH_SIZE))
        / static_cast<float>(game::terrain::PATCH_SIZE);

    // cetre_x and centre_y are the (x,y) corrdinates (in texture space)
    // of the splatt texture where the cursor is currently pointing.
    int centre_x = static_cast<int>(centre_u * splatt.get_width());
    int centre_y = static_cast<int>(centre_v * splatt.get_height());

    fw::vector centre(static_cast<float>(centre_x), static_cast<float>(centre_y), 0.0f);

    // we have to take a copy of the splatt's pixels cause we'll be modifying them
    std::vector<uint32_t> data = splatt.get_pixels();

    uint32_t new_value = get_selected_splatt_mask();
    for (int y = centre_y - static_cast<int>(_radius * scale_y); y <= centre_y + static_cast<int>(_radius * scale_y);
        y++) {
      for (int x = centre_x - static_cast<int>(_radius * scale_x); x <= centre_x + static_cast<int>(_radius * scale_x);
          x++) {
        if (y < 0 || x < 0 || y >= splatt.get_height() || x >= splatt.get_width())
          continue;

        fw::vector v(static_cast<float>(x), static_cast<float>(y), 0.0f);
        if ((v - centre).length() > (_radius * scale_x))
          continue;

        data[(y * splatt.get_width()) + x] = new_value;
      }
    }

    splatt.set_pixels(data);
    fw::framework::get_instance()->get_graphics()->run_on_render_thread([=]() {
      _terrain->set_splatt(patch_x, patch_z, splatt);
    });
  }
}

void texture_tool::render(fw::sg::scenegraph &scenegraph) {
  draw_circle(scenegraph, _terrain, _terrain->get_cursor_location(), (float) _radius);
}

void texture_tool::on_key(std::string keyname, bool is_down) {
  if (keyname == "Left-Mouse") {
    _is_painting = is_down;
  }
}

uint32_t texture_tool::get_selected_splatt_mask() {
  int layer = _wnd->get_paint_layer();
  switch (layer) {
  case 0:
    return 0x00FF0000;
  case 1:
    return 0x0000FF00;
  case 2:
    return 0x000000FF;
  case 3:
    return 0xFF000000;
  default:
    return 0x00000000;
  }
}

}
