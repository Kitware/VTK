// Copyright (c) 2014-2020 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_INTERNAL_FILE_MAPPER_WIN32_HPP
#define TAO_PEGTL_INTERNAL_FILE_MAPPER_WIN32_HPP

#if !defined( NOMINMAX )
#define NOMINMAX
#define TAO_PEGTL_NOMINMAX_WAS_DEFINED
#endif

#if !defined( WIN32_LEAN_AND_MEAN )
#define WIN32_LEAN_AND_MEAN
#define TAO_PEGTL_WIN32_LEAN_AND_MEAN_WAS_DEFINED
#endif

#include <windows.h>

#if defined( TAO_PEGTL_NOMINMAX_WAS_DEFINED )
#undef NOMINMAX
#undef TAO_PEGTL_NOMINMAX_WAS_DEFINED
#endif

#if defined( TAO_PEGTL_WIN32_LEAN_AND_MEAN_WAS_DEFINED )
#undef WIN32_LEAN_AND_MEAN
#undef TAO_PEGTL_WIN32_LEAN_AND_MEAN_WAS_DEFINED
#endif

#include "../config.hpp"
#include "../input_error.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         struct win32_file_opener
         {
            explicit win32_file_opener( const char* filename )
               : m_source( filename ),
                 m_handle( open() )
            {
            }

            win32_file_opener( const win32_file_opener& ) = delete;
            win32_file_opener( win32_file_opener&& ) = delete;

            ~win32_file_opener() noexcept
            {
               ::CloseHandle( m_handle );
            }

            void operator=( const win32_file_opener& ) = delete;
            void operator=( win32_file_opener&& ) = delete;

            std::size_t size() const
            {
               LARGE_INTEGER size;
               if( !::GetFileSizeEx( m_handle, &size ) ) {
                  TAO_PEGTL_THROW_INPUT_WIN32_ERROR( "unable to GetFileSizeEx() file " << m_source << " handle " << m_handle );
               }
               return std::size_t( size.QuadPart );
            }

            const char* const m_source;
            const HANDLE m_handle;

         private:
            HANDLE open() const
            {
               SetLastError( 0 );
               std::wstring ws( m_source, m_source + strlen( m_source ) );

#if( _WIN32_WINNT >= 0x0602 )
               const HANDLE handle = ::CreateFile2( ws.c_str(),
                                                    GENERIC_READ,
                                                    FILE_SHARE_READ,
                                                    OPEN_EXISTING,
                                                    nullptr );
               if( handle != INVALID_HANDLE_VALUE ) {
                  return handle;
               }
               TAO_PEGTL_THROW_INPUT_WIN32_ERROR( "CreateFile2() failed opening file " << m_source << " for reading" );
#else
               const HANDLE handle = ::CreateFileW( ws.c_str(),
                                                    GENERIC_READ,
                                                    FILE_SHARE_READ,
                                                    nullptr,
                                                    OPEN_EXISTING,
                                                    FILE_ATTRIBUTE_NORMAL,
                                                    nullptr );
               if( handle != INVALID_HANDLE_VALUE ) {
                  return handle;
               }
               TAO_PEGTL_THROW_INPUT_WIN32_ERROR( "CreateFileW() failed opening file " << m_source << " for reading" );
#endif
            }
         };

         struct win32_file_mapper
         {
            explicit win32_file_mapper( const char* filename )
               : win32_file_mapper( win32_file_opener( filename ) )
            {
            }

            explicit win32_file_mapper( const win32_file_opener& reader )
               : m_size( reader.size() ),
                 m_handle( open( reader ) )
            {
            }

            win32_file_mapper( const win32_file_mapper& ) = delete;
            win32_file_mapper( win32_file_mapper&& ) = delete;

            ~win32_file_mapper() noexcept
            {
               ::CloseHandle( m_handle );
            }

            void operator=( const win32_file_mapper& ) = delete;
            void operator=( win32_file_mapper&& ) = delete;

            const size_t m_size;
            const HANDLE m_handle;

         private:
            HANDLE open( const win32_file_opener& reader ) const
            {
               const uint64_t file_size = reader.size();
               SetLastError( 0 );
               // Use `CreateFileMappingW` because a) we're not specifying a
               // mapping name, so the character type is of no consequence, and
               // b) it's defined in `memoryapi.h`, unlike
               // `CreateFileMappingA`(?!)
               const HANDLE handle = ::CreateFileMappingW( reader.m_handle,
                                                           nullptr,
                                                           PAGE_READONLY,
                                                           DWORD( file_size >> 32 ),
                                                           DWORD( file_size & 0xffffffff ),
                                                           nullptr );
               if( handle != NULL || file_size == 0 ) {
                  return handle;
               }
               TAO_PEGTL_THROW_INPUT_WIN32_ERROR( "unable to CreateFileMappingW() file " << reader.m_source << " for reading" );
            }
         };

         class file_mapper
         {
         public:
            explicit file_mapper( const char* filename )
               : file_mapper( win32_file_mapper( filename ) )
            {
            }

            explicit file_mapper( const win32_file_mapper& mapper )
               : m_size( mapper.m_size ),
                 m_data( static_cast< const char* >( ::MapViewOfFile( mapper.m_handle,
                                                                      FILE_MAP_READ,
                                                                      0,
                                                                      0,
                                                                      0 ) ) )
            {
               if( ( m_size != 0 ) && ( intptr_t( m_data ) == 0 ) ) {
                  TAO_PEGTL_THROW_INPUT_WIN32_ERROR( "unable to MapViewOfFile() file mapping object with handle " << mapper.m_handle );
               }
            }

            file_mapper( const file_mapper& ) = delete;
            file_mapper( file_mapper&& ) = delete;

            ~file_mapper() noexcept
            {
               ::UnmapViewOfFile( LPCVOID( m_data ) );
            }

            void operator=( const file_mapper& ) = delete;
            void operator=( file_mapper&& ) = delete;

            bool empty() const noexcept
            {
               return m_size == 0;
            }

            std::size_t size() const noexcept
            {
               return m_size;
            }

            using iterator = const char*;
            using const_iterator = const char*;

            iterator data() const noexcept
            {
               return m_data;
            }

            iterator begin() const noexcept
            {
               return m_data;
            }

            iterator end() const noexcept
            {
               return m_data + m_size;
            }

            std::string string() const
            {
               return std::string( m_data, m_size );
            }

         private:
            const std::size_t m_size;
            const char* const m_data;
         };

      }  // namespace internal

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
