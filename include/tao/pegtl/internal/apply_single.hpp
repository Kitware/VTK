// Copyright (c) 2017-2018 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_INTERNAL_APPLY_SINGLE_HPP
#define TAO_PEGTL_INTERNAL_APPLY_SINGLE_HPP

#include "../config.hpp"

#include <type_traits>

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename Action >
         struct apply_single
         {
            template< typename Input, typename... States >
            static auto match( const Input& i2, States&&... st )
               -> typename std::enable_if< std::is_same< decltype( Action::apply( i2, st... ) ), void >::value, bool >::type
            {
               Action::apply( i2, st... );
               return true;
            }

            template< typename Input, typename... States >
            static auto match( const Input& i2, States&&... st )
               -> typename std::enable_if< std::is_same< decltype( Action::apply( i2, st... ) ), bool >::value, bool >::type
            {
               return Action::apply( i2, st... );
            }
         };

      }  // namespace internal

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
