
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#define SECURITY_WIN32
#include <Security.h>

#pragma comment( lib, "Secur32.lib" )

#include <framework/misc.h>

namespace fw {

/**
 * Attempt to get the current user's first name/last name from the passwd file. If we can't get it, then just return
 * the username anyway.
 */
std::string get_user_name() {
  char buffer[1000];
  ULONG buffer_size = sizeof(buffer);

  if (::GetUserNameEx(NameDisplay, buffer, &buffer_size)) {
    return buffer;
  } else {
    ::GetEnvironmentVariable("USERNAME", buffer, buffer_size);
    return buffer;
  }
  return 0;
}

}
