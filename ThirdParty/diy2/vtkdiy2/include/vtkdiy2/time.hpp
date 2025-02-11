#ifndef DIY_TIME_HPP
#define DIY_TIME_HPP

#ifndef _WIN32
#include <sys/time.h>
#if defined(__MACH__) && defined(__APPLE__)
#include <mach/clock.h>
#include <mach/mach.h>
#endif // __MACH__ && __APPLE__
#endif // ifndef _WIN32

namespace diy
{

typedef     unsigned long       time_type;

inline time_type get_time()
{
#if defined(__MACH__) && defined(__APPLE__) // OS X does not have clock_gettime, use clock_get_time
    clock_serv_t cclock;
    mach_timespec_t ts;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &ts);
    mach_port_deallocate(mach_task_self(), cclock);
    return ts.tv_sec*1000 + static_cast<unsigned int>(ts.tv_nsec/1000000);
#elif defined(_WIN32)
    // SOURCE: http://stackoverflow.com/questions/5404277/porting-clock-gettime-to-windows
    __int64 wintime;
    GetSystemTimeAsFileTime((FILETIME*)&wintime);
    wintime      -=116444736000000000LL;  //1jan1601 to 1jan1970
    long tv_sec  = static_cast<long>(wintime / 10000000LL);           //seconds
    long tv_nsec = static_cast<long>(wintime % 10000000LL *100);      //nano-seconds
    return static_cast<time_type>(tv_sec*1000 + tv_nsec/1000000);
#else
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec*1000 + ts.tv_nsec/1000000;
#endif
}

}

#endif
