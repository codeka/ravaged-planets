#include <functional>
#include <boost/filesystem.hpp>

#include <framework/framework.h>
#include <framework/graphics.h>
#include <framework/bitmap.h>
#include <framework/texture.h>
#include <framework/input.h>
#include <framework/timer.h>
#include <framework/scenegraph.h>
#include <framework/gui/gui.h>
#include <framework/gui/builder.h>
#include <framework/gui/button.h>
#include <framework/gui/drawable.h>
#include <framework/gui/slider.h>
#include <framework/gui/label.h>
#include <framework/gui/listbox.h>
#include <framework/gui/window.h>

#include <game/editor/editor_terrain.h>
#include <game/editor/editor_world.h>
#include <game/editor/tools/texture_tool.h>

namespace fs = boost::filesystem;
using namespace fw::gui;
using namespace std::placeholders;

enum IDS {
  TEXTURES_ID = 1,
  TEXTURE_PREVIEW_ID,
};

//-----------------------------------------------------------------------------
class texture_tool_window {
private:
  ed::texture_tool *_tool;
  Window *_wnd;

  void on_radius_updated(int new_radius);
  void on_texture_selected(int index);
  void refresh_texture_icon(fw::gui::Window *wnd, int layer_num);

public:
  texture_tool_window(ed::texture_tool *tool);
  virtual ~texture_tool_window();

  void show();
  void hide();
};

texture_tool_window::texture_tool_window(ed::texture_tool *tool) : _tool(tool) {
  _wnd = Builder<Window>(px(10), px(30), px(100), px(262)) << Window::background("frame")
      << (Builder<Label>(px(4), px(4), sum(pct(100), px(-8)), px(18)) << Label::text("Size:"))
      << (Builder<Slider>(px(4), px(26), sum(pct(100), px(-8)), px(18))
          << Slider::limits(10, 100) << Slider::on_update(std::bind(&texture_tool_window::on_radius_updated, this, _1))
          << Slider::ParticleRotation(40))
      << (Builder<Listbox>(px(4), px(48), sum(pct(100), px(-8)), px(80)) << Widget::id(TEXTURES_ID)
          << Listbox::item_selected(std::bind(&texture_tool_window::on_texture_selected, this, _1)))
      << (Builder<Label>(px(4), px(132), sum(pct(100), px(-8)), px(92)) << Widget::id(TEXTURE_PREVIEW_ID))
      << (Builder<Button>(px(4), px(228), sum(pct(100), px(-8)), px(30)) << Button::text("Change"));
  fw::Framework::get_instance()->get_gui()->attach_widget(_wnd);
}

texture_tool_window::~texture_tool_window() {
  fw::Framework::get_instance()->get_gui()->detach_widget(_wnd);
}

void texture_tool_window::show() {
  _wnd->set_visible(true);

  Listbox *lbx = _wnd->find<Listbox>(TEXTURES_ID);
  lbx->clear();
  for (int i = 0; i < _tool->get_terrain()->get_num_layers(); i++) {
    fs::path filename = _tool->get_terrain()->get_layer(i)->get_filename();
    lbx->add_item(Builder<Label>(px(0), px(0), pct(100), px(18)) << Label::text(filename.stem().string()));
  }
  lbx->select_item(0);
}

void texture_tool_window::hide() {
  _wnd->set_visible(false);
}

void texture_tool_window::on_texture_selected(int index) {
  std::shared_ptr<fw::Texture> layer(new fw::Texture());
  layer->create(_tool->get_terrain()->get_layer(index));

  std::shared_ptr<Drawable> drawable =
      fw::Framework::get_instance()->get_gui()->get_drawable_manager()->build_drawable(
        layer, 0, 0, layer->get_width(), layer->get_height());
  _wnd->find<Label>(TEXTURE_PREVIEW_ID)->set_background(drawable);
  _tool->set_layer(index);
}

void texture_tool_window::on_radius_updated(int ParticleRotation) {
  int radius = ParticleRotation / 10;
  _tool->set_radius(radius);
}

// refreshes the icon of the given window which represents the given
// terrain texture layer
void texture_tool_window::refresh_texture_icon(fw::gui::Window *wnd, int layer_num) {
}

namespace ed {
REGISTER_TOOL("texture", texture_tool);

float texture_tool::max_radius = 10;

texture_tool::texture_tool(editor_world *wrld) :
    tool(wrld), _radius(4), _is_painting(false), _layer(0) {
  _wnd = new texture_tool_window(this);
}

texture_tool::~texture_tool() {
  delete _wnd;
}

void texture_tool::activate() {
  tool::activate();

  fw::Input *inp = fw::Framework::get_instance()->get_input();
  _keybind_tokens.push_back(
      inp->bind_key("Left-Mouse", fw::InputBinding(std::bind(&texture_tool::on_key, this, _1, _2))));

  _wnd->show();
}

void texture_tool::deactivate() {
  tool::deactivate();

  _wnd->hide();
}

void texture_tool::update() {
  tool::update();

  if (_is_painting) {
    fw::Vector cursor_loc = _terrain->get_cursor_location();

    // patch_x and patch_z are the patch number(s) we're inside of
    int patch_x = static_cast<int>(cursor_loc[0] / game::terrain::PATCH_SIZE);
    int patch_z = static_cast<int>(cursor_loc[2] / game::terrain::PATCH_SIZE);

    // get the splatt texture at the current cursor location
    fw::Bitmap &splatt = _terrain->get_splatt(patch_x, patch_z);

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

    fw::Vector center(static_cast<float>(centre_x), static_cast<float>(centre_y), 0.0f);

    // we have to take a copy of the splatt's pixels cause we'll be modifying them
    std::vector<uint32_t> data = splatt.get_pixels();

    uint32_t new_value = get_selected_splatt_mask();
    for (int y = centre_y - static_cast<int>(_radius * scale_y); y <= centre_y + static_cast<int>(_radius * scale_y);
        y++) {
      for (int x = centre_x - static_cast<int>(_radius * scale_x); x <= centre_x + static_cast<int>(_radius * scale_x);
          x++) {
        if (y < 0 || x < 0 || y >= splatt.get_height() || x >= splatt.get_width())
          continue;

        fw::Vector v(static_cast<float>(x), static_cast<float>(y), 0.0f);
        if ((v - center).length() > (_radius * scale_x))
          continue;

        data[(y * splatt.get_width()) + x] = new_value;
      }
    }

    splatt.set_pixels(data);
    fw::Framework::get_instance()->get_graphics()->run_on_render_thread([=]() {
      _terrain->set_splatt(patch_x, patch_z, splatt);
    });
  }
}

void texture_tool::render(fw::sg::Scenegraph &Scenegraph) {
  draw_circle(Scenegraph, _terrain, _terrain->get_cursor_location(), (float) _radius);
}

void texture_tool::on_key(std::string keyname, bool is_down) {
  if (keyname == "Left-Mouse") {
    _is_painting = is_down;
  }
}

uint32_t texture_tool::get_selected_splatt_mask() {
  switch (_layer) {
  case 0:
    return 0x000000FF;
  case 1:
    return 0x0000FF00;
  case 2:
    return 0x00FF0000;
  case 3:
    return 0xFF000000;
  default:
    return 0x00000000;
  }
}

}
