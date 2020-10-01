// Copyright (c) 2014-2020 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_ANALYSIS_INSERT_GUARD_HPP
#define TAO_PEGTL_ANALYSIS_INSERT_GUARD_HPP

#include <utility>

#include "../config.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace analysis
      {
         template< typename C >
         class insert_guard
         {
         public:
            insert_guard( C& container, const typename C::value_type& value )
               : m_i( container.insert( value ) ),
                 m_c( &container )
            {
            }

            insert_guard( const insert_guard& ) = delete;

            insert_guard( insert_guard&& other ) noexcept
               : m_i( other.m_i ),
                 m_c( other.m_c )
            {
               other.m_c = nullptr;
            }

            ~insert_guard()
            {
               if( m_c && m_i.second ) {
                  m_c->erase( m_i.first );
               }
            }

            void operator=( const insert_guard& ) = delete;
            void operator=( insert_guard&& ) = delete;

            explicit operator bool() const noexcept
            {
               return m_i.second;
            }

         private:
            const std::pair< typename C::iterator, bool > m_i;
            C* m_c;
         };

         template< typename C >
         insert_guard< C > make_insert_guard( C& container, const typename C::value_type& value )
         {
            return insert_guard< C >( container, value );
         }

      }  // namespace analysis

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
