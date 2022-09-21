#pragma once

#include <unordered_map>
#include <vector>

#include "framework/framework.h"

namespace fw {

// Represents the current language that we're displaying all our UI and stuff in.
class Lang {
private:
  std::string lang_name_;
  std::unordered_map<std::string, std::string> def_strings_;
  std::unordered_map<std::string, std::string> strings_;

public:
  Lang(std::string const &name);

  // gets the value of the given string. If the string isn't defined in
  // the language file, we'll look in the default (English) file.
  std::string get_string(std::string const &name);
};

struct LangDescription {
  std::string name;
  std::string display_name;
};

std::vector<LangDescription> get_languages();

inline std::string text(std::string const &name) {
  fw::Lang *l = fw::Framework::get_instance()->get_lang();
  return l->get_string(name);
}

}
