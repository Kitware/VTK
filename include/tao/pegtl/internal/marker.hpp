// Copyright (c) 2014-2020 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_INTERNAL_MARKER_HPP
#define TAO_PEGTL_INTERNAL_MARKER_HPP

#include "../config.hpp"
#include "../rewind_mode.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename Iterator, rewind_mode M >
         class marker
         {
         public:
            static constexpr rewind_mode next_rewind_mode = M;

            explicit marker( const Iterator& /*unused*/ ) noexcept
            {
            }

            marker( const marker& ) = delete;

            marker( marker&& /*unused*/ ) noexcept
            {
            }

            ~marker() = default;

            void operator=( const marker& ) = delete;
            void operator=( marker&& ) = delete;

            bool operator()( const bool result ) const noexcept
            {
               return result;
            }
         };

         template< typename Iterator >
         class marker< Iterator, rewind_mode::required >
         {
         public:
            static constexpr rewind_mode next_rewind_mode = rewind_mode::active;

            explicit marker( Iterator& i ) noexcept
               : m_saved( i ),
                 m_input( &i )
            {
            }

            marker( const marker& ) = delete;

            marker( marker&& i ) noexcept
               : m_saved( i.m_saved ),
                 m_input( i.m_input )
            {
               i.m_input = nullptr;
            }

            ~marker() noexcept
            {
               if( m_input != nullptr ) {
                  ( *m_input ) = m_saved;
               }
            }

            void operator=( const marker& ) = delete;
            void operator=( marker&& ) = delete;

            bool operator()( const bool result ) noexcept
            {
               if( result ) {
                  m_input = nullptr;
                  return true;
               }
               return false;
            }

            const Iterator& iterator() const noexcept
            {
               return m_saved;
            }

         private:
            const Iterator m_saved;
            Iterator* m_input;
         };

      }  // namespace internal

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
