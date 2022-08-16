#ifndef __TOOLS__
#define __TOOLS__

// Demangle names for GNU g++ compiler
std::string demangle_name(const char* name);

// Returns the name of a type
template<typename T>
std::string demangle_type() {
#ifdef _MSC_VER
	return demangle_name(typeid(T).name() + 6);
#else
	return demangle_name(typeid(T).name());
#endif
}

int64_t getFileTime(const std::string& path);

std::vector<std::string> getFiles(const std::string& path, const std::string& extension);

#endif
