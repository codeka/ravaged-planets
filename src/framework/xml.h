#pragma once

#include <boost/lexical_cast.hpp>

#include <framework/exception.h>
#include <framework/tinyxml2.h>

#define XML_CHECK(fn) { \
  ::fw::xml::XMLError err = fn; \
  if (err != ::fw::xml::XML_NO_ERROR) { \
    BOOST_THROW_EXCEPTION(fw::Exception() << fw::xml_error_info(err)); \
  } \
}

namespace fw {
// import the tinyxml2 namespace into our own.
namespace xml = tinyxml2;

typedef boost::error_info<struct tag_xmlerror, fw::xml::XMLError> xml_error_info;
std::string to_string(xml_error_info const &err_info);

// This is a wrapper around the TiXmlElement which we use to simplify various things we like to do.
class XmlElement {
private:
  std::shared_ptr<xml::XMLDocument> doc_;
  xml::XMLElement *elem_;

public:
  XmlElement();
  XmlElement(std::shared_ptr<xml::XMLDocument> doc, xml::XMLElement *elem);
  XmlElement(XmlElement const &copy);
  XmlElement(std::string const &xml);
  ~XmlElement();

  XmlElement &operator =(XmlElement const &copy);

  std::shared_ptr<xml::XMLDocument> get_document() const;
  XmlElement get_root() const;
  xml::XMLElement *get_element() const;
  bool is_valid() const;

  std::string get_name() const;
  std::string get_value() const;
  std::string get_text() const;
  std::string get_attribute(std::string const &name) const;

  template <typename T>
  inline T get_attribute(std::string const &name) const {
    return boost::lexical_cast<T>(get_attribute(name));
  }

  bool is_attribute_defined(std::string const &name) const;

  XmlElement get_first_child() const;
  XmlElement get_next_sibling() const;

  // Converts this whole XML element and all it's children into a string. 
  std::string to_string() const;

  // Converts this whole XML element and all it's children into a "pretty" string (with indenting and whatnot).
  std::string to_pretty_string() const;
};

// This is a simple helper that loads an XML document and verifies the basic properties. If it can't, it'll return 0
// and print a diagnostic error to the log file as well.
//
// \param format_name defines what we log (for debugging) and is also expected to be the name of the root XML element.
// \param version The root XML element is expected to have a "version" attribute with a value corresponding to this.
XmlElement load_xml(boost::filesystem::path const &filepath, std::string const &format_name, int version);
}


