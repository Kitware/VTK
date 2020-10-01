// Copyright (c) 2016-2020 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_INTERNAL_CSTREAM_READER_HPP
#define TAO_PEGTL_INTERNAL_CSTREAM_READER_HPP

#include <cassert>
#include <cstddef>
#include <cstdio>

#include "../config.hpp"
#include "../input_error.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         struct cstream_reader
         {
            explicit cstream_reader( std::FILE* s ) noexcept
               : m_cstream( s )
            {
               assert( m_cstream != nullptr );
            }

            std::size_t operator()( char* buffer, const std::size_t length ) const
            {
               if( const auto r = std::fread( buffer, 1, length, m_cstream ) ) {
                  return r;
               }
               if( std::feof( m_cstream ) != 0 ) {
                  return 0;
               }
               // Please contact us if you know how to provoke the following exception.
               // The example on cppreference.com doesn't work, at least not on macOS.
               TAO_PEGTL_THROW_INPUT_ERROR( "error in fread() from cstream" );  // LCOV_EXCL_LINE
            }

            std::FILE* m_cstream;
         };

      }  // namespace internal

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
