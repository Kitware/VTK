// Copyright (c) 2017-2020 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_TRACKING_MODE_HPP
#define TAO_PEGTL_TRACKING_MODE_HPP

#include "config.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      enum class tracking_mode : bool
      {
         eager,
         lazy,

         // Compatibility, remove with 3.0.0
         IMMEDIATE = eager,
         LAZY = lazy
      };

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
