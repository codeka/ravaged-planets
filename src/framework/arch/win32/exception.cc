
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include <framework/arch/win32/stack_walker.h>
#include <framework/exception.h>

namespace fw {

class exception_stack_walker : public StackWalker {
private:
  std::vector<std::string> &_stacktrace;
  std::vector<std::string> &_modules;

public:
  exception_stack_walker(std::vector<std::string> &stacktrace, std::vector<std::string> &modules)
    : _stacktrace(stacktrace), _modules(modules) {
  }

  virtual void on_sym_init(char const * /*search_path*/, uint32_t /*options*/, char const * /*user_name*/) {
  }

  virtual void on_load_module(char const *image, char const *module, uint64_t base_addr, uint32_t size, uint32_t result,
      char const *symbol_type, char const *pdb_name, uint64_t file_version) {
    std::string line;
    if (file_version == 0) {
      line = (boost::format("%1%:%2% (%3%), size: %4% bytes (result: %5%), symbol-type: %6%, PDB: '%7%'")
          % image % module % (void *)base_addr
          % size % result % symbol_type % pdb_name).str();
    } else {
      uint32_t v4 = static_cast<uint32_t>(file_version & 0xFFFF);
      uint32_t v3 = static_cast<uint32_t>((file_version >> 16) & 0xFFFF);
      uint32_t v2 = static_cast<uint32_t>((file_version >> 32) & 0xFFFF);
      uint32_t v1 = static_cast<uint32_t>((file_version >> 48) & 0xFFFF);

      line = (boost::format(
          "%1%:%2% (%3%), size: %4% bytes, version: %5%.%6%.%7%.%8% (result: %9%), symbol-type: %10%, PDB: '%11%'")
          % image % module % (void *)base_addr
          % size % v1 % v2 % v3 % v4
          % result % symbol_type % pdb_name).str();
    }

    _modules.push_back(line);
  }

  virtual void on_callstack_entry(CallstackEntryType entry_type, callstack_entry &entry) {
    if (entry_type != last_entry && entry.offset != 0) {
      std::string fn_name = "(function-name not available)";

      if (entry.name[0] != 0)
        fn_name = entry.name;
      if (entry.undecorated_name[0] != 0)
        fn_name = entry.undecorated_name;
      if (entry.undecorated_full_name[0] != 0)
        fn_name = entry.undecorated_full_name;

      std::string file_name = "(filename not available)";
      if (entry.line_file_name[0] != 0) {
        file_name = entry.line_file_name;
        file_name += ":" + boost::lexical_cast<std::string>(static_cast<int>(entry.line_number));
      }

      std::string module_name = "N/A";
      if (entry.module_name[0] != 0)
        module_name = entry.module_name;

      _stacktrace.push_back("   " + fn_name + " [" + module_name + "] " + file_name);
    }
  }

  virtual void on_dbghelp_error(char const * /*func_name*/, uint32_t /*gle*/, DWORD64 /*addr*/) {
  }

  virtual void on_output(char const * /*text*/) {
  }
};

std::vector<std::string> Exception::generate_stack_trace() {
  std::vector<std::string> stacktrace;
  std::vector<std::string> modules;
  try {
    exception_stack_walker sw(stacktrace, modules);
    sw.walk_stack();
  } catch (...) {
    // ignore exceptions
  }

  return stacktrace;
}

}
