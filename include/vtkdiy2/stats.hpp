#ifndef DIY_STATS_HPP
#define DIY_STATS_HPP

#include <chrono>
#include <string>
#include <vector>
#include <unordered_map>

#include "log.hpp"

#if defined(DIY_USE_CALIPER)
#include <caliper/cali.h>
#include <caliper/common/Variant.h>
#endif

namespace diy
{
namespace stats
{

inline
std::ostream&
operator<<(std::ostream& out, const std::chrono::high_resolution_clock::duration& d)
{
    auto time = std::chrono::duration_cast<std::chrono::microseconds>(d).count();
    fmt::print(out, "{:02d}:{:02d}:{:02d}.{:06d}",
                    time/1000000/60/60,
                    time/1000000/60 % 60,
                    time/1000000 % 60,
                    time % 1000000);
    return out;
}

struct DurationAccumulator
{
    using   Clock    = std::chrono::high_resolution_clock;
    using   Time     = Clock::time_point;
    using   Duration = Clock::duration;

    void    operator<<(std::string name)        { last[name] = Clock::now(); }
    void    operator>>(std::string name)        { duration[name] += Clock::now() - last[name]; }

    void    clear()                             { last.clear(); duration.clear(); }

    std::unordered_map<std::string, Time>       last;
    std::unordered_map<std::string, Duration>   duration;

    void    output(std::ostream& out, std::string prefix = "") const
    {
        if (!prefix.empty())
            prefix += " ";

        for (auto& x : duration)
            out << prefix << x.second << ' ' << x.first << '\n';
    }
};

template<class Profiler>
struct  ScopedProfile
{
        ScopedProfile(Profiler& prof_, std::string name_):
            prof(prof_), name(name_), active(true)  { prof << name; }
        ~ScopedProfile()                            { if (active) prof >> name; }

        ScopedProfile(ScopedProfile&& other):
            prof(other.prof),
            name(other.name),
            active(other.active)                    { other.active = false; }

    ScopedProfile&
        operator=(ScopedProfile&& other) = delete;
        ScopedProfile(const ScopedProfile&) = delete;
    ScopedProfile&
        operator=(const ScopedProfile&) = delete;

    Profiler&   prof;
    std::string name;
    bool        active;
};


#if !defined(DIY_USE_CALIPER)
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
    using   Scoped       = ScopedProfile<Profiler>;

            Profiler()                                  { reset_time(); }

    void    reset_time()                                { start = Clock::now(); }

    void    operator<<(std::string name)                { enter(name); }
    void    operator>>(std::string name)                { exit(name); }

    void    enter(std::string name)                     { events.push_back(Event(name, true));  total << name; }
    void    exit(std::string name)                      { events.push_back(Event(name, false)); total >> name; }

    void    output(std::ostream& out, std::string prefix = "") const
    {
        if (!prefix.empty())
            prefix += " ";

        for (size_t i = 0; i < events.size(); ++i)
        {
            const Event& e = events[i];
            out << prefix << (e.stamp - start) << ' ' << (e.begin ? '<' : '>') <<  e.name << '\n';
        }

        out << "# Total times:\n";
        total.output(out, "# ");
    }

    Scoped  scoped(std::string name)                    { return Scoped(*this, name); }

    void    clear()                                     { events.clear(); total.clear(); }

    const DurationAccumulator& totals() const           { return total; }

    private:
        Time                    start;
        EventsVector            events;
        DurationAccumulator     total;
};
#else   // DIY_PROFILE
struct Profiler
{
    using   Scoped = ScopedProfile<Profiler>;

    void    reset_time()                        {}

    void    operator<<(std::string name)        { enter(name); }
    void    operator>>(std::string name)        { exit(name); }

    void    enter(std::string)                  {}
    void    exit(std::string)                   {}

    void    output(std::ostream& out, std::string = "") const
    {
        out << "# Total times:\n";
        total.output(out, "# ");
    }
    void    clear()                             { total.clear(); }

    Scoped  scoped(std::string name)            { return Scoped(*this, name); }

    const DurationAccumulator&
            totals() const                      { return total; }

    private:
        DurationAccumulator total;
};
#endif  // DIY_PROFILE

// Annotations don't do anything without Caliper
struct Annotation
{
    struct Guard
    {
                    Guard(Annotation&)              {}
    };

                    Annotation(const char*)         {}

    template<class T>
    Annotation&     set(T)                          { return *this; }
};

struct Variant
{
    template<class T>
                    Variant(T)                      {}

};

#else   // DIY_USE_CALIPER

using Annotation = cali::Annotation;
using Variant    = cali::Variant;

struct Profiler
{
    using   Scoped = ScopedProfile<Profiler>;

    void    reset_time()                        {}

    void    operator<<(std::string name)        { enter(name); }
    void    operator>>(std::string name)        { exit(name); }

    void    enter(std::string name)             { CALI_MARK_BEGIN(name.c_str()); }
    void    exit(std::string name)              { CALI_MARK_END(name.c_str()); }

    void    output(std::ostream&, std::string = "") const {}
    void    clear()                             {}

    Scoped  scoped(std::string name)            { return Scoped(*this, name); }

    // unused
    const DurationAccumulator&
            totals() const                      { return total; }

    private:
        DurationAccumulator total;
};
#endif
}
}

#endif
