#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <utility>

#include <absl/strings/numbers.h>

#include <framework/exception.h>
#include <framework/logging.h>
#include <framework/status.h>
#include <framework/tinyxml2.h>

namespace fw {
// import the tinyxml2 namespace into our own.
namespace xml = tinyxml2;

// This is a wrapper around the TiXmlElement which we use to simplify various things we like to do.
class XmlElement {
private:
  std::shared_ptr<xml::XMLDocument> doc_;
  xml::XMLElement *elem_;  // Owned by the XMLDocument

public:
  XmlElement();
  XmlElement(std::shared_ptr<xml::XMLDocument> doc, xml::XMLElement *elem);
  XmlElement(XmlElement const &copy);
  XmlElement(std::string_view xml);
  ~XmlElement();

  XmlElement &operator =(XmlElement const &copy);

  std::shared_ptr<xml::XMLDocument> get_document() const;
  XmlElement get_root() const;
  xml::XMLElement *get_element() const;
  bool is_valid() const;

  std::string get_name() const;
  std::string get_value() const;
  std::string get_text() const;
  fw::StatusOr<std::string> GetAttribute(std::string_view name) const;

  template <typename T>
  inline fw::StatusOr<uint64_t> GetAttributei(std::string_view name) const {
    ASSIGN_OR_RETURN(auto attr, GetAttribute(name));
    T value;
    if (!absl::SimpleAtoi(attr, &value)) {
      return fw::ErrorStatus("invalid integer: ") << value;
    }
    return value;
  }
  template <typename T>
  inline fw::StatusOr<float> GetAttributef(std::string_view name) const {
    ASSIGN_OR_RETURN(auto attr, GetAttribute(name));
    T value;
    if (!absl::SimpleAtof(attr, &value)) {
      return fw::ErrorStatus("invalid integer: ") << value;
    }
    return value;
  }
  
  // Converts this whole XML element and all it's children into a string. 
  std::string ToString() const;

  // Converts this whole XML element and all it's children into a "pretty" string (with indenting and whatnot).
  std::string ToPrettyString() const;

  // An iterator for iterating the children of an XmlElement.
  class ElementChildIterator {
  public:
    ElementChildIterator() {}
    ElementChildIterator(std::shared_ptr<XmlElement const> curr_child)
        : curr_child_(curr_child) {}

    using value_type = XmlElement;
    using pointer    = XmlElement const *;
    using reference  = XmlElement const &;

    reference operator*() const { return *curr_child_; }
    pointer operator->() { return curr_child_.get(); }

    ElementChildIterator& operator++();
    ElementChildIterator operator++(int);

    friend bool operator==(ElementChildIterator const &a, ElementChildIterator const &b);
    friend bool operator!=(ElementChildIterator const &a, ElementChildIterator const &b);

  private:
    std::shared_ptr<XmlElement const> curr_child_;
  };

  // Class that wraps a collection of an elements "children", which can be used in a for loop like,
  // for (auto &child : element.children()) {}
  class ElementChildren {
  public:
    ElementChildren(XmlElement const &element) : element_(element) {}

    ElementChildIterator begin();
    ElementChildIterator end();

  private:
    XmlElement const &element_;
  };

  // TODO: support filtering?
  ElementChildren children() const {
    return ElementChildren(*this);
  }
};

// This is a simple helper that loads an XML document and verifies the basic properties. If it can't, it'll return 0
// and print a diagnostic error to the log file as well.
//
// \param format_name defines what we log (for debugging) and is also expected to be the name of the root XML element.
// \param version The root XML element is expected to have a "version" attribute with a value corresponding to this.
fw::StatusOr<XmlElement> LoadXml(
    std::filesystem::path const &filepath,
    std::string_view format_name,
    int version = 1);

}
