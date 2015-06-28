#pragma once

namespace game {

// This structure defines various bits of info about a script that we gather from the *.ai file.
struct script_desc {
  std::string name;
  std::string desc;
  std::string author;
  std::string url;

  boost::filesystem::path filename;
};

/** This class manages the AI scripts. It enumerates them, lets you instantiate them, and so on. */
class ai_scriptmgr {
private:
  void populate_scripts();

public:
  ai_scriptmgr();

  std::vector<script_desc> &get_scripts();
};

}
