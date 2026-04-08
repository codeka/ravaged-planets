#pragma once

#include <map>
#include <stdexcept>
#include <string_view>

#include <absl/strings/str_cat.h>

namespace fw {
	namespace impl {
		std::map<std::string, void*>& GetServiceLocatorMap();
	}

	// Gets the service of the given type with the given name. The service must have been registered
	// with REGISTER_SERVICE.
	template <typename T>
	inline T& Get(std::string_view service_name) {
		auto& service_map = impl::GetServiceLocatorMap();
		auto service = service_map.find(std::string(service_name));
		if (service == service_map.end()) {
			throw std::runtime_error(absl::StrCat("Service not found: ", service_name));
		}

		return *static_cast<T*>(service->second);
	}

	// Gets the service of the given type. The service must have been registered with REGISTER_SERVICE.
	template <typename T>
	inline T& Get() {
		return Get<T>(T::service_name);
	}

#define REGISTER_SERVICE(service_type) \
	struct service_type##Registration { \
    service_type##Registration() { \
				fw::impl::GetServiceLocatorMap()[service_type::service_name] = new service_type(); \
		} \
	}; \
	service_type##Registration service_type##registration;

}
