
#include <framework/xml.h>
#include <framework/gui/drawable.h>

namespace fw { namespace gui {

drawable_manager::drawable_manager() {
}

drawable_manager::~drawable_manager() {
}

void drawable_manager::parse(boost::filesystem::path const &file) {
  xml::XMLDocument doc;
  XML_CHECK(doc.LoadFile(file.string().c_str()));

}


} }
