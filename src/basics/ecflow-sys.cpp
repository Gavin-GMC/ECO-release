#include<filelib.hpp>

#include"ecflow-sys.h"

namespace ECFlow
{
	void dir_create(const std::string& dir)
	{
		FileLib::create_dir(dir, true);
	}

	bool file_exist(const std::string& path)
	{
		return FileLib::file_exist(path);
	}
}