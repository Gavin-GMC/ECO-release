#ifndef ALG_PATH_H
#define ALG_PATH_H

#if defined(_WIN32) || defined(_WIN64)
#include <direct.h>
#include <io.h>
#include<Windows.h>
#else
#include <sys/io.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#endif 
#include<fstream>

namespace alglib {

    //create the directory if not exist 
    //创建一个文件夹如果该文件夹不存在
    void dir_create(const std::string& dir)
    {
        int flag;

#if defined(WIN32) || defined(_WIN64)
        if (_access(dir.c_str(), 0) == -1) // directory is no exist
        {
            flag = _mkdir(dir.c_str());
            if (flag != 0)
            {
                throw std::runtime_error("Failed to create directory: " + dir);
            }
        }
#else
        if (access(dir.c_str(), 0) == -1) // directory is no exist
        {
            flag = mkdir(dir.c_str(), S_IRWXU);
            if (flag != 0)
            {
                throw std::runtime_error("Failed to create directory: " + dir);
            }
        }
#endif 
    }

    // 查询该文件是否存在
    bool file_exist(const std::string& path)
    {
        std::ifstream f(path);

        return f.good();
    }
}

#endif // !ALG_PATH_H



