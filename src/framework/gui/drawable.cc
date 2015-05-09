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
static std::shared_ptr<fw::shader> g_shader;
static std::shared_ptr<fw::vertex_buffer> g_vertex_buffer;
static std::shared_ptr<fw::index_buffer> g_index_buffer;
static std::shared_ptr<fw::texture> g_texture;

//-----------------------------------------------------------------------------

drawable::drawable() :
    _top(0), _left(0), _width(0), _height(0) {
  if (g_vertex_buffer == nullptr) {
    g_vertex_buffer = fw::vertex_buffer::create<fw::vertex::xyz_uv>(false);
    fw::vertex::xyz_uv vertices[4];
    vertices[0] = fw::vertex::xyz_uv(0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    vertices[1] = fw::vertex::xyz_uv(0.0f, -1.0f, 0.0f, 0.0f, 1.0f);
    vertices[2] = fw::vertex::xyz_uv(1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    vertices[3] = fw::vertex::xyz_uv(1.0f, -1.0f, 0.0f, 1.0f, 1.0f);
    g_vertex_buffer->set_data(4, vertices);

    g_index_buffer = std::shared_ptr<fw::index_buffer>(new fw::index_buffer());
    uint16_t indices[4];
    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;
    indices[3] = 3;
    g_index_buffer->set_data(4, indices);

    g_shader = fw::shader::create("gui");

    g_texture = std::shared_ptr<fw::texture>(new fw::texture());
    g_texture->create(fw::resolve("gui/drawables/elements.png"));
  }
}

drawable::drawable(fw::xml::XMLElement *elem) : drawable() {
  parse_tuple_attribute(elem->Attribute("pos"), _left, _top);
  parse_tuple_attribute(elem->Attribute("size"), _width, _height);
  _shader_params = g_shader->create_parameters();
  _shader_params->set_texture("uv", g_texture);
}

void drawable::render() {
  g_vertex_buffer->begin();
  g_index_buffer->begin();
  g_shader->begin(_shader_params);
  FW_CHECKED(glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, nullptr));
  g_shader->end();
  g_index_buffer->end();
  g_vertex_buffer->end();
}

//-----------------------------------------------------------------------------

ninepatch_drawable::ninepatch_drawable(fw::xml::XMLElement *elem) {
}

//-----------------------------------------------------------------------------

drawable_manager::drawable_manager() {
}

drawable_manager::~drawable_manager() {
}

void drawable_manager::parse(boost::filesystem::path const &file) {
  xml::XMLDocument doc;
  XML_CHECK(doc.LoadFile(file.string().c_str()));

  xml::XMLElement *root_elem = doc.FirstChildElement("drawables");
  for (xml::XMLElement *drawable_elem = root_elem->FirstChildElement(); drawable_elem != nullptr;
      drawable_elem = drawable_elem->NextSiblingElement()) {
    parse_drawable_element(drawable_elem);
  }
}

std::shared_ptr<drawable> drawable_manager::get_drawable(std::string const &name) {
  return _drawables[name];
}

void drawable_manager::parse_drawable_element(fw::xml::XMLElement *elem) {
  std::shared_ptr<drawable> new_drawable;
  if (elem->Name() == nullptr) {
    BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info(std::string("Element has null name: ") + elem->Value()));
  }
  std::string type_name(elem->Name());
  if (type_name == "drawable") {
    new_drawable = std::shared_ptr<drawable>(new drawable(elem));
  } else if (type_name == "ninepatch") {
    new_drawable = std::shared_ptr<drawable>(new ninepatch_drawable(elem));
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
