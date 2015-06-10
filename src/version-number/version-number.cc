#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>

int get_revision_number(char const *src_directory) {
  std::string command = "git -C \"";
  command += src_directory;
  command += "\" rev-list HEAD --count";
  FILE *fp = popen(command.c_str(), "r");
  if (fp == nullptr) {
    //??
    return 0;
  }
  char buffer[1024];
  fread(buffer, 1, 1024, fp);
  fclose(fp);

  return atoi(buffer);
}

int main(int argc, char const **argv) {
  char const *src_directory = argv[1];
  char const *build_type_in = argv[2];
  char const *dest_file = argv[3];

  int major = 0;
  int minor = 1;
  int revision = get_revision_number(src_directory);
  std::string build_type = build_type_in;
  std::transform(build_type.begin(), build_type.end(), build_type.begin(), ::tolower);

  std::fstream fs;
  fs.open(dest_file, std::ios::out);
  fs << "// version.cc, auto-generated by version-number.cc" << std::endl;
  fs << "#include <framework/version.h>" << std::endl;
  fs << "namespace fw {" << std::endl;
  fs << "int version_major = " << major << ";" << std::endl;
  fs << "int version_minor = " << minor << ";" << std::endl;
  fs << "int version_revision = " << revision << ";" << std::endl;
  fs << "char const * const version_build_type = \"" << build_type << "\";" << std::endl;
  fs << "char const * const version_str = \"v" << major << "." << minor << " build " << revision
      << " (" << build_type << ")\";" << std::endl;
  fs << "char const * const version_date = \"" << __DATE__ << " " << __TIME__ << "\";" << std::endl;
  fs << "}" << std::endl;
  fs.close();
}