// Copyright (c) 2014-2020 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_INTERNAL_ANY_HPP
#define TAO_PEGTL_INTERNAL_ANY_HPP

#include "../config.hpp"

#include "peek_char.hpp"
#include "skip_control.hpp"

#include "../analysis/generic.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename Peek >
         struct any;

         template<>
         struct any< peek_char >
         {
            using analyze_t = analysis::generic< analysis::rule_type::any >;

            template< typename Input >
            static bool match( Input& in ) noexcept( noexcept( in.empty() ) )
            {
               if( !in.empty() ) {
                  in.bump();
                  return true;
               }
               return false;
            }
         };

         template< typename Peek >
         struct any
         {
            using analyze_t = analysis::generic< analysis::rule_type::any >;

            template< typename Input >
            static bool match( Input& in ) noexcept( noexcept( Peek::peek( in ) ) )
            {
               if( const auto t = Peek::peek( in ) ) {
                  in.bump( t.size );
                  return true;
               }
               return false;
            }
         };

         template< typename Peek >
         struct skip_control< any< Peek > > : std::true_type
         {
         };

      }  // namespace internal

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
