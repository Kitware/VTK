// Copyright (c) 2014-2018 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_PARSE_HPP
#define TAO_PEGTL_PARSE_HPP

#include <cassert>

#include "apply_mode.hpp"
#include "config.hpp"
#include "normal.hpp"
#include "nothing.hpp"
#include "parse_error.hpp"
#include "rewind_mode.hpp"

#include "internal/action_input.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename From >
         struct copy_internals
         {
            template< typename To >
            static void apply( const From& from, To& to ) noexcept
            {
               assert( to.internal_state == nullptr );
               to.internal_state = from.internal_state;
            }
         };

         template< typename Input >
         struct copy_internals< action_input< Input > >
         {
            template< typename To >
            static void apply( const action_input< Input >& from, To& to ) noexcept
            {
               assert( to.internal_state == nullptr );
               to.internal_state = from.input().internal_state;
            }
         };
      }  // namespace internal

      template< typename Rule,
                template< typename... > class Action = nothing,
                template< typename... > class Control = normal,
                apply_mode A = apply_mode::ACTION,
                rewind_mode M = rewind_mode::REQUIRED,
                typename Input,
                typename... States >
      bool parse( Input&& in, States&&... st )
      {
         return Control< Rule >::template match< A, M, Action, Control >( in, st... );
      }

      template< typename Rule,
                template< typename... > class Action = nothing,
                template< typename... > class Control = normal,
                apply_mode A = apply_mode::ACTION,
                rewind_mode M = rewind_mode::REQUIRED,
                typename Outer,
                typename Input,
                typename... States >
      bool parse_nested( const Outer& oi, Input&& in, States&&... st )
      {
         try {
            internal::copy_internals< Outer >::apply( oi, in );
            return parse< Rule, Action, Control, A, M >( in, st... );
         }
         catch( parse_error& e ) {
            e.positions.push_back( oi.position() );
            throw;
         }
      }

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
