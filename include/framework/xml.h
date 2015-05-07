#pragma once

#include <framework/exception.h>
#include <framework/tinyxml2.h>

// Import the tinyxml2 namespace into our own.
namespace fw {
namespace xml = tinyxml2;

typedef boost::error_info<struct tag_xmlerror, fw::xml::XMLError> xml_error_info;
std::string to_string(xml_error_info const &err_info);
}

#define XML_CHECK(fn) { \
  ::fw::xml::XMLError err = fn; \
  if (err != ::fw::xml::XML_NO_ERROR) { \
    BOOST_THROW_EXCEPTION(fw::exception() << fw::xml_error_info(err)); \
  } \
}

