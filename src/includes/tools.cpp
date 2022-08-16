#include "includes.h"

#include "tools.h"

#include <boost/filesystem.hpp>

#ifdef _MSC_VER

#include <winsock2.h>
#include <windows.h>

#pragma warning (push)
#pragma warning (disable:4091) // warning C4091: 'typedef ': ignored on left of '' when no variable is declared
#include <dbghelp.h>
#pragma warning (pop)

#else
#include <cxxabi.h>
#include <cstdlib>
#endif

std::string demangle_name(const char* name)
{
	const unsigned BufferSize = 1024;
	char Buffer[BufferSize] = {};

#ifdef _MSC_VER
	int written = UnDecorateSymbolName(name, Buffer, BufferSize - 1, UNDNAME_COMPLETE);
	Buffer[written] = '\0';
#else
	size_t len;
	int status;
	char* demangled = abi::__cxa_demangle(name, 0, &len, &status);

	if (demangled) {
		std::string ret(demangled);
		Buffer[BufferSize - 1] = '\0';
		free(demangled);
	}
	else {
		Buffer[0] = '\0';
	}
#endif
	return Buffer;

}

int64_t getFileTime(const std::string& path)
{
	if (!boost::filesystem::exists(path))
		return -1;

	std::time_t ftime = boost::filesystem::last_write_time(path);
	return ftime;
}

std::vector<std::string> getFiles(const std::string& path, const std::string& extension)
{
	std::vector<std::string> files;
	
	namespace fs = boost::filesystem;

	auto current = fs::current_path();
	const auto dir = current / "project" / path;
	if (!fs::exists(dir) || !fs::is_directory(dir)) {
		std::cout << "[Warning - getFiles] Can not load folder '" << path << "'." << std::endl;
		return files;
	}

	fs::recursive_directory_iterator endit;
	std::vector<fs::path> v;
	for (fs::recursive_directory_iterator it(dir); it != endit; ++it) {
		auto obj = it->path();
		if (!fs::is_directory(obj) && obj.extension().string() == extension) {
			const std::string& relative_name = obj.relative_path().string();
			const char* file_name = relative_name.c_str() + current.string().length() - 2;

			files.emplace_back(file_name);
		}
	}

	return files;
}
