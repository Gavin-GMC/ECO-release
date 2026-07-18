#include<timelib.hpp>

#include"ecflow-time.h"

namespace ECFlow
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