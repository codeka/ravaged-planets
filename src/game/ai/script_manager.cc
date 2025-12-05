#include <filesystem>

#include <framework/logging.h>
#include <framework/xml.h>
#include <framework/paths.h>
#include <framework/status.h>

#include <game/ai/script_manager.h>

namespace fs = std::filesystem;

namespace game {

namespace {

fw::StatusOr<ScriptDesc> ParseAi(fs::path containing_dir, fs::path ai_file) {
  ASSIGN_OR_RETURN(fw::XmlElement root, fw::LoadXml(ai_file, "ai", /* version= */ 1));

  ScriptDesc desc;
  for (fw::XmlElement elem : root.children()) {
    if (elem.get_value() == "description") {
      ASSIGN_OR_RETURN(desc.name, elem.GetAttribute("name"));
      desc.desc = elem.get_text();
      auto author = elem.GetAttribute("author");
      if (author.ok()) {
        desc.author = *author;
      }
      auto url = elem.GetAttribute("url");
      if (url.ok()) {
        desc.url = *url;
      }
    } else if (elem.get_value() == "script") {
      desc.filename = (containing_dir / elem.get_text());
    }
  }

  return desc;
}

void EnumerateFiles(fs::path dir, std::vector<ScriptDesc> &desc_list) {
  if (!fs::exists(dir))
    return;

  fs::directory_iterator end;
  for (fs::directory_iterator it(dir); it != end; ++it) {
    if (fs::is_directory(it->status())) {
      EnumerateFiles(it->path(), desc_list);
    } else if (it->path().extension() == ".ai") {
      auto desc = ParseAi(dir, it->path());
      if (!desc.ok()) {
        LOG(ERR) << "error loading AI file '" << it->path() << "': " << desc.status();
      } else {
        desc_list.push_back(*desc);
      }
    }
  }
}

}  // namespace


static std::vector<ScriptDesc> g_scripts;
static bool g_populated;

ScriptManager::ScriptManager() {
}

std::vector<ScriptDesc> &ScriptManager::get_scripts() {
  if (!g_populated) {
    populate_scripts();
    g_populated = true;
  }

  return g_scripts;
}

void ScriptManager::populate_scripts() {
  // look through all the files and directories under .\data\ai for *.ai files, and populate the
  // script definitions from there
  EnumerateFiles(fw::install_base_path() / "ai", g_scripts);

  // also look through the data directory.
  EnumerateFiles(fw::user_base_path() / "ai", g_scripts);
}

}
