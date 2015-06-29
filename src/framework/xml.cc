#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <framework/exception.h>
#include <framework/logging.h>
#include <framework/xml.h>

namespace fs = boost::filesystem;

namespace fw {

std::string to_string(xml_error_info const &err_info) {
  fw::xml::XMLError err = err_info.value();
  switch (err) {
  case fw::xml::XML_NO_ATTRIBUTE:
    return "XML_NO_ATTRIBUTE";
  case fw::xml::XML_WRONG_ATTRIBUTE_TYPE:
    return "XML_WRONG_ATTRIBUTE_TYPE";
  case fw::xml::XML_ERROR_FILE_NOT_FOUND:
    return "XML_ERROR_FILE_NOT_FOUND";
  case fw::xml::XML_ERROR_FILE_COULD_NOT_BE_OPENED:
    return "XML_ERROR_FILE_COULD_NOT_BE_OPENED";
  case fw::xml::XML_ERROR_FILE_READ_ERROR:
    return "XML_ERROR_FILE_READ_ERROR";
  case fw::xml::XML_ERROR_ELEMENT_MISMATCH:
    return "XML_ERROR_ELEMENT_MISMATCH";
  case fw::xml::XML_ERROR_PARSING_ELEMENT:
    return "XML_ERROR_PARSING_ELEMENT";
  case fw::xml::XML_ERROR_PARSING_ATTRIBUTE:
    return "XML_ERROR_PARSING_ATTRIBUTE";
  case fw::xml::XML_ERROR_IDENTIFYING_TAG:
    return "XML_ERROR_IDENTIFYING_TAG";
  case fw::xml::XML_ERROR_PARSING_TEXT:
    return "XML_ERROR_PARSING_TEXT";
  case fw::xml::XML_ERROR_PARSING_CDATA:
    return "XML_ERROR_PARSING_CDATA";
  case fw::xml::XML_ERROR_PARSING_COMMENT:
    return "XML_ERROR_PARSING_COMMENT";
  case fw::xml::XML_ERROR_PARSING_DECLARATION:
    return "XML_ERROR_PARSING_DECLARATION";
  case fw::xml::XML_ERROR_PARSING_UNKNOWN:
    return "XML_ERROR_PARSING_UNKNOWN";
  case fw::xml::XML_ERROR_EMPTY_DOCUMENT:
    return "XML_ERROR_EMPTY_DOCUMENT";
  case fw::xml::XML_ERROR_MISMATCHED_ELEMENT:
    return "XML_ERROR_MISMATCHED_ELEMENT";
  case fw::xml::XML_ERROR_PARSING:
    return "XML_ERROR_PARSING";
  case fw::xml::XML_CAN_NOT_CONVERT_TEXT:
    return "XML_CAN_NOT_CONVERT_TEXT";
  case fw::xml::XML_NO_TEXT_NODE:
    return "XML_NO_TEXT_NODE";
  default:
    return "Unknown Error";
  }
}

xml_element load_xml(fs::path const &filepath, std::string const & format_name, int version) {
  if (!fs::is_regular_file(filepath)) {
    debug << boost::format("error: could not load %1% \"%2%\": no such file") % format_name % filepath.string()
        << std::endl;
    return xml_element();
  }
  debug << boost::format("loading %1%: \"%2%\"") % format_name % filepath.string() << std::endl;

  std::shared_ptr<xml::XMLDocument> doc(new xml::XMLDocument());
  doc->LoadFile(filepath.string().c_str());
  if (doc->Error()) {
    BOOST_THROW_EXCEPTION(fw::exception() << fw::filename_error_info(filepath.string())
        << fw::message_error_info(doc->ErrorName()));
  }

  xml::XMLHandle doch(doc.get());
  xml::XMLElement *root = doch.FirstChildElement().ToElement();
  if (root != 0) {
    if (std::string(root->Value()) != format_name) {
      BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info(
          (boost::format("invalid root node, expected \"%1%\" found \"%2%\"") % format_name % root->Value()).str()));
    }

    std::string actual_version(root->Attribute("version"));
    if (actual_version != boost::lexical_cast<std::string>(version)) {
      BOOST_THROW_EXCEPTION(fw::exception() << fw::message_error_info(
          (boost::format("invalid %1% version: %2% (expected \"%3%\")") % format_name % actual_version % version).str()));
    }
  }

  return xml_element(doc, root);
}

//-------------------------------------------------------------------------

xml_element::xml_element() :
    _elem(0) {
}

xml_element::xml_element(std::shared_ptr<xml::XMLDocument> doc, xml::XMLElement *elem) :
    _doc(doc), _elem(elem) {
}

xml_element::xml_element(xml_element const &copy) :
    _doc(copy._doc), _elem(copy._elem) {
}

xml_element::xml_element(std::string const &xml) {
  std::shared_ptr<xml::XMLDocument> doc(new xml::XMLDocument());
  doc->Parse(xml.c_str());

  _doc = doc;
  _elem = doc->FirstChildElement();
}

xml_element::~xml_element() {
}

xml_element &xml_element::operator =(xml_element const &copy) {
  _doc = copy._doc;
  _elem = copy._elem;
  return *this;
}

std::shared_ptr<xml::XMLDocument> xml_element::get_document() const {
  return _doc;
}

xml_element xml_element::get_root() const {
  return xml_element(_doc, _doc->FirstChildElement());
}

xml::XMLElement *xml_element::get_element() const {
  return _elem;
}

bool xml_element::is_valid() const {
  return (_elem != nullptr);
}

std::string xml_element::get_name() const {
  return std::string(_elem->Name());
}

std::string xml_element::get_value() const {
  return std::string(_elem->Value());
}

std::string xml_element::get_text() const {
  xml::XMLNode *node = _elem->FirstChild();
  if (node != nullptr) {
    return std::string(node->Value());
  }

  return "";
}

std::string xml_element::get_attribute(std::string const &name) const {
  char const *value = _elem->Attribute(name.c_str());
  if (value == 0) {
    BOOST_THROW_EXCEPTION(fw::exception()
        << fw::message_error_info((boost::format("'%1%' attribute expected.") % name).str()));
  }

  return value;
}

bool xml_element::is_attribute_defined(std::string const &name) const {
  return (_elem->Attribute(name.c_str()) != 0);
}

xml_element xml_element::get_first_child() const {
  return xml_element(_doc, _elem->FirstChildElement());
}

xml_element xml_element::get_next_sibling() const {
  return xml_element(_doc, _elem->NextSiblingElement());
}

std::string xml_element::to_string() const {
  xml::XMLPrinter printer(nullptr, true);
  _elem->Accept(&printer);
  return printer.CStr();
}

std::string xml_element::to_pretty_string() const {
  xml::XMLPrinter printer(nullptr, false);
  _elem->Accept(&printer);
  return printer.CStr();
}

}
