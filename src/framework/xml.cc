#include <filesystem>
#include <string>

#include <absl/strings/str_cat.h>

#include <framework/logging.h>
#include <framework/status.h>
#include <framework/xml.h>

namespace fs = std::filesystem;

namespace fw {

std::string to_string(fw::xml::XMLError xml_error) {
  switch (xml_error) {
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

fw::StatusOr<XmlElement> LoadXml(
    fs::path const &filepath, std::string_view format_name, int version) {
  if (!fs::is_regular_file(filepath)) {
    debug << "error: could not load " << format_name << " " << filepath.string()
        << ": no such file" << std::endl;
    return XmlElement();
  }
  debug << "loading " << format_name << ": " << filepath.string() << std::endl;

  std::shared_ptr<xml::XMLDocument> doc(new xml::XMLDocument());
  doc->LoadFile(filepath.string().c_str());
  if (doc->Error()) {
    return fw::ErrorStatus(
        absl::StrCat("could not parse '", filepath.string(), "': ", doc->ErrorName()));
  }

  xml::XMLHandle doch(doc.get());
  xml::XMLElement *root = doch.FirstChildElement().ToElement();
  if (root != nullptr) {
    if (std::string(root->Value()) != format_name) {
      return fw::ErrorStatus(absl::StrCat(
          "invalid root name, expected '", format_name, "' found '", root->Value(), "'"));
    }

    std::string actual_version = "1";
    if (root->Attribute("version") != nullptr) {
      actual_version = root->Attribute("version");
    }
    if (actual_version != std::to_string(version)) {
      return fw::ErrorStatus(absl::StrCat(
          "invalid ", format_name, " version: ", actual_version, " (expected ", version, ")"));
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

fw::StatusOr<std::string> XmlElement::GetAttribute(std::string_view name) const {
  std::string name2(name);
  char const *attr = elem_->Attribute(name2.c_str());
  if (attr == nullptr) {
    return fw::ErrorStatus(absl::StrCat("'", name, "' attribute expected."));
  }

  return std::string(attr);
}

std::string XmlElement::ToString() const {
  xml::XMLPrinter printer(nullptr, true);
  elem_->Accept(&printer);
  return printer.CStr();
}

std::string XmlElement::ToPrettyString() const {
  xml::XMLPrinter printer(nullptr, false);
  elem_->Accept(&printer);
  return printer.CStr();
}

//-------------------------------------------------------------------------

XmlElement::ElementChildIterator& XmlElement::ElementChildIterator::operator++() {
    if (curr_child_ && curr_child_->get_element() != nullptr) {
      const auto doc = curr_child_->get_document();
      const auto next_sibling_element = curr_child_->get_element()->NextSiblingElement();
      if (next_sibling_element == nullptr) {
        curr_child_.reset();
      } else {
        curr_child_ = std::make_shared<XmlElement>(doc, next_sibling_element);
      }
    }
    return *this;
  }

XmlElement::ElementChildIterator XmlElement::ElementChildIterator::operator++(int) {
  auto tmp = *this;
  ++(*this);
  return tmp;
}

bool operator==(
    XmlElement::ElementChildIterator const &a, XmlElement::ElementChildIterator const &b) {
  if (!a.curr_child_ && !b.curr_child_) {
    // Both null, they are equal.
    return true;
  }
  if (!a.curr_child_ || !b.curr_child_) {
    // One is null (and one is not), not equal.
    return false;
  }
  return a.curr_child_ == b.curr_child_;
}

bool operator!=(
    XmlElement::ElementChildIterator const &a, XmlElement::ElementChildIterator const &b) {
  return !(a == b);
}     

XmlElement::ElementChildIterator XmlElement::ElementChildren::begin() {
  auto first_child =
      std::make_shared<XmlElement>(
          element_.get_document(), element_.get_element()->FirstChildElement());
  return ElementChildIterator(first_child);
}

XmlElement::ElementChildIterator XmlElement::ElementChildren::end() {
  return ElementChildIterator();
}


}
