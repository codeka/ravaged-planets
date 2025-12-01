#include <framework/gui/drawable.h>

#include <filesystem>
#include <string>

#include <absl/strings/numbers.h>
#include <absl/strings/str_split.h>

#include <framework/exception.h>
#include <framework/graphics.h>
#include <framework/paths.h>
#include <framework/texture.h>
#include <framework/shader.h>
#include <framework/status.h>
#include <framework/xml.h>

namespace fw::gui {

namespace {

// Parses an attribute value of the form "n,m" and populates the two integers.
fw::Status ParseTupleAttribute(std::string_view attr_value, int& left, int& right) {
  std::vector<std::string> parts = absl::StrSplit(attr_value, ",");
  if (parts.size() != 2) {
    return fw::ErrorStatus("expected value of the form 'n,m', got: ") << attr_value;
  }

  if (!absl::SimpleAtoi(parts[0], &left) || !absl::SimpleAtoi(parts[1], &right)) {
    return fw::ErrorStatus("expected integers of the form 'n,m', got: ") << attr_value;
  }

  return fw::OkStatus();
}

// All drawables share the same vertex buffer, index buffer
static std::shared_ptr<fw::VertexBuffer> g_vertex_buffer;
static std::shared_ptr<fw::VertexBuffer> g_flipped_vertex_buffer;
static std::shared_ptr<fw::IndexBuffer> g_index_buffer;

}  // namespace

//-----------------------------------------------------------------------------

Drawable::Drawable() {
}

Drawable::~Drawable() {
}

void Drawable::render(float x, float y, float width, float height) {
}

//-----------------------------------------------------------------------------

BitmapDrawable::BitmapDrawable(std::shared_ptr<fw::Texture> texture) :
    top_(0), left_(0), width_(0), height_(0), texture_(texture), flipped_(false) {
  if (g_vertex_buffer == nullptr) {
    fw::vertex::xyz_uv vertices[4];
    vertices[0] = fw::vertex::xyz_uv(0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    vertices[1] = fw::vertex::xyz_uv(0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
    vertices[2] = fw::vertex::xyz_uv(1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    vertices[3] = fw::vertex::xyz_uv(1.0f, 1.0f, 0.0f, 1.0f, 1.0f);
    g_vertex_buffer = fw::VertexBuffer::create<fw::vertex::xyz_uv>(false);
    g_vertex_buffer->set_data(4, vertices);

    vertices[0] = fw::vertex::xyz_uv(0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    vertices[1] = fw::vertex::xyz_uv(0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
    vertices[2] = fw::vertex::xyz_uv(1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
    vertices[3] = fw::vertex::xyz_uv(1.0f, 1.0f, 0.0f, 1.0f, 0.0f);
    g_flipped_vertex_buffer = fw::VertexBuffer::create<fw::vertex::xyz_uv>(false);
    g_flipped_vertex_buffer->set_data(4, vertices);

    g_index_buffer = std::shared_ptr<fw::IndexBuffer>(new fw::IndexBuffer());
    uint16_t indices[4];
    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;
    indices[3] = 3;
    g_index_buffer->set_data(4, indices);
  }

  shader_ = fw::Shader::CreateOrEmpty("gui.shader");
  shader_params_ = shader_->CreateParameters();
  shader_params_->set_texture("texsampler", texture_);
}

fw::Status BitmapDrawable::Initialize(XmlElement const &element) {
  ASSIGN_OR_RETURN(auto pos, element.GetAttribute("pos"));
  RETURN_IF_ERROR(ParseTupleAttribute(pos, left_, top_));

  ASSIGN_OR_RETURN(auto size, element.GetAttribute("size"));
  RETURN_IF_ERROR(ParseTupleAttribute(size, width_, height_));
  return fw::OkStatus();
}

BitmapDrawable::~BitmapDrawable() {
}

fw::Matrix BitmapDrawable::get_uv_transform() {
  texture_->ensure_created();
  const float x = static_cast<float>(left_) / static_cast<float>(texture_->get_width());
  const float y = static_cast<float>(top_) / static_cast<float>(texture_->get_height());
  const float width = static_cast<float>(width_) / static_cast<float>(texture_->get_width());
  const float height = static_cast<float>(height_) / static_cast<float>(texture_->get_height());
  return fw::scale(fw::Vector(width, height, 0.0f)) * fw::translation(fw::Vector(x, y, 0));
}

fw::Matrix BitmapDrawable::get_pos_transform(float x, float y, float width, float height) {
  fw::Graphics *g = fw::Framework::get_instance()->get_graphics();
  fw::Matrix transform =
    fw::projection_orthographic(
      0.0f, static_cast<float>(g->get_width()),
      static_cast<float>(g->get_height()), 0.0f, 1.0f, -1.0f);
  return fw::scale(fw::Vector(width, height, 0.0f))
      * fw::translation(fw::Vector(x, y, 0))
      * transform;
}

void BitmapDrawable::render(float x, float y, float width, float height) {
  // TODO: recalculating this every time seems wasteful.
  shader_params_->set_matrix("pos_transform", get_pos_transform(x, y, width, height));
  shader_params_->set_matrix("uv_transform", get_uv_transform());

  std::shared_ptr<fw::VertexBuffer> vb = flipped_ ? g_flipped_vertex_buffer : g_vertex_buffer;
  vb->begin();
  g_index_buffer->begin();
  shader_->Begin(shader_params_);
  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, nullptr);
  shader_->End();
  g_index_buffer->end();
  vb->end();
}

//-----------------------------------------------------------------------------

NinePatchDrawable::NinePatchDrawable(std::shared_ptr<fw::Texture> texture)
    : BitmapDrawable(texture) {
}

fw::Status NinePatchDrawable::Initialize(XmlElement const &element) {
  for (auto child : element.children()) {
    if (child.get_value() == "inner") {
      ASSIGN_OR_RETURN(auto pos, child.GetAttribute("pos"));
      RETURN_IF_ERROR(ParseTupleAttribute(pos, inner_left_, inner_top_));

      ASSIGN_OR_RETURN(auto size, child.GetAttribute("size"));
      RETURN_IF_ERROR(ParseTupleAttribute(size, inner_width_, inner_height_));
    } else if (child.get_value() == "outer") {
      ASSIGN_OR_RETURN(auto pos, child.GetAttribute("pos"));
      RETURN_IF_ERROR(ParseTupleAttribute(pos, left_, top_));

      ASSIGN_OR_RETURN(auto size, child.GetAttribute("size"));
      RETURN_IF_ERROR(ParseTupleAttribute(size, width_, height_));
    }
  }

  shader_ = fw::Shader::CreateOrEmpty("gui.shader");
  shader_params_ = shader_->CreateParameters();
  shader_params_->set_program_name("ninepatch");
  shader_params_->set_texture("texsampler", texture_);
  return fw::OkStatus();
}

void NinePatchDrawable::render(float x, float y, float width, float height) {
  texture_->ensure_created();

  const float pixel_width = 1.0f / static_cast<float>(texture_->get_width());
  const float pixel_height = 1.0f / static_cast<float>(texture_->get_height());
  const float width_scale = width / static_cast<float>(width_);
  const float height_scale = height / static_cast<float>(height_);

  const float inner_top =
      (static_cast<float>(top_) + static_cast<float>(inner_top_ - top_) / height_scale)
      * pixel_height;
  const float inner_left =
      (static_cast<float>(left_) + static_cast<float>(inner_left_ - left_) / width_scale)
      * pixel_width;
  const float inner_bottom =
      (static_cast<float>(top_ + height_)
          - (static_cast<float>(top_ + height_ - inner_top_ - inner_height_) / height_scale))
      * pixel_height;
  const float inner_right =
      (static_cast<float>(left_ + width_)
          - (static_cast<float>(left_ + width_ - inner_left_ - inner_width_) / width_scale))
      * pixel_width;
  shader_params_->set_scalar("inner_top", inner_top);
  shader_params_->set_scalar("inner_left", inner_left);
  shader_params_->set_scalar("inner_bottom", inner_bottom);
  shader_params_->set_scalar("inner_right", inner_right);
  shader_params_->set_scalar("inner_top_v", static_cast<float>(inner_top_) * pixel_height);
  shader_params_->set_scalar("inner_left_u", static_cast<float>(inner_left_) * pixel_width);
  shader_params_->set_scalar(
        "inner_bottom_v", static_cast<float>(inner_top_ + inner_height_) * pixel_height);
  shader_params_->set_scalar(
        "inner_right_u", static_cast<float>(inner_left_ + inner_width_) * pixel_width);
  shader_params_->set_scalar("fraction_width", width_scale);
  shader_params_->set_scalar("fraction_height", height_scale);
  shader_params_->set_scalar("fraction_width2", width / static_cast<float>(inner_width_));
  shader_params_->set_scalar("fraction_height2", height / static_cast<float>(inner_height_));
  shader_params_->set_scalar("pixel_width", pixel_width);
  shader_params_->set_scalar("pixel_height", pixel_height);

  BitmapDrawable::render(x, y, width, height);
}

//-----------------------------------------------------------------------------

StateDrawable::StateDrawable() :
  curr_state_(kNormal) {
}

StateDrawable::~StateDrawable() {
}

void StateDrawable::add_drawable(State state, std::shared_ptr<Drawable> drawable) {
  drawable_map_[state] = drawable;
}

void StateDrawable::set_current_state(State state) {
  curr_state_ = state;
}

void StateDrawable::render(float x, float y, float width, float height) {
  std::shared_ptr<Drawable> curr_drawable = drawable_map_[curr_state_];
  if (!curr_drawable) {
    curr_drawable = drawable_map_[kNormal];
  }

  if (curr_drawable) {
    curr_drawable->render(x, y, width, height);
  }
}

//-----------------------------------------------------------------------------

DrawableManager::DrawableManager() {
}

DrawableManager::~DrawableManager() {
}

fw::Status DrawableManager::Parse(std::filesystem::path const &file) {
  ASSIGN_OR_RETURN(auto drawables, LoadXml(file, "drawables"));

  for (auto element : drawables.children()) {
    // Parse <image src=""> element
    if (element.get_value() == "image") {
      ASSIGN_OR_RETURN(auto src, element.GetAttribute("src"));
      auto texture = std::make_shared<fw::Texture>();
      texture->create(fw::resolve("gui/drawables/" + src));

      for (auto drawable_elem : element.children()) {
        RETURN_IF_ERROR(ParseDrawableElement(texture, drawable_elem));
      }
    }
  }
  return fw::OkStatus();
}

std::shared_ptr<Drawable> DrawableManager::get_drawable(std::string const &name) {
  return drawables_[name];
}

fw::Status DrawableManager::ParseDrawableElement(
    std::shared_ptr<fw::Texture> texture, XmlElement const &element) {
  std::shared_ptr<Drawable> new_drawable;
  if (element.get_value() == "drawable") {
    auto bitmap_drawable = std::make_shared<BitmapDrawable>(texture);
    RETURN_IF_ERROR(bitmap_drawable->Initialize(element));
    new_drawable = bitmap_drawable;
  } else if (element.get_value() == "ninepatch") {
    auto nine_patch_drawable = std::make_shared<NinePatchDrawable>(texture);
    RETURN_IF_ERROR(nine_patch_drawable->Initialize(element));
    new_drawable = nine_patch_drawable;
  } else {
    return fw::ErrorStatus("unknown element: ") << element.get_value();
  }

  ASSIGN_OR_RETURN(auto name, element.GetAttribute("name"));
  drawables_[name] = new_drawable;
  return fw::OkStatus();
}

std::shared_ptr<Drawable> DrawableManager::build_drawable(std::shared_ptr<fw::Texture> texture,
    float top, float left, float width, float height) {
  auto new_drawable = std::shared_ptr<BitmapDrawable>(new BitmapDrawable(texture));
  new_drawable->top_ = top;
  new_drawable->left_ = left;
  new_drawable->width_ = width;
  new_drawable->height_ = height;
  return new_drawable;
}

}
