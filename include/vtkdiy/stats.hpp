#ifndef DIY_STATS_HPP
#define DIY_STATS_HPP

#include <chrono>
#include <string>
#include <vector>

#include "log.hpp"      // need this for format

namespace diy
{
namespace stats
{

#if defined(DIY_PROFILE)
struct Profiler
{
    using   Clock = std::chrono::high_resolution_clock;
    using   Time  = Clock::time_point;

    struct Event
    {
            Event(const std::string& name_, bool begin_):
                name(name_),
                begin(begin_),
                stamp(Clock::now())
                                                        {}

        std::string     name;
        bool            begin;
        Time            stamp;
    };

    using   EventsVector = std::vector<Event>;

    struct  Scoped
    {
            Scoped(Profiler& prof_, std::string name_):
                prof(prof_), name(name_), active(true)  { prof << name; }
            ~Scoped()                                   { if (active) prof >> name; }

            Scoped(Scoped&& other):
                prof(other.prof),
                name(other.name),
                active(other.active)                    { other.active = false; }

        Scoped&
            operator=(Scoped&& other) = delete;
            Scoped(const Scoped&) = delete;
        Scoped&
            operator=(const Scoped&) = delete;

        Profiler&   prof;
        std::string name;
        bool        active;
    };

            Profiler()                                  { reset_time(); }

    void    reset_time()                                { start = Clock::now(); }

    void    operator<<(std::string name)                { enter(name); }
    void    operator>>(std::string name)                { exit(name); }

    void    enter(std::string name)                     { events.push_back(Event(name, true)); }
    void    exit(std::string name)                      { events.push_back(Event(name, false)); }

    void    output(std::ostream& out)
    {
        for (size_t i = 0; i < events.size(); ++i)
        {
            const Event& e = events[i];
            auto time = std::chrono::duration_cast<std::chrono::microseconds>(e.stamp - start).count();

            fmt::print(out, "{:02d}:{:02d}:{:02d}.{:06d} {}{}\n",
                            time/1000000/60/60,
                            time/1000000/60 % 60,
                            time/1000000 % 60,
                            time % 1000000,
                            (e.begin ? '<' : '>'),
                            e.name);
        }
    }

    Scoped  scoped(std::string name)                    { return Scoped(*this, name); }

    void    clear()                                     { events.clear(); }

    private:
        Time            start;
        EventsVector    events;
};
#else
struct Profiler
{
    struct Scoped {};

    void    reset_time()                                {}

    void    operator<<(std::string)                     {}
    void    operator>>(std::string)                     {}

    void    enter(const std::string&)                   {}
    void    exit(const std::string&)                    {}

    void    output(std::ostream&)                       {}
    void    clear()                                     {}

    Scoped  scoped(std::string)                         { return Scoped(); }
};
#endif
}
}

#endif
