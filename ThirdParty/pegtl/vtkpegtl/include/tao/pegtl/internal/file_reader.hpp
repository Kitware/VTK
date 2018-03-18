// Copyright (c) 2014-2018 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_INTERNAL_FILE_READER_HPP
#define TAO_PEGTL_INTERNAL_FILE_READER_HPP

#include <cstdio>
#include <memory>
#include <string>
#include <utility>

#include "../config.hpp"
#include "../input_error.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         inline std::FILE* file_open( const char* filename )
         {
            errno = 0;
#if defined( _MSC_VER )
            std::FILE* file;
            if(::fopen_s( &file, filename, "rb" ) == 0 )
#elif defined( __MINGW32__ )
            if( auto* file = std::fopen( filename, "rb" ) )
#else
            if( auto* file = std::fopen( filename, "rbe" ) )
#endif
            {
               return file;
            }
            TAO_PEGTL_THROW_INPUT_ERROR( "unable to fopen() file " << filename << " for reading" );
         }

         struct file_close
         {
            void operator()( FILE* f ) const noexcept
            {
               std::fclose( f );
            }
         };

         class file_reader
         {
         public:
            explicit file_reader( const char* filename )
               : m_source( filename ),
                 m_file( file_open( m_source ) )
            {
            }

            file_reader( FILE* file, const char* filename ) noexcept
               : m_source( filename ),
                 m_file( file )
            {
            }

            file_reader( const file_reader& ) = delete;
            file_reader( file_reader&& ) = delete;

            ~file_reader() = default;

            void operator=( const file_reader& ) = delete;
            void operator=( file_reader&& ) = delete;

            std::size_t size() const
            {
               errno = 0;
               if( std::fseek( m_file.get(), 0, SEEK_END ) != 0 ) {
                  TAO_PEGTL_THROW_INPUT_ERROR( "unable to fseek() to end of file " << m_source );  // LCOV_EXCL_LINE
               }
               errno = 0;
               const auto s = std::ftell( m_file.get() );
               if( s < 0 ) {
                  TAO_PEGTL_THROW_INPUT_ERROR( "unable to ftell() file size of file " << m_source );  // LCOV_EXCL_LINE
               }
               errno = 0;
               if( std::fseek( m_file.get(), 0, SEEK_SET ) != 0 ) {
                  TAO_PEGTL_THROW_INPUT_ERROR( "unable to fseek() to beginning of file " << m_source );  // LCOV_EXCL_LINE
               }
               return std::size_t( s );
            }

            std::string read() const
            {
               std::string nrv;
               nrv.resize( size() );
               errno = 0;
               if( !nrv.empty() && ( std::fread( &nrv[ 0 ], nrv.size(), 1, m_file.get() ) != 1 ) ) {
                  TAO_PEGTL_THROW_INPUT_ERROR( "unable to fread() file " << m_source << " size " << nrv.size() );  // LCOV_EXCL_LINE
               }
               return nrv;
            }

         private:
            const char* const m_source;
            const std::unique_ptr< std::FILE, file_close > m_file;
         };

      }  // namespace internal

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
