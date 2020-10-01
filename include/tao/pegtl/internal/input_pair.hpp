// Copyright (c) 2014-2020 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_INTERNAL_INPUT_PAIR_HPP
#define TAO_PEGTL_INTERNAL_INPUT_PAIR_HPP

#include <cstdint>

#include "../config.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename Data >
         struct input_pair
         {
            Data data;
            std::uint8_t size;

            using data_t = Data;

            explicit operator bool() const noexcept
            {
               return size > 0;
            }
         };

      }  // namespace internal

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
