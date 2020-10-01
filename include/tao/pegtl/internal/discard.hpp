// Copyright (c) 2016-2020 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_INTERNAL_DISCARD_HPP
#define TAO_PEGTL_INTERNAL_DISCARD_HPP

#include "../config.hpp"

#include "skip_control.hpp"

#include "../analysis/generic.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         struct discard
         {
            using analyze_t = analysis::generic< analysis::rule_type::opt >;

            template< typename Input >
            static bool match( Input& in ) noexcept
            {
               static_assert( noexcept( in.discard() ), "an input's discard()-method must be noexcept" );
               in.discard();
               return true;
            }
         };

         template<>
         struct skip_control< discard > : std::true_type
         {
         };

      }  // namespace internal

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
