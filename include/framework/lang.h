#pragma once

#include <unordered_map>

#include "framework/framework.h"

namespace fw {

// Represents the current language that we're displaying all our UI and stuff in.
class lang {
private:
  std::string _lang_name;
  std::unordered_map<std::string, std::string> _def_strings;
  std::unordered_map<std::string, std::string> _strings;

public:
  lang(std::string const &name);

  // gets the value of the given string. If the string isn't defined in
  // the language file, we'll look in the default (English) file.
  std::string get_string(std::string const &name);
};

struct lang_description {
  std::string name;
  std::string display_name;
};

std::vector<lang_description> get_languages();

inline std::string text(std::string const &name) {
  fw::lang *l = fw::framework::get_instance()->get_lang();
  return l->get_string(name);
}

}
