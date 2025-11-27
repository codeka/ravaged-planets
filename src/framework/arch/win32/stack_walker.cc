// adapted from: Http://www.codeproject.com/threads/StackWalker.asp

#include <filesystem>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dbghelp.h>
#include <psapi.h>

#include <framework/arch/win32/stack_walker.h>
#include <framework/logging.h>
#include <framework/settings.h>
#include <framework/misc.h>

namespace fs = std::filesystem;

// normally it should be enough to use 'CONTEXT_FULL' (better would be 'CONTEXT_ALL')
#define USED_CONTEXT_FLAGS CONTEXT_FULL

// we hard-code the structure here because we don't actually want the "latest" version of the struct
// (in case we're run on an older version of dbghelp.dll)
struct IMAGEHLP_MODULE64_V2 {
  DWORD    SizeOfStruct;           // set to sizeof(IMAGEHLP_MODULE64)
  DWORD64  BaseOfImage;            // base load address of module
  DWORD    ImageSize;              // virtual size of the loaded module
  DWORD    TimeDateStamp;          // date/time stamp from pe header
  DWORD    CheckSum;               // checksum from the pe header
  DWORD    NumSyms;                // number of symbols in the symbol table
  SYM_TYPE SymType;                // type of symbols loaded
  CHAR     ModuleName[32];         // module name
  CHAR     ImageName[256];         // image name
  CHAR     LoadedImageName[256];   // symbol file name
};

// The "ugly" assembler-implementation is needed for systems before XP
// If you have a new PSDK and you only compile for XP and later, then you can use 
// the "RtlCaptureContext"
// Currently there is no define which determines the PSDK-Version... 
// So we just use the compiler-version (and assumes that the PSDK is 
// the one which was installed by the VS-IDE)
#if defined(_M_IX86) && (_WIN32_WINNT <= 0x0500) && (_MSC_VER < 1400)

#ifdef CURRENT_THREAD_VIA_EXCEPTION
// TODO: The following is not a "good" implementation, 
// because the callstack is only valid in the "__except" block...
#define GET_CURRENT_CONTEXT(c, contextFlags) \
  do { \
    memset(&c, 0, sizeof(CONTEXT)); \
    EXCEPTION_POINTERS *pExp = NULL; \
    __try { \
      throw 0; \
    } __except( ( (pExp = GetExceptionInformation()) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_EXECUTE_HANDLER)) {} \
    if (pExp != NULL) \
      memcpy(&c, pExp->ContextRecord, sizeof(CONTEXT)); \
      c.ContextFlags = contextFlags; \
  } while(0);
#else
// The following should be enough for walking the callstack...
#define GET_CURRENT_CONTEXT(c, contextFlags) \
  do { \
    memset(&c, 0, sizeof(CONTEXT)); \
    c.ContextFlags = contextFlags; \
    __asm    call x \
    __asm x: pop eax \
    __asm    mov c.Eip, eax \
    __asm    mov c.Ebp, ebp \
    __asm    mov c.Esp, esp \
  } while(0);
#endif

#else

// The following is defined for x86 (XP and higher), x64 and IA64:
#define GET_CURRENT_CONTEXT(c, contextFlags) \
  do { \
    memset(&c, 0, sizeof(CONTEXT)); \
    c.ContextFlags = contextFlags; \
    RtlCaptureContext(&c); \
} while(0);
#endif

namespace fw {
namespace detail {

class StackWalkerInternal {
public:

  // SymCleanup()
  typedef BOOL (__stdcall *tSymCleanup)( HANDLE hProcess );
  tSymCleanup pSymCleanup;

  // SymFunctionTableAccess64()
  typedef PVOID (__stdcall *tSymFunctionTableAccess64)( HANDLE hProcess, DWORD64 AddrBase );
  tSymFunctionTableAccess64 pSymFunctionTableAccess64;

  // SymGetLineFromAddr64()
  typedef BOOL (__stdcall *tSymGetLineFromAddr64)( HANDLE hProcess, DWORD64 dwAddr,
    OUT PDWORD pdwDisplacement, OUT PIMAGEHLP_LINE64 Line );
  tSymGetLineFromAddr64 pSymGetLineFromAddr64;

  // SymGetModuleBase64()
  typedef DWORD64 (__stdcall *tSymGetModuleBase64)( HANDLE hProcess, DWORD64 dwAddr );
  tSymGetModuleBase64 pSymGetModuleBase64;

  // SymGetModuleInfo64()
  typedef BOOL (__stdcall *tSymGetModuleInfo64)( HANDLE hProcess, DWORD64 dwAddr, IMAGEHLP_MODULE64_V2 *ModuleInfo );
  tSymGetModuleInfo64 pSymGetModuleInfo64;

  //  // SymGetModuleInfo64()
  //  typedef BOOL (__stdcall *tSymGetModuleInfo64_V3)( IN HANDLE hProcess, IN DWORD64 dwAddr, OUT IMAGEHLP_MODULE64_V3 *ModuleInfo );
  //  tSymGetModuleInfo64_V3 pSymGetModuleInfo64_V3;

  // SymGetOptions()
  typedef DWORD (__stdcall *tSymGetOptions)( VOID );
  tSymGetOptions pSymGetOptions;

  // SymGetSymFromAddr64()
  typedef BOOL (__stdcall *tSymGetSymFromAddr64)( IN HANDLE hProcess, IN DWORD64 dwAddr,
    OUT PDWORD64 pdwDisplacement, OUT PIMAGEHLP_SYMBOL64 Symbol );
  tSymGetSymFromAddr64 pSymGetSymFromAddr64;

  // SymInitialize()
  typedef BOOL (__stdcall *tSymInitialize)( IN HANDLE hProcess, IN PCSTR UserSearchPath, IN BOOL fInvadeProcess );
  tSymInitialize pSymInitialize;

  // SymLoadModule64()
  typedef DWORD64 (__stdcall *tSymLoadModule64)( HANDLE hProcess, HANDLE hFile,
    PCSTR ImageName, PCSTR ModuleName, DWORD64 BaseOfDll, DWORD SizeOfDll );
  tSymLoadModule64 pSymLoadModule64;

  // SymSetOptions()
  typedef DWORD (__stdcall *tSymSetOptions)( IN DWORD SymOptions );
  tSymSetOptions pSymSetOptions;

  // StackWalk64()
  typedef BOOL (__stdcall *tStackWalk64)( 
    DWORD MachineType, 
    HANDLE hProcess,
    HANDLE hThread, 
    LPSTACKFRAME64 StackFrame, 
    PVOID ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
    PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
    PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
    PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress );
  tStackWalk64 pStackWalk64;

  // UnDecorateSymbolName()
  typedef DWORD (__stdcall WINAPI *tUnDecorateSymbolName)( PCSTR DecoratedName, PSTR UnDecoratedName,
    DWORD UndecoratedLength, DWORD Flags );
  tUnDecorateSymbolName pUnDecorateSymbolName;

  // SymGetSearchPath()
  typedef BOOL (__stdcall WINAPI *tSymGetSearchPath)(HANDLE hProcess, PSTR SearchPath, DWORD SearchPathLength);
  tSymGetSearchPath pSymGetSearchPath;

  StackWalkerInternal(fw::StackWalker *parent, HANDLE process) {
    parent_ = parent;
    _dbghelp = 0;
    _process = process;

    pSymCleanup = 0;
    pSymFunctionTableAccess64 = 0;
    pSymGetLineFromAddr64 = 0;
    pSymGetModuleBase64 = 0;
    pSymGetModuleInfo64 = 0;
    pSymGetOptions = 0;
    pSymGetSymFromAddr64 = 0;
    pSymInitialize = 0;
    pSymLoadModule64 = 0;
    pSymSetOptions = 0;
    pStackWalk64 = 0;
    pUnDecorateSymbolName = 0;
    pSymGetSearchPath = 0;
  }

  ~StackWalkerInternal() {
    if (pSymCleanup != 0)
      pSymCleanup(_process);
    if (_dbghelp != NULL)
      ::FreeLibrary(_dbghelp);
    _dbghelp = 0;
    parent_ = 0;
  }

  bool initialize(char const *symbol_path) {
    if (parent_ == 0)
      return false;

    std::string base_path = Settings::get<std::string>("dbghelp-path");
    if (base_path != "") {
      fs::path path(base_path);
      if (fs::exists(path) && fs::is_directory(path))
        path /= "dbghelp.dll";

      _dbghelp = ::LoadLibrary(path.string().c_str());
    } else {
      // if they haven't explicitly given us a path to dbghelp.dll, try to load
      // it from the default search path...
      _dbghelp = ::LoadLibrary("dbghelp.dll");
    }

    // if we can't load dbghelp.dll, then we can't really do much...
    if (_dbghelp == 0)
      return false;

    // load all of the functions we'll use from dbghelp.dll
    pSymInitialize = (tSymInitialize) ::GetProcAddress(_dbghelp, "SymInitialize" );
    pSymCleanup = (tSymCleanup) ::GetProcAddress(_dbghelp, "SymCleanup" );

    pStackWalk64 = (tStackWalk64) ::GetProcAddress(_dbghelp, "StackWalk64" );
    pSymGetOptions = (tSymGetOptions) ::GetProcAddress(_dbghelp, "SymGetOptions" );
    pSymSetOptions = (tSymSetOptions) ::GetProcAddress(_dbghelp, "SymSetOptions" );

    pSymFunctionTableAccess64 = (tSymFunctionTableAccess64) ::GetProcAddress(_dbghelp, "SymFunctionTableAccess64" );
    pSymGetLineFromAddr64 = (tSymGetLineFromAddr64) ::GetProcAddress(_dbghelp, "SymGetLineFromAddr64" );
    pSymGetModuleBase64 = (tSymGetModuleBase64) ::GetProcAddress(_dbghelp, "SymGetModuleBase64" );
    pSymGetModuleInfo64 = (tSymGetModuleInfo64) ::GetProcAddress(_dbghelp, "SymGetModuleInfo64" );
    pSymGetSymFromAddr64 = (tSymGetSymFromAddr64) ::GetProcAddress(_dbghelp, "SymGetSymFromAddr64" );
    pUnDecorateSymbolName = (tUnDecorateSymbolName) ::GetProcAddress(_dbghelp, "UnDecorateSymbolName" );
    pSymLoadModule64 = (tSymLoadModule64) ::GetProcAddress(_dbghelp, "SymLoadModule64" );
    pSymGetSearchPath =(tSymGetSearchPath) ::GetProcAddress(_dbghelp, "SymGetSearchPath" );

    if (pSymCleanup == 0 || pSymFunctionTableAccess64 == 0 || pSymGetModuleBase64 == 0 || pSymGetModuleInfo64 == 0 ||
      pSymGetOptions == 0 || pSymGetSymFromAddr64 == 0 || pSymInitialize == 0 || pSymSetOptions == 0 ||
      pStackWalk64 == 0 || pUnDecorateSymbolName == 0 || pSymLoadModule64 == 0 ) {
      // if one (or more) of the functions don't exist, we can't use this dbghelp.dll...
      ::FreeLibrary(_dbghelp);
      _dbghelp = NULL;
      pSymCleanup = NULL;
      return false;
    }

    if (symbol_path != 0)
      _symbol_path = fs::path(symbol_path);
    if (pSymInitialize(_process, symbol_path, FALSE) == FALSE)
      parent_->on_dbghelp_error("SymInitialize", ::GetLastError(), 0);

    DWORD options = pSymGetOptions();
    options |= SYMOPT_LOAD_LINES;
    options |= SYMOPT_FAIL_CRITICAL_ERRORS;
    //options |= SYMOPT_NO_PROMPTS;
    options &= ~SYMOPT_UNDNAME;
    options &= ~SYMOPT_DEFERRED_LOADS;
    options = pSymSetOptions(options);

    char buf[StackWalker::STACKWALK_MAX_NAMELEN] = {0};
    if (this->pSymGetSearchPath != 0) {
      if (pSymGetSearchPath(_process, buf, StackWalker::STACKWALK_MAX_NAMELEN) == FALSE) {
        parent_->on_dbghelp_error("SymGetSearchPath", ::GetLastError(), 0);
      }
    }

    char user_name[1024] = {0};
    DWORD size = 1024;
    ::GetUserNameA(user_name, &size);
    parent_->on_sym_init(buf, options, user_name);

    return true;
  }

  HMODULE _dbghelp;

private:
  StackWalker *parent_;
  HANDLE _process;
  fs::path _symbol_path;

  bool get_module_list(HANDLE process) {
    DWORD needed;
    std::vector<HMODULE> modules(8096);
    if (!::EnumProcessModules(process, &modules[0], modules.size() * sizeof(HMODULE), &needed)) {
      //_ftprintf(fLogFile, _T("%lu: EPM failed, GetLastError = %lu\n"), g_dwShowCount, gle );
      return false;
    }

    int num_modules = needed / sizeof(HMODULE);
    if (num_modules > static_cast<int>(modules.size())) {
      //_ftprintf(fLogFile, _T("%lu: More than %lu module handles. Huh?\n"), g_dwShowCount, lenof( hMods ) );
      return false;
    }

    std::vector<char> file_name(4096);
    std::vector<char> module_name(4096);
    for(int i = 0; i < num_modules; i++) {
      HMODULE mod = modules[i];

      // base address, size
      MODULEINFO mi;
      ::GetModuleInformation(process, mod, &mi, sizeof(MODULEINFO));

      // get the module file name and base name
      ::GetModuleFileName(mod, &file_name[0], file_name.size());
      ::GetModuleBaseName(process, mod, &module_name[0], module_name.size());

      DWORD err = load_module(process, &file_name[0], &module_name[0], (uint64_t) mi.lpBaseOfDll, mi.SizeOfImage);
      if (err != ERROR_SUCCESS) {
        parent_->on_dbghelp_error("LoadModule", err, 0);
      }
    }

    return true;
  }

  DWORD load_module(
      HANDLE process, char const *file_name, char const *module_name, uint64_t base_addr, DWORD size) {
    DWORD result = ERROR_SUCCESS;

    if (pSymLoadModule64(process, 0, file_name, module_name, static_cast<DWORD64>(base_addr), size) == 0) {
      result = ::GetLastError();
    }

    uint64_t file_version = 0;
    if ((parent_->options_ & StackWalker::retrieve_file_version) != 0) {
      VS_FIXEDFILEINFO *ff_info = 0;
      DWORD handle;
      DWORD size = ::GetFileVersionInfoSize(file_name, &handle);
      if (size > 0) {
        std::vector<char> buffer(size);
        if (::GetFileVersionInfo(file_name, handle, size, reinterpret_cast<void *>(&buffer[0])) != 0) {
          UINT len;
          if (::VerQueryValue(reinterpret_cast<void *>(&buffer[0]), "\\", reinterpret_cast<void **>(&ff_info), &len)
              != 0) {
            file_version = ((uint64_t)ff_info->dwFileVersionLS) + ((uint64_t)ff_info->dwFileVersionMS << 32);
          }
        }
      }
    }

    // Retrive some additional-infos about the module
    IMAGEHLP_MODULE64_V2 module_info;
    const char *symbol_type = "-unknown-";
    if (get_module_info(process, base_addr, &module_info)) {
      switch(module_info.SymType) {
        case SymNone:
        symbol_type = "-nosymbols-";
        break;
        case SymCoff:
        symbol_type = "COFF";
        break;
        case SymCv:
        symbol_type = "CV";
        break;
        case SymPdb:
        symbol_type = "PDB";
        break;
        case SymExport:
        symbol_type = "-exported-";
        break;
        case SymDeferred:
        symbol_type = "-deferred-";
        break;
        case SymSym:
        symbol_type = "SYM";
        break;
  #if API_VERSION_NUMBER >= 9
        case SymDia:
        symbol_type = "DIA";
        break;
  #endif
        case 8: //SymVirtual:
        symbol_type = "Virtual";
        break;
      }
    }

    parent_->on_load_module(
      file_name, module_name, base_addr, size, result, symbol_type, module_info.LoadedImageName, file_version);
    return result;
  }

public:
  bool load_modules(HANDLE process, DWORD /*process_id*/) {
    return get_module_list(process);
  }

  bool get_module_info(HANDLE process, uint64_t base_addr, IMAGEHLP_MODULE64_V2 *module_info) {
    if(pSymGetModuleInfo64 == NULL) {
      ::SetLastError(ERROR_DLL_INIT_FAILED);
      return false;
    }

    module_info->SizeOfStruct = sizeof(IMAGEHLP_MODULE64_V2);

    std::vector<char> buffer(4096); // reserve lots of memory, so the bug in v6.3.5.1 does not lead to memory-overwrites
    memcpy(&buffer[0], module_info, sizeof(IMAGEHLP_MODULE64_V2));

    if (pSymGetModuleInfo64(process, static_cast<DWORD64>(base_addr),
        reinterpret_cast<IMAGEHLP_MODULE64_V2 *>(&buffer[0])) != FALSE) {
      // only copy as much memory as is reserved...
      memcpy(module_info, &buffer[0], sizeof(IMAGEHLP_MODULE64_V2));
      module_info->SizeOfStruct = sizeof(IMAGEHLP_MODULE64_V2);
      return true;
    }

    SetLastError(ERROR_DLL_INIT_FAILED);
    return false;
  }
};

} // namespace detail

// ---------------------------------------------------------------------------

StackWalker::StackWalker(DWORD process_id, HANDLE process) {
  options_ = options_all;
  _modules_loaded = false;
  _process = process;
  _process_id = process_id;

  std::shared_ptr<detail::StackWalkerInternal> sw_internal(new detail::StackWalkerInternal(this, process));
  _sw = sw_internal;
}

StackWalker::StackWalker(int options, char const *symbol_path, DWORD process_id, HANDLE process) {
  options_ = options;
  _modules_loaded = false;
  _process = process;
  _process_id = process_id;

  std::shared_ptr<detail::StackWalkerInternal> sw_internal(new detail::StackWalkerInternal(this, process));
  _sw = sw_internal;

  if (symbol_path != 0) {
    _symbol_path = symbol_path;
    options_ |= sym_build_path;
  }
}

StackWalker::~StackWalker() {
}

bool StackWalker::load_modules() {
  if (_modules_loaded)
    return true;

  // build the symbol-path:
  std::vector<char> symbol_path;
  if ((options_ & sym_build_path) != 0) {
    symbol_path.resize(4096);
    symbol_path[0] = 0;

    // first add the (optional) provided sympath:
    if (_symbol_path != "") {
      strcat_s(&symbol_path[0], symbol_path.size(), _symbol_path.c_str());
      strcat_s(&symbol_path[0], symbol_path.size(), ";");
    }

    // add the current directory
    strcat_s(&symbol_path[0], symbol_path.size(), ".;");

    std::vector<char> tmp(1024);
    if (::GetEnvironmentVariable("_NT_SYMBOL_PATH", &tmp[0], tmp.size()) > 0) {
      strcat_s(&symbol_path[0], symbol_path.size(), &tmp[0]);
      strcat_s(&symbol_path[0], symbol_path.size(), ";");
    }

    if (::GetEnvironmentVariable("_NT_ALTERNATE_SYMBOL_PATH", &tmp[0], tmp.size()) > 0) {
      strcat_s(&symbol_path[0], symbol_path.size(), &tmp[0]);
      strcat_s(&symbol_path[0], symbol_path.size(), ";");
    }

    if (GetEnvironmentVariable("SYSTEMROOT", &tmp[0], tmp.size()) > 0) {
      strcat_s(&symbol_path[0], symbol_path.size(), &tmp[0]);
      strcat_s(&symbol_path[0], symbol_path.size(), ";");

      // also add the "system32"-directory:
      strcat_s(&tmp[0], tmp.size(), "\\system32");
      strcat_s(&symbol_path[0], symbol_path.size(), &tmp[0]);
      strcat_s(&symbol_path[0], symbol_path.size(), ";");
    }
  }

  bool success = _sw->initialize(&symbol_path[0]);
  if (!success) {
    on_dbghelp_error("Error initializing dbghelp.dll", 0, 0);
    return false;
  }

  success = _sw->load_modules(_process, _process_id);
  _modules_loaded = success;

  return success;
}

// The following is used to pass the "userData"-Pointer to the user-provided readMemoryFunction
// This has to be done due to a problem with the "hProcess"-parameter in x64...
// Because this class is in no case multi-threading-enabled anyway (because of the limitations 
// of dbghelp.dll) it is "safe" to use a static-variable
static StackWalker::PReadProcessMemoryRoutine s_read_memory_function = 0;
static LPVOID s_read_memory_function_UserData = 0;

bool StackWalker::walk_stack(
  HANDLE thread, const CONTEXT *context, PReadProcessMemoryRoutine read_memory_function, void *user_data) {
  if (!_modules_loaded && !load_modules())
    return false;

  if (_sw->_dbghelp == 0)
    return false;

  s_read_memory_function = read_memory_function;
  s_read_memory_function_UserData = user_data;

  CONTEXT c;
  if (context == NULL) {
    // if no context is provided, capture the context
    if (thread == ::GetCurrentThread()) {
      GET_CURRENT_CONTEXT(c, USED_CONTEXT_FLAGS);
    } else {
      ::SuspendThread(thread);
      memset(&c, 0, sizeof(CONTEXT));
      c.ContextFlags = USED_CONTEXT_FLAGS;
      if (::GetThreadContext(thread, &c) == FALSE) {
        ::ResumeThread(thread);
        return false;
      }
    }
  } else {
    c = *context;
  }

  // init STACKFRAME for first call
  STACKFRAME64 stack_frame;
  memset(&stack_frame, 0, sizeof(STACKFRAME64));
  DWORD image_type;

#ifdef _M_IX86
  // normally, call ImageNtHeader() and use machine info from PE header
  image_type = IMAGE_FILE_MACHINE_I386;
  stack_frame.AddrPC.Offset = c.Eip;
  stack_frame.AddrPC.Mode = AddrModeFlat;
  stack_frame.AddrFrame.Offset = c.Ebp;
  stack_frame.AddrFrame.Mode = AddrModeFlat;
  stack_frame.AddrStack.Offset = c.Esp;
  stack_frame.AddrStack.Mode = AddrModeFlat;
#elif _M_X64
  image_type = IMAGE_FILE_MACHINE_AMD64;
  stack_frame.AddrPC.Offset = c.Rip;
  stack_frame.AddrPC.Mode = AddrModeFlat;
  stack_frame.AddrFrame.Offset = c.Rsp;
  stack_frame.AddrFrame.Mode = AddrModeFlat;
  stack_frame.AddrStack.Offset = c.Rsp;
  stack_frame.AddrStack.Mode = AddrModeFlat;
#elif _M_IA64
  image_type = IMAGE_FILE_MACHINE_IA64;
  stack_frame.AddrPC.Offset = c.StIIP;
  stack_frame.AddrPC.Mode = AddrModeFlat;
  stack_frame.AddrFrame.Offset = c.IntSp;
  stack_frame.AddrFrame.Mode = AddrModeFlat;
  stack_frame.AddrBStore.Offset = c.RsBSP;
  stack_frame.AddrBStore.Mode = AddrModeFlat;
  stack_frame.AddrStack.Offset = c.IntSp;
  stack_frame.AddrStack.Mode = AddrModeFlat;
#else
  #error "Platform not supported!"
#endif

  // set up our symbol variable which'll hold a reference to the current symbol
  IMAGEHLP_SYMBOL64 *symbol =
    reinterpret_cast<IMAGEHLP_SYMBOL64 *>(new char[sizeof(IMAGEHLP_SYMBOL64) + STACKWALK_MAX_NAMELEN]);
  memset(symbol, 0, sizeof(IMAGEHLP_SYMBOL64) + STACKWALK_MAX_NAMELEN);
  symbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
  symbol->MaxNameLength = STACKWALK_MAX_NAMELEN;

  IMAGEHLP_LINE64 line;
  memset(&line, 0, sizeof(IMAGEHLP_LINE64));
  line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

  for (int frame_num = 0; ; ++frame_num) {
    // get next stack frame (StackWalk64(), SymFunctionTableAccess64(), SymGetModuleBase64())
    // if this returns ERROR_INVALID_ADDRESS (487) or ERROR_NOACCESS (998), you can
    // assume that either you are done, or that the stack is so hosed that the next
    // deeper frame could not be found.
    // CONTEXT need not to be suplied if image_type is IMAGE_FILE_MACHINE_I386!
    if (!_sw->pStackWalk64(image_type, _process, thread, &stack_frame, &c, my_read_memory_function,
          _sw->pSymFunctionTableAccess64, _sw->pSymGetModuleBase64, 0)) {
      on_dbghelp_error("StackWalk64", ::GetLastError(), stack_frame.AddrPC.Offset);
      break;
    }

    // populate a callstack_entry from all the stuff we got
    callstack_entry entry = {0};
    entry.offset = stack_frame.AddrPC.Offset;
    if (stack_frame.AddrPC.Offset != 0) {
      // we seem to have a valid PC
      if (_sw->pSymGetSymFromAddr64(_process, stack_frame.AddrPC.Offset,
          reinterpret_cast<PDWORD64>(&(entry.offset_from_symbol)), symbol)) {
        // TODO: Mache dies sicher...!
        strcpy_s(entry.name, symbol->Name);
        // UnDecorateSymbolName()
        _sw->pUnDecorateSymbolName(symbol->Name, entry.undecorated_name, STACKWALK_MAX_NAMELEN, UNDNAME_NAME_ONLY);
        _sw->pUnDecorateSymbolName(symbol->Name, entry.undecorated_full_name, STACKWALK_MAX_NAMELEN, UNDNAME_COMPLETE);
      } else {
        on_dbghelp_error("SymGetSymFromAddr64", ::GetLastError(), stack_frame.AddrPC.Offset);
      }

      // show line number info, NT5.0-method (SymGetLineFromAddr64())
      if (_sw->pSymGetLineFromAddr64 != 0) {
        if (_sw->pSymGetLineFromAddr64(_process, stack_frame.AddrPC.Offset,
            reinterpret_cast<PDWORD>(&(entry.offset_from_line)), &line)) {
          entry.line_number = line.LineNumber;
          strcpy_s(entry.line_file_name, line.FileName);
        } else {
          on_dbghelp_error("SymGetLineFromAddr64", GetLastError(), stack_frame.AddrPC.Offset);
        }
      }

      IMAGEHLP_MODULE64_V2 module_info;
      memset(&module_info, 0, sizeof(IMAGEHLP_MODULE64_V2));
      module_info.SizeOfStruct = sizeof(IMAGEHLP_MODULE64_V2);

      // show module info (SymGetModuleInfo64())
      if (_sw->get_module_info(_process, static_cast<uint64_t>(stack_frame.AddrPC.Offset), &module_info)) {
        switch (module_info.SymType) {
        case SymNone:
          entry.symbol_type_string = "-nosymbols-";
          break;
        case SymCoff:
          entry.symbol_type_string = "COFF";
          break;
        case SymCv:
          entry.symbol_type_string = "CV";
          break;
        case SymPdb:
          entry.symbol_type_string = "PDB";
          break;
        case SymExport:
          entry.symbol_type_string = "-exported-";
          break;
        case SymDeferred:
          entry.symbol_type_string = "-deferred-";
          break;
        case SymSym:
          entry.symbol_type_string = "SYM";
          break;
    #if API_VERSION_NUMBER >= 9
        case SymDia:
          entry.symbol_type_string = "DIA";
          break;
    #endif
        case 8: //SymVirtual:
          entry.symbol_type_string = "Virtual";
          break;
        default:
          //_snprintf( ty, sizeof ty, "symtype=%ld", (long) Module.SymType );
          entry.symbol_type_string = 0;
          break;
        }

        strcpy_s(entry.module_name, module_info.ModuleName);
        entry.base_of_image = module_info.BaseOfImage;
        strcpy_s(entry.loaded_image_name, module_info.LoadedImageName);
      } else {
        on_dbghelp_error("SymGetModuleInfo64", GetLastError(), stack_frame.AddrPC.Offset);
      }
    }

    CallstackEntryType et = next_entry;
    if (frame_num == 0)
      et = first_entry;
    on_callstack_entry(et, entry);
      
    if (stack_frame.AddrReturn.Offset == 0) {
      on_callstack_entry(last_entry, entry);
      break;
    }
  }

  delete[] reinterpret_cast<char *>(symbol);

  if (context == 0)
    ::ResumeThread(thread);

  return true;
}

BOOL __stdcall StackWalker::my_read_memory_function(HANDLE hProcess, DWORD64 qwBaseAddress, PVOID lpBuffer,
    DWORD nSize, LPDWORD lpNumberOfBytesRead) {
  if (s_read_memory_function == 0) {
    SIZE_T st;
    BOOL ret = ::ReadProcessMemory(hProcess, (LPVOID) qwBaseAddress, lpBuffer, nSize, &st);
    *lpNumberOfBytesRead = (DWORD) st;
    //printf("ReadMemory: hProcess: %p, baseAddr: %p, buffer: %p, size: %d, read: %d, result: %d\n", hProcess,
    //    (LPVOID) qwBaseAddress, lpBuffer, nSize, (DWORD) st, (DWORD) bRet);
    return ret;
  } else {
    return s_read_memory_function(
        hProcess, qwBaseAddress, lpBuffer, nSize, lpNumberOfBytesRead, s_read_memory_function_UserData);
  }
}

void StackWalker::on_load_module(char const *image, char const *module, uint64_t base_addr, uint32_t size,
    uint32_t result, char const *symbol_type, char const *pdb_name, uint64_t file_version) {
  char buffer[STACKWALK_MAX_NAMELEN];
  if (file_version == 0) {
    sprintf_s(buffer, "%s:%s (%p), size: %d (result: %d), SymType: '%s', PDB: '%s'",
        image, module, (LPVOID) base_addr, size, result, symbol_type, pdb_name);
  } else {
    uint32_t v4 = static_cast<uint32_t>(file_version & 0xFFFF);
    uint32_t v3 = static_cast<uint32_t>((file_version>>16) & 0xFFFF);
    uint32_t v2 = static_cast<uint32_t>((file_version>>32) & 0xFFFF);
    uint32_t v1 = static_cast<uint32_t>((file_version>>48) & 0xFFFF);
    sprintf_s(buffer, "%s:%s (%p), size: %d (result: %d), symbol type: '%s', PDB: '%s', file version: %d.%d.%d.%d",
        image, module, (LPVOID) base_addr, size, result, symbol_type, pdb_name, v1, v2, v3, v4);
  }
  on_output(buffer);
}

void StackWalker::on_callstack_entry(CallstackEntryType entry_type, callstack_entry &entry) {
  char buffer[STACKWALK_MAX_NAMELEN];
  if (entry_type != last_entry && entry.offset != 0) {
    if (entry.name[0] == 0)
      strcpy_s(entry.name, "(function-name not available)");
    if (entry.undecorated_name[0] != 0)
      strcpy_s(entry.name, entry.undecorated_name);
    if (entry.undecorated_full_name[0] != 0)
      strcpy_s(entry.name, entry.undecorated_full_name);

    if (entry.line_file_name[0] == 0) {
      strcpy_s(entry.line_file_name, "(filename not available)");
      if (entry.module_name[0] == 0)
        strcpy_s(entry.module_name, "(module-name not available)");

      sprintf_s(buffer, "%p (%s): %s: %s", (LPVOID) entry.offset, entry.module_name, entry.line_file_name, entry.name);
    } else {
      sprintf_s(buffer, "%s (%d): %s", entry.line_file_name, entry.line_number, entry.name);
    }

    on_output(buffer);
  }
}

void StackWalker::on_dbghelp_error(char const *func_name, uint32_t gle, DWORD64 addr) {
  char buffer[STACKWALK_MAX_NAMELEN];
  sprintf_s(buffer, "ERROR: %s, GetLastError: %d (Address: %p)", func_name, gle, (LPVOID) addr);
  on_output(buffer);
}

void StackWalker::on_sym_init(char const *search_path, uint32_t options, char const *user_name) {
  char buffer[STACKWALK_MAX_NAMELEN];
  sprintf_s(buffer, "SymInit: Symbol-SearchPath: '%s', symOptions: %d, UserName: '%s'",
      search_path, options, user_name);
  on_output(buffer);

  // Also display the OS-version
  OSVERSIONINFOEX ver = {0};
  ver.dwOSVersionInfoSize = sizeof(ver);
  if (GetVersionEx((OSVERSIONINFO *)&ver)) {
    sprintf_s(buffer, STACKWALK_MAX_NAMELEN, "OS-Version: %d.%d.%d (%s) 0x%x-0x%x", 
        ver.dwMajorVersion, ver.dwMinorVersion, ver.dwBuildNumber,
        ver.szCSDVersion, ver.wSuiteMask, ver.wProductType);
    on_output(buffer);
  }
}

void StackWalker::on_output(char const *text) {
  fw::debug << text << std::endl;
}

}