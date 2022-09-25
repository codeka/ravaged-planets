#pragma once

namespace game {

// This structure defines various bits of info about a script that we gather from the *.ai file.
struct ScriptDesc {
  std::string name;
  std::string desc;
  std::string author;
  std::string url;

  boost::filesystem::path filename;
};

/** This class manages the AI scripts. It enumerates them, lets you instantiate them, and so on. */
class ScriptManager {
private:
  void populate_scripts();

public:
  ScriptManager();

  std::vector<ScriptDesc> &get_scripts();
};

}
