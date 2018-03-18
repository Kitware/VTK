// Copyright (c) 2014-2018 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_INTERNAL_IF_MUST_HPP
#define TAO_PEGTL_INTERNAL_IF_MUST_HPP

#include "../config.hpp"

#include "must.hpp"
#include "seq.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename Cond, typename... Thens >
         using if_must = seq< Cond, must< Thens... > >;

      }  // namespace internal

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
