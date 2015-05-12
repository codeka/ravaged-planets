#include <string>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <framework/exception.h>
#include <framework/graphics.h>
#include <framework/index_buffer.h>
#include <framework/paths.h>
#include <framework/texture.h>
#include <framework/vertex_buffer.h>
#include <framework/vertex_formats.h>
#include <framework/shader.h>
#include <framework/xml.h>
#include <framework/gui/drawable.h>

namespace fw {
namespace gui {

// Parses an attribute value of the form "n,m" and populates the two integers. Throws an exception if we can't parse
// the value successfully.
void parse_tuple_attribute(std::string attr_value, int &left, int &right) {
  std::vector<std::string> parts;
  boost::split(parts, attr_value, boost::is_any_of(","));
  if (parts.size() != 2) {
    BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info("Expected value of the form 'n,m'"));
  }

  left = boost::lexical_cast<int>(parts[0]);
  right = boost::lexical_cast<int>(parts[1]);
}

// All drawables share the same vertex buffer, index buffer
static std::shared_ptr<fw::vertex_buffer> g_vertex_buffer;
static std::shared_ptr<fw::index_buffer> g_index_buffer;

//-----------------------------------------------------------------------------

drawable::drawable() {
}

drawable::~drawable() {
}

void drawable::render(float x, float y, float width, float height) {
}

//-----------------------------------------------------------------------------

bitmap_drawable::bitmap_drawable(std::shared_ptr<fw::texture> texture) :
    _top(0), _left(0), _width(0), _height(0), _texture(texture) {
  if (g_vertex_buffer == nullptr) {
    g_vertex_buffer = fw::vertex_buffer::create<fw::vertex::xyz_uv>(false);
    fw::vertex::xyz_uv vertices[4];
    vertices[0] = fw::vertex::xyz_uv(0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    vertices[1] = fw::vertex::xyz_uv(0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
    vertices[2] = fw::vertex::xyz_uv(1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    vertices[3] = fw::vertex::xyz_uv(1.0f, 1.0f, 0.0f, 1.0f, 1.0f);
    g_vertex_buffer->set_data(4, vertices);

    g_index_buffer = std::shared_ptr<fw::index_buffer>(new fw::index_buffer());
    uint16_t indices[4];
    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;
    indices[3] = 3;
    g_index_buffer->set_data(4, indices);
  }
}

bitmap_drawable::bitmap_drawable(std::shared_ptr<fw::texture> texture, fw::xml::XMLElement *elem) :
    bitmap_drawable(texture) {
  parse_tuple_attribute(elem->Attribute("pos"), _left, _top);
  parse_tuple_attribute(elem->Attribute("size"), _width, _height);
  _shader = fw::shader::create("gui");
  _shader_params = _shader->create_parameters();
  _shader_params->set_texture("texsampler", _texture);
}

bitmap_drawable::~bitmap_drawable() {
}

void bitmap_drawable::render(float x, float y, float width, float height) {
  fw::matrix pos_transform;
  cml::matrix_orthographic_RH(pos_transform, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f, -1.0f, cml::z_clip_neg_one);
  pos_transform = fw::scale(fw::vector(width, height, 0.0f)) * fw::translation(fw::vector(x, y, 0)) * pos_transform;
  _shader_params->set_matrix("pos_transform", pos_transform);

  x = static_cast<float>(_left) / static_cast<float>(_texture->get_width());
  y = static_cast<float>(_top) / static_cast<float>(_texture->get_height());
  width = static_cast<float>(_width) / static_cast<float>(_texture->get_width());
  height = static_cast<float>(_height) / static_cast<float>(_texture->get_height());
  fw::matrix uv_transform = fw::scale(fw::vector(width, height, 0.0f)) * fw::translation(fw::vector(x, y, 0));
  _shader_params->set_matrix("uv_transform", uv_transform);

  g_vertex_buffer->begin();
  g_index_buffer->begin();
  _shader->begin(_shader_params);
  FW_CHECKED(glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, nullptr));
  _shader->end();
  g_index_buffer->end();
  g_vertex_buffer->end();
}

//-----------------------------------------------------------------------------

ninepatch_drawable::ninepatch_drawable(std::shared_ptr<fw::texture> texture, fw::xml::XMLElement *elem) :
    bitmap_drawable(texture) {
  for (xml::XMLElement *child_elem = elem->FirstChildElement(); child_elem != nullptr;
      child_elem = child_elem->NextSiblingElement()) {
    if (std::string(child_elem->Name()) == "inner") {
      parse_tuple_attribute(child_elem->Attribute("pos"), _inner_left, _inner_top);
      parse_tuple_attribute(child_elem->Attribute("size"), _inner_width, _inner_height);
    } else if (std::string(child_elem->Name()) == "outer") {
      parse_tuple_attribute(child_elem->Attribute("pos"), _left, _top);
      parse_tuple_attribute(child_elem->Attribute("size"), _width, _height);
    }
  }

  _shader = fw::shader::create("gui_ninepatch");
  _shader_params = _shader->create_parameters();
  _shader_params->set_texture("texsampler", _texture);
}

void ninepatch_drawable::render(float x, float y, float width, float height) {
  float pixel_width = 1.0f / static_cast<float>(_texture->get_width());
  float pixel_height = 1.0f / static_cast<float>(_texture->get_height());
  float width_scale = width / static_cast<float>(_width);
  float height_scale = height / static_cast<float>(_height);

  float inner_top = (static_cast<float>(_top) + static_cast<float>(_inner_top - _top) / height_scale) * pixel_height;
  float inner_left = (static_cast<float>(_left) + static_cast<float>(_inner_left - _left) / width_scale) * pixel_width;
  float inner_bottom = (static_cast<float>(_top + _height) - (static_cast<float>(_top + _height - _inner_top - _inner_height) / height_scale)) * pixel_height;
  float inner_right = (static_cast<float>(_left + _width) - (static_cast<float>(_left + _width - _inner_left - _inner_width) / width_scale)) * pixel_width;
  _shader_params->set_scalar("inner_top", inner_top);
  _shader_params->set_scalar("inner_left", inner_left);
  _shader_params->set_scalar("inner_bottom", inner_bottom);
  _shader_params->set_scalar("inner_right", inner_right);
  _shader_params->set_scalar("inner_top_v", static_cast<float>(_inner_top) * pixel_height);
  _shader_params->set_scalar("inner_left_u", static_cast<float>(_inner_left) * pixel_width);
  _shader_params->set_scalar("inner_bottom_v", static_cast<float>(_inner_top + _inner_height) * pixel_height);
  _shader_params->set_scalar("inner_right_u", static_cast<float>(_inner_left + _inner_width) * pixel_width);
  _shader_params->set_scalar("fraction_width", width_scale);
  _shader_params->set_scalar("fraction_height", height_scale);
  _shader_params->set_scalar("fraction_width2", width / static_cast<float>(_inner_width));
  _shader_params->set_scalar("fraction_height2", height / static_cast<float>(_inner_height));
  _shader_params->set_scalar("pixel_width", pixel_width);
  _shader_params->set_scalar("pixel_height", pixel_height);

  bitmap_drawable::render(x, y, width, height);
}

//-----------------------------------------------------------------------------

state_drawable::state_drawable() :
  _curr_state(normal) {
}

state_drawable::~state_drawable() {
}

void state_drawable::add_drawable(state state, std::shared_ptr<drawable> drawable) {
  _drawable_map[state] = drawable;
}

void state_drawable::set_current_state(state state) {
  _curr_state = state;
}

void state_drawable::render(float x, float y, float width, float height) {
  std::shared_ptr<drawable> curr_drawable = _drawable_map[_curr_state];
  if (!curr_drawable) {
    curr_drawable = _drawable_map[normal];
  }

  if (curr_drawable) {
    curr_drawable->render(x, y, width, height);
  }
}

//-----------------------------------------------------------------------------

drawable_manager::drawable_manager() {
}

drawable_manager::~drawable_manager() {
}

void drawable_manager::parse(boost::filesystem::path const &file) {
  xml::XMLDocument doc;
  try {
    XML_CHECK(doc.LoadFile(file.string().c_str()));

    xml::XMLElement *root_elem = doc.FirstChildElement("drawables");
    for (xml::XMLElement *child_elem = root_elem->FirstChildElement(); child_elem != nullptr;
        child_elem = child_elem->NextSiblingElement()) {
      // Parse <image src=""> element
      if (std::string(child_elem->Name()) == "image") {
        std::string src(child_elem->Attribute("src"));
        std::shared_ptr<fw::texture> texture(new fw::texture());
        texture->create(fw::resolve(std::string("gui/drawables/") + src));

        for (xml::XMLElement *drawable_elem = child_elem->FirstChildElement(); drawable_elem != nullptr;
            drawable_elem = drawable_elem->NextSiblingElement()) {
          parse_drawable_element(texture, drawable_elem);
        }
      }
    }
  } catch (fw::exception &e) {
    // If we get an exception here, add the filename we were trying to parse.
    e << fw::filename_error_info(file);
    throw e;
  }

}

std::shared_ptr<drawable> drawable_manager::get_drawable(std::string const &name) {
  return _drawables[name];
}

void drawable_manager::parse_drawable_element(std::shared_ptr<fw::texture> texture, fw::xml::XMLElement *elem) {
  std::shared_ptr<drawable> new_drawable;
  if (elem->Name() == nullptr) {
    BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info(std::string("Element has null name: ") + elem->Value()));
  }
  std::string type_name(elem->Name());
  if (type_name == "drawable") {
    new_drawable = std::shared_ptr<drawable>(new bitmap_drawable(texture, elem));
  } else if (type_name == "ninepatch") {
    new_drawable = std::shared_ptr<drawable>(new ninepatch_drawable(texture, elem));
  } else {
    BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info("Unknown element: " + type_name));
  }

  if (elem->Attribute("name") == nullptr) {
    BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info("Attribute 'name' is required."));
  }
  std::string name(elem->Attribute("name"));
  _drawables[name] = new_drawable;
}

}
}
