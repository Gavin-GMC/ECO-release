// SPDX-License-Identifier: BSD-3-Clause

#include<filelib.hpp>

#include"evoflow-sys.h"

namespace EvoFlow
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
