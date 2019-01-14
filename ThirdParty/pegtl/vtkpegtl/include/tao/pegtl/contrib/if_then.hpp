// Copyright (c) 2018 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_CONTRIB_IF_THEN_HPP
#define TAO_PEGTL_CONTRIB_IF_THEN_HPP

#include <type_traits>

#include "../config.hpp"

#include "../internal/if_then_else.hpp"
#include "../internal/seq.hpp"
#include "../internal/skip_control.hpp"
#include "../internal/trivial.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename Cond, typename Then >
         struct if_pair
         {
         };

         template< typename... Pairs >
         struct if_then;

         template< typename Cond, typename Then, typename... Pairs >
         struct if_then< if_pair< Cond, Then >, Pairs... >
            : if_then_else< Cond, Then, if_then< Pairs... > >
         {
            template< typename ElseCond, typename... Thens >
            using else_if_then = if_then< if_pair< Cond, Then >, Pairs..., if_pair< ElseCond, seq< Thens... > > >;

            template< typename... Thens >
            using else_then = if_then_else< Cond, Then, if_then< Pairs..., if_pair< trivial< true >, seq< Thens... > > > >;
         };

         template<>
         struct if_then<>
            : trivial< false >
         {
         };

         template< typename... Pairs >
         struct skip_control< if_then< Pairs... > > : std::true_type
         {
         };

      }  // namespace internal

      template< typename Cond, typename... Thens >
      using if_then = internal::if_then< internal::if_pair< Cond, internal::seq< Thens... > > >;

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
