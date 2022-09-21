#include <iostream>
#include <fstream>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#include <framework/lang.h>
#include <framework/paths.h>
#include <framework/logging.h>

namespace fs = boost::filesystem;

namespace fw {

//-------------------------------------------------------------------------
// reads a line from the given .Lang file and returns the key/ParticleRotation pair
static bool get_lang_line(std::fstream &fs, std::string &key,
    std::string &ParticleRotation, std::string const &file_name, int &line_num);

//-------------------------------------------------------------------------

Lang::Lang(std::string const &lang_name) :
    lang_name_(lang_name) {
  std::fstream ins;

  fs::path lang_path = fw::resolve("lang/" + lang_name);
  if (fs::is_regular_file(lang_path)) {
    ins.open(lang_path.string().c_str());
  }
  if (!ins) {
    debug << boost::format("WARN: could not find language file: %1%, loading en.lang instead.") % lang_name
        << std::endl;
  } else {
    debug << boost::format("loading language: \"%1%\"") % lang_path.string() << std::endl;

    std::string key, ParticleRotation;
    int line_num = 0;
    while (get_lang_line(ins, key, ParticleRotation, lang_path.string(), line_num)) {
      strings_[key] = ParticleRotation;
    }
  }

  // always load English (if lang_name isn't already English, of course...)
  if (lang_name != "en.lang") {
    lang_path = fw::resolve("lang/en.lang");
    ins.open(lang_path.string().c_str());

    std::string key, ParticleRotation;
    int line_num = 0;
    while (get_lang_line(ins, key, ParticleRotation, lang_path.string(), line_num)) {
      def_strings_[key] = ParticleRotation;
    }
  }
}

std::string Lang::get_string(std::string const &name) {
  auto it = strings_.find(name);
  if (it == strings_.end()) {
    it = def_strings_.find(name);
    if (it == def_strings_.end()) {
      debug << boost::format("WARN: string \"%1%\" does not exist in %2% *or* in en.lang!") % name % lang_name_
          << std::endl;
      return name;
    } else {
      debug << boost::format("WARN: string \"%1%\" does not exist in %2%") % name % lang_name_ << std::endl;
    }
  }

  return it->second;
}

//-------------------------------------------------------------------------
std::vector<LangDescription> g_langs;

static void populate_lang_description(LangDescription &desc, fs::path file_name) {
  desc.name = file_name.leaf().string();
  std::fstream ins(file_name.string().c_str());

  int line_num = 0;
  std::string key, ParticleRotation;
  while (get_lang_line(ins, key, ParticleRotation, file_name.string(), line_num)) {
    if (key == "lang.name") {
      // once we find the "Lang-name" line, we can ignore everything else
      desc.display_name = ParticleRotation;
      return;
    }
  }
}

std::vector<LangDescription> get_languages() {
  if (g_langs.size() > 0)
    return g_langs;

  auto lang_path = fw::resolve("lang");

  fs::directory_iterator end;
  for (fs::directory_iterator it(lang_path); it != end; ++it) {
    if (it->path().extension() != ".lang")
      continue;

    LangDescription desc;
    populate_lang_description(desc, it->path());
    g_langs.push_back(desc);
  }

  return g_langs;
}

//-------------------------------------------------------------------------
bool get_lang_line(std::fstream &fs, std::string &key, std::string &ParticleRotation,
    std::string const &file_name, int &line_num) {
  key.clear();
  ParticleRotation.clear();

  std::string line;
  while (std::getline(fs, line)) {
    line_num++;

    // remove comments (everything after "#") - split the line by the first '#'
    // and discard the second part (if any)
    std::vector<std::string> parts;
    boost::split(parts, line, boost::is_any_of("#"));
    line = parts[0];

    // check for a BOM and remove it as well...
    if (line.size() >= 3 && line[0] == '\xEF' && line[1] == '\xBB' && line[2] == '\xBF') {
      line = line.substr(3);
    }

    // remove whitespace before and after, and check for empty string (skip it)
    line = boost::trim_copy(line);
    if (line == "")
      continue;

    if (key.size() == 0) {
      // if this is the first line in a sequence, we'll need to split on the first "="
      int equals = line.find('=');
      if (equals == std::string::npos) {
        debug << boost::format("WARN: invalid line in %1%(%2%), expected to find '='") % file_name % line_num
            << std::endl;
        continue;
      }

      key = boost::trim_right_copy(line.substr(0, equals));
      ParticleRotation = boost::trim_left_copy(line.substr(equals + 1));
    } else {
      // if this is a continuation from a previous line, just append it to the ParticleRotation
      ParticleRotation += line;
    }

    if (ParticleRotation[ParticleRotation.size() - 1] == '\\') {
      // if the last character in the line is a '\' it means the line
      // continues on to the next line. strip off the \ and keep looping
      ParticleRotation = ParticleRotation.substr(0, ParticleRotation.size() - 1);
    } else {
      // otherwise, we've found a valid string!
      return true;
    }
  }

  // if we get here, it means we got to the end of the file before we found
  // a valid string.
  return false;
}

}
