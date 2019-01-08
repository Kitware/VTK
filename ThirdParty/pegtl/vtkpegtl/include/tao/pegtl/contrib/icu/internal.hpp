// Copyright (c) 2018 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_CONTRIB_ICU_INTERNAL_HPP
#define TAO_PEGTL_CONTRIB_ICU_INTERNAL_HPP

#include <unicode/uchar.h>

#include "../../config.hpp"

#include "../../analysis/generic.hpp"
#include "../../internal/skip_control.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         namespace icu
         {
            template< typename Peek, UProperty P, bool V = true >
            struct binary_property
            {
               using analyze_t = analysis::generic< analysis::rule_type::ANY >;

               template< typename Input >
               static bool match( Input& in ) noexcept( noexcept( Peek::peek( in ) ) )
               {
                  if( const auto r = Peek::peek( in ) ) {
                     if( u_hasBinaryProperty( r.data, P ) == V ) {
                        in.bump( r.size );
                        return true;
                     }
                  }
                  return false;
               }
            };

            template< typename Peek, UProperty P, int V >
            struct property_value
            {
               using analyze_t = analysis::generic< analysis::rule_type::ANY >;

               template< typename Input >
               static bool match( Input& in ) noexcept( noexcept( Peek::peek( in ) ) )
               {
                  if( const auto r = Peek::peek( in ) ) {
                     if( u_getIntPropertyValue( r.data, P ) == V ) {
                        in.bump( r.size );
                        return true;
                     }
                  }
                  return false;
               }
            };

         }  // namespace icu

         template< typename Peek, UProperty P, bool V >
         struct skip_control< icu::binary_property< Peek, P, V > > : std::true_type
         {
         };

         template< typename Peek, UProperty P, int V >
         struct skip_control< icu::property_value< Peek, P, V > > : std::true_type
         {
         };

      }  // namespace internal

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
