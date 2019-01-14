// Copyright (c) 2017-2018 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_INTERNAL_HAS_APPLY0_HPP
#define TAO_PEGTL_INTERNAL_HAS_APPLY0_HPP

#include <type_traits>

#include "../config.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename, typename, typename... >
         struct has_apply0 : std::false_type
         {
         };

         template< typename A, typename... S >
         struct has_apply0< A, decltype( A::apply0( std::declval< S >()... ) ), S... > : std::true_type
         {
         };

      }  // namespace internal

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
