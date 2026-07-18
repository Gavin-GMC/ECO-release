#pragma once
#include<string>

namespace ECFlow
{
	/**
	* @brief 创建指定路径的目录（支持递归创建多级目录）
	*
	* 该函数用于创建指定的目录路径，如果目录已存在则不会报错；
	* 如果路径包含多级不存在的目录（如"a/b/c"），会递归创建所有层级的目录。
	*
	* @param dir 待创建的目录路径（支持绝对路径/相对路径）
	* @note 1. 路径分隔符兼容Windows（\）和Linux（/）；2. 若目录已存在，函数无操作且不抛出异常
	*/
	void dir_create(const std::string& dir);

	/**
	* @brief 检查指定路径的文件/目录是否存在
	*
	* 该函数可同时检查文件或目录的存在性，不区分文件类型，仅判断路径是否有效且存在。
	*
	* @param path 待检查的文件/目录路径（支持绝对路径/相对路径）
	* @return bool 存在返回true，不存在/权限不足/路径非法返回false
	* @note 1. 路径分隔符兼容Windows（\）和Linux（/）；2. 若路径权限不足导致无法访问，也返回false
	*/
	bool file_exist(const std::string& path);
 }
