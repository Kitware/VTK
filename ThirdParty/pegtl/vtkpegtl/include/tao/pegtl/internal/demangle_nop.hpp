// Copyright (c) 2014-2018 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_INTERNAL_DEMANGLE_NOP_HPP
#define TAO_PEGTL_INTERNAL_DEMANGLE_NOP_HPP

#include <string>

#include "../config.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         inline std::string demangle( const char* symbol )
         {
            return symbol;
         }

      }  // namespace internal

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
