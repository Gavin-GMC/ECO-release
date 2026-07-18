#pragma once
#include <cstdint>
#include <string>
#include <ctime>

// 跨平台宏定义：区分Windows和POSIX系统（Linux/macOS/BSD等）
#ifdef _WIN32
#include <windows.h>
// Windows下缺少一些POSIX类型定义，补充
using suseconds_t = long;
#else
#include <unistd.h>
#include <errno.h>
#endif

namespace timelib {

    /**
     * @brief 睡眠指定毫秒数（跨平台）
     * @param ms 要睡眠的毫秒数，范围：0 ~ UINT64_MAX
     */
    inline void sleep_ms(uint64_t ms) {
#ifdef _WIN32
        // Windows API：Sleep接收毫秒数（无符号32位），超过范围则分段睡眠
        if (ms > UINT32_MAX) {
            uint32_t sleep_part = UINT32_MAX;
            while (ms > sleep_part) {
                Sleep(sleep_part);
                ms -= sleep_part;
            }
        }
        Sleep(static_cast<DWORD>(ms));
#else
        // POSIX系统：使用nanosleep（比usleep更推荐，usleep已被废弃）
        struct timespec req {};
        req.tv_sec = static_cast<time_t>(ms / 1000);          // 秒部分
        req.tv_nsec = static_cast<long>((ms % 1000) * 1000000); // 纳秒部分（1毫秒=1e6纳秒）

        // 处理nanosleep可能被信号中断的情况
        while (nanosleep(&req, &req) == -1 && errno == EINTR);
#endif
    }

    /**
     * @brief 睡眠指定微秒数（跨平台）
     * @param us 要睡眠的微秒数，范围：0 ~ UINT64_MAX
     */
    inline void sleep_us(uint64_t us) {
#ifdef _WIN32
        // Windows无直接微秒睡眠API，使用高精度计时器实现
        if (us == 0) return;

        LARGE_INTEGER freq, start, end;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&start);

        // 计算需要等待的计时器周期数
        uint64_t target_cycles = (static_cast<uint64_t>(freq.QuadPart) * us) / 1000000;

        do {
            QueryPerformanceCounter(&end);
        } while ((static_cast<uint64_t>(end.QuadPart) - static_cast<uint64_t>(start.QuadPart)) < target_cycles);
#else
        struct timespec req {};
        req.tv_sec = static_cast<time_t>(us / 1000000);        // 秒部分
        req.tv_nsec = static_cast<long>((us % 1000000) * 1000); // 纳秒部分（1微秒=1e3纳秒）

        while (nanosleep(&req, &req) == -1 && errno == EINTR);
#endif
    }

    /**
     * @brief 获取当前时间戳（毫秒级，从1970-01-01 00:00:00 UTC开始）
     * @return 毫秒级时间戳（uint64_t）
     */
    inline uint64_t get_timestamp_ms() {
#ifdef _WIN32
        // Windows：使用FileTime转换（避免时区问题）
        FILETIME ft;
        GetSystemTimeAsFileTime(&ft);

        // FileTime以100纳秒为单位，转换为毫秒
        uint64_t timestamp = (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
        timestamp /= 10000; // 100纳秒 = 0.1微秒，/10000得到毫秒
        timestamp -= 11644473600000ULL; // 减去1970-01-01到1601-01-01的毫秒数（Windows epoch偏移）
        return timestamp;
#else
        // POSIX系统：使用clock_gettime（CLOCK_REALTIME为系统实时时间）
        struct timespec ts {};
        clock_gettime(CLOCK_REALTIME, &ts);
        return static_cast<uint64_t>(ts.tv_sec) * 1000 + ts.tv_nsec / 1000000;
#endif
    }

    /**
     * @brief 获取当前时间戳（微秒级，从1970-01-01 00:00:00 UTC开始）
     * @return 微秒级时间戳（uint64_t）
     */
    inline uint64_t get_timestamp_us() {
#ifdef _WIN32
        FILETIME ft;
        GetSystemTimeAsFileTime(&ft);

        uint64_t timestamp = (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
        timestamp /= 10; // 100纳秒 = 0.1微秒，/10得到微秒
        timestamp -= 11644473600000000ULL; // 偏移调整
        return timestamp;
#else
        struct timespec ts {};
        clock_gettime(CLOCK_REALTIME, &ts);
        return static_cast<uint64_t>(ts.tv_sec) * 1000000 + ts.tv_nsec / 1000;
#endif
    }

    /**
     * @brief 获取本地时间的格式化字符串（YYYY-MM-DD HH:MM:SS）
     * @return 格式化的本地时间字符串
     */
    inline std::string get_local_time_str() {
        time_t now = time(nullptr);
        struct tm local_tm {};
#ifdef _WIN32
        localtime_s(&local_tm, &now); // Windows安全版本
#else
        localtime_r(&now, &local_tm); // POSIX安全版本
#endif

        char buf[64] = { 0 };
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &local_tm);
        return std::string(buf);
    }

    /**
     * @brief 获取UTC时间的格式化字符串（YYYY-MM-DD HH:MM:SS）
     * @return 格式化的UTC时间字符串
     */
    inline std::string get_utc_time_str() {
        time_t now = time(nullptr);
        struct tm utc_tm {};
#ifdef _WIN32
        gmtime_s(&utc_tm, &now); // Windows安全版本
#else
        gmtime_r(&now, &utc_tm); // POSIX安全版本
#endif

        char buf[64] = { 0 };
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &utc_tm);
        return std::string(buf);
    }

} // namespace timelib