// Copyright (c) 2018 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_INTERNAL_THREE_HPP
#define TAO_PEGTL_INTERNAL_THREE_HPP

#include <utility>

#include "../config.hpp"

#include "bump_help.hpp"
#include "result_on_found.hpp"
#include "skip_control.hpp"

#include "../analysis/generic.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< char C >
         struct three
         {
            using analyze_t = analysis::generic< analysis::rule_type::ANY >;

            template< typename Input >
            static bool match( Input& in ) noexcept( noexcept( in.size( 3 ) ) )
            {
               if( in.size( 3 ) >= 3 ) {
                  if( ( in.peek_char( 0 ) == C ) && ( in.peek_char( 1 ) == C ) && ( in.peek_char( 2 ) == C ) ) {
                     bump_help< result_on_found::SUCCESS, Input, char, C >( in, 3 );
                     return true;
                  }
               }
               return false;
            }
         };

         template< char C >
         struct skip_control< three< C > > : std::true_type
         {
         };

      }  // namespace internal

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
