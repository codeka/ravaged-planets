#include <filesystem>

#include <absl/strings/str_cat.h>

#include <framework/exception.h>
#include <framework/logging.h>
#include <framework/xml.h>

namespace fs = std::filesystem;

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

XmlElement load_xml(fs::path const &filepath, std::string_view format_name, int version) {
  if (!fs::is_regular_file(filepath)) {
    debug << "error: could not load " << format_name << " " << filepath.string()
        << ": no such file" << std::endl;
    return XmlElement();
  }
  debug << "loading " << format_name << ": " << filepath.string() << std::endl;

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
          absl::StrCat("invalid root node, expected \"", format_name, "\" found \"", root->Value(), "\"")));
    }

    std::string actual_version = "1";
    if (root->Attribute("version") != nullptr) {
      actual_version = root->Attribute("version");
    }
    if (actual_version != boost::lexical_cast<std::string>(version)) {
      BOOST_THROW_EXCEPTION(fw::Exception()
          << fw::message_error_info(absl::StrCat("invalid ", format_name, " version: ", actual_version, " (expected ", version, ")")));
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

XmlElement::XmlElement(std::string_view xml) {
  std::shared_ptr<xml::XMLDocument> doc(new xml::XMLDocument());
  doc->Parse(xml.data(), xml.size());

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

std::string XmlElement::get_attribute(std::string_view name) const {
  std::string name2(name);
  char const *attr = elem_->Attribute(name2.c_str());
  if (attr == nullptr) {
    BOOST_THROW_EXCEPTION(fw::Exception()
        << fw::message_error_info(absl::StrCat("'", name, "' attribute expected.")));
  }

  return attr;
}

bool XmlElement::is_attribute_defined(std::string_view name) const {
  std::string name2(name);
  return (elem_->Attribute(name2.c_str()) != 0);
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
