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
class TextureToolWindow {
private:
  ed::TextureTool *tool_;
  Window *wnd_;

  void on_radius_updated(int new_radius);
  void on_texture_selected(int index);
  void refresh_texture_icon(fw::gui::Window *wnd, int layer_num);

public:
  TextureToolWindow(ed::TextureTool *tool);
  virtual ~TextureToolWindow();

  void show();
  void hide();
};

TextureToolWindow::TextureToolWindow(ed::TextureTool *tool) : tool_(tool) {
  wnd_ = Builder<Window>(px(10), px(30), px(100), px(262)) << Window::background("frame")
      << (Builder<Label>(px(4), px(4), sum(pct(100), px(-8)), px(18)) << Label::text("Size:"))
      << (Builder<Slider>(px(4), px(26), sum(pct(100), px(-8)), px(18))
          << Slider::limits(10, 100) << Slider::on_update(std::bind(&TextureToolWindow::on_radius_updated, this, _1))
          << Slider::ParticleRotation(40))
      << (Builder<Listbox>(px(4), px(48), sum(pct(100), px(-8)), px(80)) << Widget::id(TEXTURES_ID)
          << Listbox::item_selected(std::bind(&TextureToolWindow::on_texture_selected, this, _1)))
      << (Builder<Label>(px(4), px(132), sum(pct(100), px(-8)), px(92)) << Widget::id(TEXTURE_PREVIEW_ID))
      << (Builder<Button>(px(4), px(228), sum(pct(100), px(-8)), px(30)) << Button::text("Change"));
  fw::Framework::get_instance()->get_gui()->attach_widget(wnd_);
}

TextureToolWindow::~TextureToolWindow() {
  fw::Framework::get_instance()->get_gui()->detach_widget(wnd_);
}

void TextureToolWindow::show() {
  wnd_->set_visible(true);

  Listbox *lbx = wnd_->find<Listbox>(TEXTURES_ID);
  lbx->clear();
  for (int i = 0; i < tool_->get_terrain()->get_num_layers(); i++) {
    fs::path filename = tool_->get_terrain()->get_layer(i)->get_filename();
    lbx->add_item(Builder<Label>(px(0), px(0), pct(100), px(18)) << Label::text(filename.stem().string()));
  }
  lbx->select_item(0);
}

void TextureToolWindow::hide() {
  wnd_->set_visible(false);
}

void TextureToolWindow::on_texture_selected(int index) {
  std::shared_ptr<fw::Texture> layer(new fw::Texture());
  layer->create(tool_->get_terrain()->get_layer(index));

  std::shared_ptr<Drawable> drawable =
      fw::Framework::get_instance()->get_gui()->get_drawable_manager()->build_drawable(
        layer, 0, 0, layer->get_width(), layer->get_height());
  wnd_->find<Label>(TEXTURE_PREVIEW_ID)->set_background(drawable);
  tool_->set_layer(index);
}

void TextureToolWindow::on_radius_updated(int value) {
  int radius = value / 10;
  tool_->set_radius(radius);
}

// refreshes the icon of the given window which represents the given
// terrain texture layer
void TextureToolWindow::refresh_texture_icon(fw::gui::Window *wnd, int layer_num) {
}

namespace ed {
REGISTER_TOOL("texture", TextureTool);

float TextureTool::max_radius = 10;

TextureTool::TextureTool(EditorWorld *wrld) :
    Tool(wrld), radius_(4), is_painting_(false), layer_(0) {
  wnd_ = new TextureToolWindow(this);
}

TextureTool::~TextureTool() {
  delete wnd_;
}

void TextureTool::activate() {
  Tool::activate();

  fw::Input *inp = fw::Framework::get_instance()->get_input();
  keybind_tokens_.push_back(
      inp->bind_key("Left-Mouse", fw::InputBinding(std::bind(&TextureTool::on_key, this, _1, _2))));

  indicator_ = std::make_shared<IndicatorNode>(terrain_);
  indicator_->set_radius(radius_);
  fw::Framework::get_instance()->get_scenegraph_manager()->enqueue(
    [indicator = indicator_](fw::sg::Scenegraph& sg) {
      sg.add_node(indicator);
    });

  wnd_->show();
}

void TextureTool::deactivate() {
  Tool::deactivate();

  fw::Framework::get_instance()->get_scenegraph_manager()->enqueue(
    [indicator = indicator_](fw::sg::Scenegraph& sg) {
      sg.remove_node(indicator);
    });
  indicator_.reset();

  wnd_->hide();
}

void TextureTool::set_radius(int radius) {
  radius_ = radius;
  indicator_->set_radius(radius);
}

void TextureTool::update() {
  Tool::update();

  fw::Vector cursor_loc = terrain_->get_cursor_location();

  if (is_painting_) {

    // patch_x and patch_z are the patch number(s) we're inside of
    int patch_x = static_cast<int>(cursor_loc[0] / game::Terrain::PATCH_SIZE);
    int patch_z = static_cast<int>(cursor_loc[2] / game::Terrain::PATCH_SIZE);

    // get the splatt texture at the current cursor location
    fw::Bitmap &splatt = terrain_->get_splatt(patch_x, patch_z);

    // scale_x and scale_y represent the number of pixels in the splatt texture
    // per game unit of the terrain
    float scale_x = static_cast<float>(splatt.get_width()) / game::Terrain::PATCH_SIZE;
    float scale_y = static_cast<float>(splatt.get_height()) / game::Terrain::PATCH_SIZE;

    // centre_u and centre_v are the texture coordinates (in the range [0..1])
    // of what the cursor is currently pointing at
    float centre_u = (cursor_loc[0] - (patch_x * game::Terrain::PATCH_SIZE))
        / static_cast<float>(game::Terrain::PATCH_SIZE);
    float centre_v = (cursor_loc[2] - (patch_z * game::Terrain::PATCH_SIZE))
        / static_cast<float>(game::Terrain::PATCH_SIZE);

    // cetre_x and centre_y are the (x,y) corrdinates (in texture space)
    // of the splatt texture where the cursor is currently pointing.
    int centre_x = static_cast<int>(centre_u * splatt.get_width());
    int centre_y = static_cast<int>(centre_v * splatt.get_height());

    fw::Vector center(static_cast<float>(centre_x), static_cast<float>(centre_y), 0.0f);

    // we have to take a copy of the splatt's pixels cause we'll be modifying them
    std::vector<uint32_t> data = splatt.get_pixels();

    uint8_t new_value = layer_;
    for (int y = centre_y - static_cast<int>(radius_ * scale_y); y <= centre_y + static_cast<int>(radius_ * scale_y);
        y++) {
      for (int x = centre_x - static_cast<int>(radius_ * scale_x); x <= centre_x + static_cast<int>(radius_ * scale_x);
          x++) {
        if (y < 0 || x < 0 || y >= splatt.get_height() || x >= splatt.get_width())
          continue;

        fw::Vector v(static_cast<float>(x), static_cast<float>(y), 0.0f);
        if ((v - center).length() > (radius_ * scale_x))
          continue;

        data[(y * splatt.get_width()) + x] = new_value;
      }
    }

    splatt.set_pixels(data);
    fw::Framework::get_instance()->get_graphics()->run_on_render_thread([=]() {
      terrain_->set_splatt(patch_x, patch_z, splatt);
    });
  }

  indicator_->set_center(cursor_loc);
}

void TextureTool::on_key(std::string keyname, bool is_down) {
  if (keyname == "Left-Mouse") {
    is_painting_ = is_down;
  }
}

}
