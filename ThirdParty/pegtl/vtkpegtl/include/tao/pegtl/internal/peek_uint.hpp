// Copyright (c) 2018-2020 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_INTERNAL_PEEK_UINT_HPP
#define TAO_PEGTL_INTERNAL_PEEK_UINT_HPP

#include <cstddef>
#include <cstdint>

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
         struct peek_uint_impl
         {
            using data_t = typename R::type;
            using pair_t = input_pair< data_t >;

            template< typename Input >
            static pair_t peek( Input& in ) noexcept( noexcept( in.size( sizeof( data_t ) ) ) )
            {
               if( in.size( sizeof( data_t ) ) < sizeof( data_t ) ) {
                  return { 0, 0 };
               }
               const data_t data = R::read( in.current() );
               return { data, sizeof( data_t ) };
            }
         };

         using peek_uint16_be = peek_uint_impl< read_uint16_be >;
         using peek_uint16_le = peek_uint_impl< read_uint16_le >;

         using peek_uint32_be = peek_uint_impl< read_uint32_be >;
         using peek_uint32_le = peek_uint_impl< read_uint32_le >;

         using peek_uint64_be = peek_uint_impl< read_uint64_be >;
         using peek_uint64_le = peek_uint_impl< read_uint64_le >;

      }  // namespace internal

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
