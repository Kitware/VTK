// Copyright (c) 2014-2019 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_INTERNAL_DEMANGLE_HPP
#define TAO_PEGTL_INTERNAL_DEMANGLE_HPP

#include <string>
#include <typeinfo>

#include "../config.hpp"

#if defined( __GLIBCXX__ )
#define TAO_PEGTL_USE_CXXABI_DEMANGLE
#elif defined( __has_include )
#if __has_include( <cxxabi.h> )
#define TAO_PEGTL_USE_CXXABI_DEMANGLE
#endif
#endif

#if defined( TAO_PEGTL_USE_CXXABI_DEMANGLE )
#include "demangle_cxxabi.hpp"
#undef TAO_PEGTL_USE_CXXABI_DEMANGLE
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
