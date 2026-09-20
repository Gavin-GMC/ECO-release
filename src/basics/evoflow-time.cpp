// SPDX-License-Identifier: BSD-3-Clause

#include<timelib.hpp>

#include"evoflow-time.h"

namespace EvoFlow
{
	void sys_sleep(int sec)
	{
		timelib::sleep_ms(1000 * sec);
	}

	std::string formatTime()
	{
		return timelib::get_local_time_str();
	}
}
