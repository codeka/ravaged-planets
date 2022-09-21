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

XmlElement load_xml(fs::path const &filepath, std::string const & format_name, int version) {
  if (!fs::is_regular_file(filepath)) {
    debug << boost::format("error: could not load %1% \"%2%\": no such file") % format_name % filepath.string()
        << std::endl;
    return XmlElement();
  }
  debug << boost::format("loading %1%: \"%2%\"") % format_name % filepath.string() << std::endl;

  std::shared_ptr<xml::XMLDocument> doc(new xml::XMLDocument());
  doc->LoadFile(filepath.string().c_str());
  if (doc->Error()) {
    BOOST_THROW_EXCEPTION(fw::Exception() << fw::filename_error_info(filepath.string())
        << fw::message_error_info(doc->ErrorName()));
  }

  xml::XMLHandle doch(doc.get());
  xml::XMLElement *root = doch.FirstChildElement().ToElement();
  if (root != 0) {
    if (std::string(root->Value()) != format_name) {
      BOOST_THROW_EXCEPTION(fw::Exception() << fw::message_error_info(
          (boost::format("invalid root node, expected \"%1%\" found \"%2%\"") % format_name % root->Value()).str()));
    }

    std::string actual_version = "1";
    if (root->Attribute("version") != nullptr) {
      actual_version = root->Attribute("version");
    }
    if (actual_version != boost::lexical_cast<std::string>(version)) {
      BOOST_THROW_EXCEPTION(fw::Exception()
          << fw::message_error_info((boost::format("invalid %1% version: %2% (expected \"%3%\")")
              % format_name % actual_version % version).str()));
    }
  }

  return XmlElement(doc, root);
}

//-------------------------------------------------------------------------

XmlElement::XmlElement() :
    elem_(0) {
}

XmlElement::XmlElement(std::shared_ptr<xml::XMLDocument> doc, xml::XMLElement *elem) :
    doc_(doc), elem_(elem) {
}

XmlElement::XmlElement(XmlElement const &copy) :
    doc_(copy.doc_), elem_(copy.elem_) {
}

XmlElement::XmlElement(std::string const &xml) {
  std::shared_ptr<xml::XMLDocument> doc(new xml::XMLDocument());
  doc->Parse(xml.c_str());

  doc_ = doc;
  elem_ = doc->FirstChildElement();
}

XmlElement::~XmlElement() {
}

XmlElement &XmlElement::operator =(XmlElement const &copy) {
  doc_ = copy.doc_;
  elem_ = copy.elem_;
  return *this;
}

std::shared_ptr<xml::XMLDocument> XmlElement::get_document() const {
  return doc_;
}

XmlElement XmlElement::get_root() const {
  return XmlElement(doc_, doc_->FirstChildElement());
}

xml::XMLElement *XmlElement::get_element() const {
  return elem_;
}

bool XmlElement::is_valid() const {
  return (elem_ != nullptr);
}

std::string XmlElement::get_name() const {
  return std::string(elem_->Name());
}

std::string XmlElement::get_value() const {
  return std::string(elem_->Value());
}

std::string XmlElement::get_text() const {
  xml::XMLNode *Node = elem_->FirstChild();
  if (Node != nullptr) {
    return std::string(Node->Value());
  }

  return "";
}

std::string XmlElement::get_attribute(std::string const &name) const {
  char const *ParticleRotation = elem_->Attribute(name.c_str());
  if (ParticleRotation == 0) {
    BOOST_THROW_EXCEPTION(fw::Exception()
        << fw::message_error_info((boost::format("'%1%' attribute expected.") % name).str()));
  }

  return ParticleRotation;
}

bool XmlElement::is_attribute_defined(std::string const &name) const {
  return (elem_->Attribute(name.c_str()) != 0);
}

XmlElement XmlElement::get_first_child() const {
  return XmlElement(doc_, elem_->FirstChildElement());
}

XmlElement XmlElement::get_next_sibling() const {
  return XmlElement(doc_, elem_->NextSiblingElement());
}

std::string XmlElement::to_string() const {
  xml::XMLPrinter printer(nullptr, true);
  elem_->Accept(&printer);
  return printer.CStr();
}

std::string XmlElement::to_pretty_string() const {
  xml::XMLPrinter printer(nullptr, false);
  elem_->Accept(&printer);
  return printer.CStr();
}

}
