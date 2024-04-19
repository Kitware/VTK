// Copyright (c) 2017-2020 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_EOL_PAIR_HPP
#define TAO_PEGTL_EOL_PAIR_HPP

#include <cstddef>
#include <utility>

#include "config.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      using eol_pair = std::pair< bool, std::size_t >;

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
