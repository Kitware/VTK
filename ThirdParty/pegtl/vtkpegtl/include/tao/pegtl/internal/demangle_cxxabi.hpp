// Copyright (c) 2014-2020 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_INTERNAL_DEMANGLE_CXXABI_HPP
#define TAO_PEGTL_INTERNAL_DEMANGLE_CXXABI_HPP

#include <cstdlib>
#include <cxxabi.h>
#include <memory>
#include <string>

#include "../config.hpp"

#include "demangle_sanitise.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         inline std::string demangle( const char* symbol )
         {
            const std::unique_ptr< char, decltype( &std::free ) > demangled( abi::__cxa_demangle( symbol, nullptr, nullptr, nullptr ), &std::free );
            if( !demangled ) {
               return symbol;
            }
            std::string result( demangled.get() );
#ifdef TAO_PEGTL_PRETTY_DEMANGLE
            demangle_sanitise_chars( result );  // LCOV_EXCL_LINE
#endif
            return result;
         }

      }  // namespace internal

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
