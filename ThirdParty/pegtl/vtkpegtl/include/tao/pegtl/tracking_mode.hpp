// Copyright (c) 2017-2018 Dr. Colin Hirsch and Daniel Frey
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
         IMMEDIATE,
         LAZY
      };

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
