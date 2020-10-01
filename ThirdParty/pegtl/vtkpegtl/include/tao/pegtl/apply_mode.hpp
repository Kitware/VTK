// Copyright (c) 2014-2020 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_APPLY_MODE_HPP
#define TAO_PEGTL_APPLY_MODE_HPP

#include "config.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      enum class apply_mode : bool
      {
         action = true,
         nothing = false,

         // Compatibility, remove with 3.0.0
         ACTION = action,
         NOTHING = nothing
      };

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
