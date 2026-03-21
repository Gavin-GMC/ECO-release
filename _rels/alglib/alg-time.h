#ifndef ALG_TIME_H
#define ALG_TIME_H

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

#include<string>
#include<sstream>
#include<iomanip>

namespace alglib {
    //system environment initialization
//系统睡眠时长
    void sys_sleep(int sec)
    {
#if defined(WIN32) || defined(_WIN64)
        Sleep(sec * 1000);
#else
        sleep(sec);
#endif 
    }

    std::string formatTime()
    {
        time_t now = time(NULL);
        tm tm_now;

#if defined(WIN32) || defined(_WIN64)
        localtime_s(&tm_now, &now);
#else
        localtime_r(&now, &tm_now);
#endif 

        std::stringstream ss;
        ss << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S");

        return ss.str();
    }
}

#endif // !ALG_TIME_H