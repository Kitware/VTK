// Copyright (c) 2018 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_INTERNAL_READ_UINT_HPP
#define TAO_PEGTL_INTERNAL_READ_UINT_HPP

#include <cstdint>

#include "../config.hpp"

#include "endian.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         struct read_uint16_be
         {
            using type = std::uint16_t;

            static std::uint16_t read( const void* d ) noexcept
            {
               return be_to_h< std::uint16_t >( d );
            }
         };

         struct read_uint16_le
         {
            using type = std::uint16_t;

            static std::uint16_t read( const void* d ) noexcept
            {
               return le_to_h< std::uint16_t >( d );
            }
         };

         struct read_uint32_be
         {
            using type = std::uint32_t;

            static std::uint32_t read( const void* d ) noexcept
            {
               return be_to_h< std::uint32_t >( d );
            }
         };

         struct read_uint32_le
         {
            using type = std::uint32_t;

            static std::uint32_t read( const void* d ) noexcept
            {
               return le_to_h< std::uint32_t >( d );
            }
         };

         struct read_uint64_be
         {
            using type = std::uint64_t;

            static std::uint64_t read( const void* d ) noexcept
            {
               return be_to_h< std::uint64_t >( d );
            }
         };

         struct read_uint64_le
         {
            using type = std::uint64_t;

            static std::uint64_t read( const void* d ) noexcept
            {
               return le_to_h< std::uint64_t >( d );
            }
         };

      }  // namespace internal

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
