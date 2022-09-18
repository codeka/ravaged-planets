#pragma once

#include <memory>
#include <string>
#include <stdint.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace fw {

namespace detail {
  class StackWalkerInternal;
}

/** You can inherit from this class and implement the various on_* methods to customize it's behaviour. */
class StackWalker {
public:
  enum options {
    // no addition info will be retrived (only the address is available)
    retrieve_none = 0,

    // try to get the symbol-name
    retrieve_symbol = 1,

    // try to get the line number for this symbol
    retrieve_line = 2,

    // try to retrieve the module-infos
    retrieve_module_info = 4,

    // try to retrieve the version for the DLL/EXE
    retrieve_file_version = 8,

    // a combination of all of the above
    retrieve_all = 0xF,

    // generate a "good" symbol-search-path
    sym_build_path = 0x10,

    // also use the public Microsoft-Symbol-Server (if available)
    sym_use_sym_srv = 0x20,

    // all the "Sym"-options
    sym_all = sym_build_path | sym_use_sym_srv,

    // all options (this is the default)
    options_all = sym_all | retrieve_all
  };

  StackWalker(
    int options = options_all, // 'int' is by design, to kore easily combine the enum-flags
    char const *symbol_path = 0, 
    DWORD process_id = ::GetCurrentProcessId(), 
    HANDLE process = ::GetCurrentProcess()
  );
  StackWalker(DWORD process_id, HANDLE process);
  virtual ~StackWalker();

  typedef BOOL (__stdcall *PReadProcessMemoryRoutine)(
    HANDLE      hProcess,
    DWORD64     qwBaseAddress,
    PVOID       lpBuffer,
    DWORD       nSize,
    LPDWORD     lpNumberOfBytesRead,
    LPVOID      pUserData  // optional data, which was passed in "ShowCallstack"
  );

  bool load_modules();

  bool walk_stack(
    HANDLE thread = GetCurrentThread(), 
    const CONTEXT *context = 0, 
    PReadProcessMemoryRoutine read_memory_function = 0,
    void *user_data = 0  // optional to identify some data in the 'readMemoryFunction'-callback
  );

protected:
  enum { STACKWALK_MAX_NAMELEN = 1024 }; // max name length for found symbols

  struct callstack_entry {
    intptr_t offset;  // if 0, we have no valid entry
    char name[STACKWALK_MAX_NAMELEN];
    char undecorated_name[STACKWALK_MAX_NAMELEN];
    char undecorated_full_name[STACKWALK_MAX_NAMELEN];
    uint64_t offset_from_symbol;
    uint32_t offset_from_line;
    uint32_t line_number;
    char line_file_name[STACKWALK_MAX_NAMELEN];
    uint32_t symbol_type;
    char const *symbol_type_string;
    char module_name[STACKWALK_MAX_NAMELEN];
    intptr_t base_of_image;
    char loaded_image_name[STACKWALK_MAX_NAMELEN];
  };

  enum CallstackEntryType {
    first_entry,
    next_entry,
    last_entry
  };

  virtual void on_sym_init(char const *search_path, uint32_t options, char const *user_name);
  virtual void on_load_module(char const *image, char const *module, uint64_t base_addr, uint32_t size, uint32_t result,
      char const *symbol_type, char const *pdb_name, uint64_t file_version);
  virtual void on_callstack_entry(CallstackEntryType entry_type, callstack_entry &entry);
  virtual void on_dbghelp_error(char const *func_name, uint32_t gle, DWORD64 addr);
  virtual void on_output(char const *text);

  std::shared_ptr<detail::StackWalkerInternal> _sw;
  HANDLE _process;
  uint32_t _process_id;
  bool _modules_loaded;
  std::string _symbol_path;
  int _options;

  static BOOL __stdcall my_read_memory_function(HANDLE hProcess, DWORD64 qwBaseAddress, PVOID lpBuffer, DWORD nSize,
      LPDWORD lpNumberOfBytesRead);

  friend detail::StackWalkerInternal;
};

}
