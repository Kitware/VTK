// Copyright (c) 2014-2018 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_MMAP_INPUT_HPP
#define TAO_PEGTL_MMAP_INPUT_HPP

#include <string>
#include <utility>

#include "config.hpp"
#include "eol.hpp"
#include "memory_input.hpp"
#include "tracking_mode.hpp"

#include "internal/file_mapper.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         struct mmap_holder
         {
            const std::string filename;
            const file_mapper data;

            template< typename T >
            explicit mmap_holder( T&& in_filename )
               : filename( std::forward< T >( in_filename ) ),
                 data( filename.c_str() )
            {
            }

            mmap_holder( const mmap_holder& ) = delete;
            mmap_holder( mmap_holder&& ) = delete;

            ~mmap_holder() = default;

            void operator=( const mmap_holder& ) = delete;
            void operator=( mmap_holder&& ) = delete;
         };

      }  // namespace internal

      template< tracking_mode P = tracking_mode::IMMEDIATE, typename Eol = eol::lf_crlf >
      struct mmap_input
         : private internal::mmap_holder,
           public memory_input< P, Eol, const char* >
      {
         template< typename T >
         explicit mmap_input( T&& in_filename )
            : internal::mmap_holder( std::forward< T >( in_filename ) ),
              memory_input< P, Eol, const char* >( data.begin(), data.end(), filename.c_str() )
         {
         }

         mmap_input( const mmap_input& ) = delete;
         mmap_input( mmap_input&& ) = delete;

         ~mmap_input() = default;

         void operator=( const mmap_input& ) = delete;
         void operator=( mmap_input&& ) = delete;
      };

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
