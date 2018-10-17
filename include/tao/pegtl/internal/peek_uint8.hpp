// Copyright (c) 2018 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_INTERNAL_PEEK_UINT8_HPP
#define TAO_PEGTL_INTERNAL_PEEK_UINT8_HPP

#include <cstddef>
#include <cstdint>

#include "../config.hpp"

#include "input_pair.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         struct peek_uint8
         {
            using data_t = std::uint8_t;
            using pair_t = input_pair< std::uint8_t >;

            template< typename Input >
            static pair_t peek( Input& in, const std::size_t o = 0 ) noexcept( noexcept( in.peek_byte( 0 ) ) )
            {
               return { in.peek_byte( o ), 1 };
            }
         };

      }  // namespace internal

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
