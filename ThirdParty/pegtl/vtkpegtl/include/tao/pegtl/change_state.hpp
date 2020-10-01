// Copyright (c) 2019-2020 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_CHANGE_STATE_HPP
#define TAO_PEGTL_CHANGE_STATE_HPP

#include <type_traits>

#include "apply_mode.hpp"
#include "config.hpp"
#include "match.hpp"
#include "nothing.hpp"
#include "rewind_mode.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      template< typename NewState >
      struct change_state
         : maybe_nothing
      {
         template< typename Rule,
                   apply_mode A,
                   rewind_mode M,
                   template< typename... >
                   class Action,
                   template< typename... >
                   class Control,
                   typename Input,
                   typename... States >
         static auto match( Input& in, States&&... st )
            -> typename std::enable_if< ( A == apply_mode::action ), bool >::type
         {
            NewState s( static_cast< const Input& >( in ), st... );
            if( TAO_PEGTL_NAMESPACE::match< Rule, A, M, Action, Control >( in, s ) ) {
               Action< Rule >::success( static_cast< const Input& >( in ), s, st... );
               return true;
            }
            return false;
         }

         template< typename Rule,
                   apply_mode A,
                   rewind_mode M,
                   template< typename... >
                   class Action,
                   template< typename... >
                   class Control,
                   typename Input,
                   typename... States,
                   int = 1 >
         static auto match( Input& in, States&&... st )
            -> typename std::enable_if< ( A == apply_mode::nothing ), bool >::type
         {
            NewState s( static_cast< const Input& >( in ), st... );
            return TAO_PEGTL_NAMESPACE::match< Rule, A, M, Action, Control >( in, s );
         }

         template< typename Input,
                   typename... States >
         static void success( const Input& in, NewState& s, States&&... st ) noexcept( noexcept( s.success( in, st... ) ) )
         {
            s.success( in, st... );
         }
      };

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
