#ifndef DIY_VERTICES_HPP
#define DIY_VERTICES_HPP

#include <iterator>

namespace diy
{

namespace detail
{
    template<class Vertex, size_t I>
    struct IsLast
    {
        static constexpr bool value = (Vertex::dimension() - 1 == I);
    };

    template<class Vertex, class Callback, size_t I, bool P>
    struct ForEach
    {
        void operator()(Vertex& pos, const Vertex& from, const Vertex& to, const Callback& callback) const
        {
            for (pos[I] = from[I]; pos[I] <= to[I]; ++pos[I])
                ForEach<Vertex, Callback, I+1, IsLast<Vertex,I+1>::value>()(pos, from, to, callback);
        }
    };

    template<class Vertex, class Callback, size_t I>
    struct ForEach<Vertex,Callback,I,true>
    {
        void operator()(Vertex& pos, const Vertex& from, const Vertex& to, const Callback& callback) const
        {
            for (pos[I] = from[I]; pos[I] <= to[I]; ++pos[I])
                callback(pos);
        }
    };
}

template<class Vertex, class Callback>
void for_each(const Vertex& from, const Vertex& to, const Callback& callback)
{
    Vertex pos;
    diy::detail::ForEach<Vertex, Callback, 0, detail::IsLast<Vertex,0>::value>()(pos, from, to, callback);
}

template<class Vertex, class Callback>
void for_each(const Vertex& shape, const Callback& callback)
{
    // specify grid namespace to disambiguate with std::for_each(...)
    diy::for_each(Vertex::zero(), shape - Vertex::one(), callback);
}

}

#endif
