
#include <framework/exception.h>
#include <framework/xml.h>

namespace fw {

std::string to_string(xml_error_info const &err_info) {
  fw::xml::XMLError err = err_info.value();
  switch (err) {
  case fw::xml::XML_NO_ATTRIBUTE: return "XML_NO_ATTRIBUTE";
  case fw::xml::XML_WRONG_ATTRIBUTE_TYPE: return "XML_WRONG_ATTRIBUTE_TYPE";
  case fw::xml::XML_ERROR_FILE_NOT_FOUND: return "XML_ERROR_FILE_NOT_FOUND";
  case fw::xml::XML_ERROR_FILE_COULD_NOT_BE_OPENED: return "XML_ERROR_FILE_COULD_NOT_BE_OPENED";
  case fw::xml::XML_ERROR_FILE_READ_ERROR: return "XML_ERROR_FILE_READ_ERROR";
  case fw::xml::XML_ERROR_ELEMENT_MISMATCH: return "XML_ERROR_ELEMENT_MISMATCH";
  case fw::xml::XML_ERROR_PARSING_ELEMENT: return "XML_ERROR_PARSING_ELEMENT";
  case fw::xml::XML_ERROR_PARSING_ATTRIBUTE: return "XML_ERROR_PARSING_ATTRIBUTE";
  case fw::xml::XML_ERROR_IDENTIFYING_TAG: return "XML_ERROR_IDENTIFYING_TAG";
  case fw::xml::XML_ERROR_PARSING_TEXT: return "XML_ERROR_PARSING_TEXT";
  case fw::xml::XML_ERROR_PARSING_CDATA: return "XML_ERROR_PARSING_CDATA";
  case fw::xml::XML_ERROR_PARSING_COMMENT: return "XML_ERROR_PARSING_COMMENT";
  case fw::xml::XML_ERROR_PARSING_DECLARATION: return "XML_ERROR_PARSING_DECLARATION";
  case fw::xml::XML_ERROR_PARSING_UNKNOWN: return "XML_ERROR_PARSING_UNKNOWN";
  case fw::xml::XML_ERROR_EMPTY_DOCUMENT: return "XML_ERROR_EMPTY_DOCUMENT";
  case fw::xml::XML_ERROR_MISMATCHED_ELEMENT: return "XML_ERROR_MISMATCHED_ELEMENT";
  case fw::xml::XML_ERROR_PARSING: return "XML_ERROR_PARSING";
  case fw::xml::XML_CAN_NOT_CONVERT_TEXT: return "XML_CAN_NOT_CONVERT_TEXT";
  case fw::xml::XML_NO_TEXT_NODE: return "XML_NO_TEXT_NODE";
  default: return "Unknown Error";
  }
}

}
