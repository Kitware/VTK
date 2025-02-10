#ifndef DIY_BLOCK_TRAITS_HPP
#define DIY_BLOCK_TRAITS_HPP

#include "traits.hpp"

namespace diy
{
namespace detail
{
    template<class F>
    struct block_traits
    {
        typedef typename std::remove_pointer<typename std::remove_reference<typename function_traits<F>::template arg<0>::type>::type>::type type;
    };

    // matches block member functions
    template<class Block, class R, class... Args>
    struct block_traits<R(Block::*)(Args...)>
    {
        typedef Block type;
    };

    template<class Block, class R, class... Args>
    struct block_traits<R(Block::*)(Args...) const>
    {
        typedef Block type;
    };
}
}

#endif
