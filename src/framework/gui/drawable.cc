#include <string>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <framework/exception.h>
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

//-----------------------------------------------------------------------------

drawable::drawable() :
        _top(0), _left(0), _width(0), _height(0) {
}

drawable::drawable(fw::xml::XMLElement *elem) : drawable() {
  parse_tuple_attribute(elem->Attribute("pos"), _left, _top);
  parse_tuple_attribute(elem->Attribute("size"), _width, _height);
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
