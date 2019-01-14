// Copyright (c) 2014-2018 Dr. Colin Hirsch and Daniel Frey
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
         ACTION = true,
         NOTHING = false
      };

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
