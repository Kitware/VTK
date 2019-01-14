// Copyright (c) 2014-2018 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_INTERNAL_PEEK_UTF32_HPP
#define TAO_PEGTL_INTERNAL_PEEK_UTF32_HPP

#include <cstddef>

#include "../config.hpp"

#include "input_pair.hpp"
#include "read_uint.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename R >
         struct peek_utf32_impl
         {
            using data_t = char32_t;
            using pair_t = input_pair< char32_t >;

            static_assert( sizeof( char32_t ) == 4, "expected size 4 for 32bit value" );

            template< typename Input >
            static pair_t peek( Input& in ) noexcept( noexcept( in.size( 4 ) ) )
            {
               const std::size_t s = in.size( 4 );
               if( s >= 4 ) {
                  const char32_t t = R::read( in.current() );
                  if( ( 0 <= t ) && ( t <= 0x10ffff ) && !( t >= 0xd800 && t <= 0xdfff ) ) {
                     return { t, 4 };
                  }
               }
               return { 0, 0 };
            }
         };

         using peek_utf32_be = peek_utf32_impl< read_uint32_be >;
         using peek_utf32_le = peek_utf32_impl< read_uint32_le >;

      }  // namespace internal

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
