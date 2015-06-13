
#include <pwd.h>
#include <unistd.h>
#include <sys/types.h>

#include <framework/misc.h>

namespace fw {

/**
 * Attempt to get the current user's first name/last name from the passwd file. If we can't get it, then just return
 * the username anyway.
 */
std::string get_user_name() {
  struct passwd *entry = getpwuid(geteuid());
  if (entry != nullptr) {
    if (entry->pw_gecos != nullptr) {
      // pw_gecos is a comma-separated list of values, we only care about the first one.
      std::string gecos = entry->pw_gecos;
      int comma = gecos.find(',');
      if (comma > 0) {
        return gecos.substr(0, comma);
      }
      return gecos;
    }
    return entry->pw_name;
  }

  return getenv("USER");
}

}
