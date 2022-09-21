#include <boost/filesystem.hpp>

#include <framework/logging.h>
#include <framework/xml.h>
#include <framework/paths.h>

#include <game/ai/ai_scriptmgr.h>

namespace fs = boost::filesystem;

namespace game {

static std::vector<script_desc> g_scripts;
static bool g_populated;

ai_scriptmgr::ai_scriptmgr() {
}

std::vector<script_desc> &ai_scriptmgr::get_scripts() {
  if (!g_populated) {
    populate_scripts();
    g_populated = true;
  }

  return g_scripts;
}

void parse_ai(fs::path containing_dir, fw::XmlElement &root, script_desc &desc) {
  for (fw::XmlElement elem = root.get_first_child(); elem.is_valid(); elem = elem.get_next_sibling()) {
    if (elem.get_value() == "description") {
      desc.name = elem.get_attribute("name");
      desc.desc = elem.get_text();
      if (elem.is_attribute_defined("author")) {
        desc.author = elem.get_attribute("author");
      }
      if (elem.is_attribute_defined("url")) {
        desc.url = elem.get_attribute("url");
      }
    } else if (elem.get_value() == "script") {
      desc.filename = (containing_dir / elem.get_text());
    }
  }
}

void enumerate_files(fs::path dir, std::vector<script_desc> &desc_list) {
  if (!fs::exists(dir))
    return;

  fs::directory_iterator end;
  for (fs::directory_iterator it(dir); it != end; ++it) {
    if (fs::is_directory(it->status())) {
      enumerate_files(it->path(), desc_list);
    } else if (it->path().extension() == ".ai") {
      fw::XmlElement root = fw::load_xml(it->path(), "ai", 1);

      script_desc desc;
      parse_ai(dir, root, desc);
      desc_list.push_back(desc);
    }
  }
}

void ai_scriptmgr::populate_scripts() {
  // look through all the files and directories under .\data\ai for *.ai
  // files, and populate the script definitions from there
  enumerate_files(fw::install_base_path() / "ai", g_scripts);

  // also look through the data directory.
  enumerate_files(fw::user_base_path() / "ai", g_scripts);
}

}
