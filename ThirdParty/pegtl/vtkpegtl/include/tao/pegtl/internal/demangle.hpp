// Copyright (c) 2014-2018 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_INTERNAL_DEMANGLE_HPP
#define TAO_PEGTL_INTERNAL_DEMANGLE_HPP

#include <string>
#include <typeinfo>

#include "../config.hpp"

#if defined( __GLIBCXX__ )
#include "demangle_cxxabi.hpp"
#elif defined( __has_include )
#if __has_include( <cxxabi.h> )
#include "demangle_cxxabi.hpp"
#else
#include "demangle_nop.hpp"
#endif
#else
#include "demangle_nop.hpp"
#endif

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename T >
         std::string demangle()
         {
            return demangle( typeid( T ).name() );
         }

      }  // namespace internal

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
