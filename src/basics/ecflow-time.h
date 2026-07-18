#pragma once
#include<string>

namespace ECFlow
{
	/**
	* @brief 让当前线程休眠指定的秒数
	*
	* 该函数封装了跨平台的休眠接口，使当前执行线程暂停运行指定的秒数，
	* 休眠期间线程不会占用CPU资源，直到指定时间结束或被信号中断（仅Linux下）。
	*
	* @param sec 休眠的秒数，取值范围：≥ 0（若传入负数，函数会默认休眠0秒，无报错）
	* @note 1. 跨平台兼容（Windows/Linux）；2. 休眠精度为秒级，无法精确到毫秒/微秒；
	*       3. Windows下不会被信号中断，Linux下若收到中断信号会提前结束休眠
	*/
	void sys_sleep(int sec);

	/**
	* @brief 获取当前系统时间并格式化为字符串
	*
	* 函数会获取当前系统的本地时间，按照固定格式转换为字符串返回，
	* 便于日志输出、时间戳记录等场景使用。
	*
	* @return std::string 格式化后的时间字符串，默认格式为 "YYYY-MM-DD HH:MM:SS"（可根据实际实现调整）
	* @note 1. 返回的是本地时间（而非UTC时间）；2. 时间格式可根据需求修改，如添加毫秒："YYYY-MM-DD HH:MM:SS.ms"；
	*       3. 若系统时间异常，返回空字符串或错误提示字符串（根据实际实现调整）
	*/
	std::string formatTime();
}
