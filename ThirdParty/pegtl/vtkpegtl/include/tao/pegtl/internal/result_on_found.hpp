// Copyright (c) 2014-2018 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_INTERNAL_RESULT_ON_FOUND_HPP
#define TAO_PEGTL_INTERNAL_RESULT_ON_FOUND_HPP

#include "../config.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         enum class result_on_found : bool
         {
            SUCCESS = true,
            FAILURE = false
         };

      }  // namespace internal

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
