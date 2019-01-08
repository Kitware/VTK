// Copyright (c) 2018 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_INTERNAL_CONDITIONAL_HPP
#define TAO_PEGTL_INTERNAL_CONDITIONAL_HPP

#include "../config.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< bool >
         struct conditional;

         template<>
         struct conditional< true >
         {
            template< typename T, typename >
            using type = T;
         };

         template<>
         struct conditional< false >
         {
            template< typename, typename T >
            using type = T;
         };

      }  // namespace internal

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
