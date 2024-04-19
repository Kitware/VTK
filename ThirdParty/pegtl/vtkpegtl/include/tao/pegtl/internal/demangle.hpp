// Copyright (c) 2014-2020 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_INTERNAL_DEMANGLE_HPP
#define TAO_PEGTL_INTERNAL_DEMANGLE_HPP

#include <string>
#include <typeinfo>

#include "../config.hpp"

#if defined( __clang__ )
#if __has_feature( cxx_rtti )
#define TAO_PEGTL_RTTI_ENABLED
#endif
#elif defined( __GNUC__ )
#if defined( __GXX_RTTI )
#define TAO_PEGTL_RTTI_ENABLED
#endif
#elif defined( _MSC_VER )
#if defined( _CPPRTTI )
#define TAO_PEGTL_RTTI_ENABLED
#endif
#else
#define TAO_PEGTL_RTTI_ENABLED
#endif

#if !defined( TAO_PEGTL_RTTI_ENABLED )
#include <cassert>
#include <cstring>
#endif

#if defined( TAO_PEGTL_RTTI_ENABLED )
#if defined( __GLIBCXX__ )
#define TAO_PEGTL_USE_CXXABI_DEMANGLE
#elif defined( __has_include )
#if __has_include( <cxxabi.h> )
#define TAO_PEGTL_USE_CXXABI_DEMANGLE
#endif
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
#if defined( TAO_PEGTL_RTTI_ENABLED )
            return demangle( typeid( T ).name() );
#else
            const char* start = nullptr;
            const char* stop = nullptr;
#if defined( __clang__ ) || defined( __GNUC__ )
            start = std::strchr( __PRETTY_FUNCTION__, '=' ) + 2;
            stop = std::strrchr( start, ';' );
#elif defined( _MSC_VER )
            start = std::strstr( __FUNCSIG__, "demangle<" ) + ( sizeof( "demangle<" ) - 1 );
            stop = std::strrchr( start, '>' );
#else
            static_assert( false, "expected to use rtti with this compiler" );
#endif
            assert( start != nullptr );
            assert( stop != nullptr );
            return { start, std::size_t( stop - start ) };
#endif
         }

      }  // namespace internal

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
