#include <framework/service_locator.h>

namespace fw::impl {

std::map<std::string, void*>& GetServiceLocatorMap() {
	static std::map<std::string, void*> service_map;
	return service_map;
}

}
