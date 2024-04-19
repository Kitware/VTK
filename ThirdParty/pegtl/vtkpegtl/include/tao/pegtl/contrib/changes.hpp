// Copyright (c) 2015-2020 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_CONTRIB_CHANGES_HPP
#define TAO_PEGTL_CONTRIB_CHANGES_HPP

#include <type_traits>

#include "../config.hpp"
#include "../normal.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         struct dummy_disabled_state
         {
            template< typename... Ts >
            void success( Ts&&... /*unused*/ ) const noexcept
            {
            }
         };

         template< apply_mode A, typename NewState >
         using state_disable_helper = typename std::conditional< A == apply_mode::action, NewState, dummy_disabled_state >::type;

      }  // namespace internal

      template< typename Rule, typename NewState, template< typename... > class Base = normal >
      struct change_state
         : public Base< Rule >
      {
         template< apply_mode A,
                   rewind_mode M,
                   template< typename... >
                   class Action,
                   template< typename... >
                   class Control,
                   typename Input,
                   typename... States >
         static bool match( Input& in, States&&... st )
         {
            internal::state_disable_helper< A, NewState > s;

            if( Base< Rule >::template match< A, M, Action, Control >( in, s ) ) {
               s.success( st... );
               return true;
            }
            return false;
         }
      };

      template< typename Rule, template< typename... > class NewAction, template< typename... > class Base = normal >
      struct change_action
         : public Base< Rule >
      {
         template< apply_mode A,
                   rewind_mode M,
                   template< typename... >
                   class Action,
                   template< typename... >
                   class Control,
                   typename Input,
                   typename... States >
         static bool match( Input& in, States&&... st )
         {
            return Base< Rule >::template match< A, M, NewAction, Control >( in, st... );
         }
      };

      template< template< typename... > class NewAction, template< typename... > class Base >
      struct change_both_helper
      {
         template< typename T >
         using change_action = change_action< T, NewAction, Base >;
      };

      template< typename Rule, typename NewState, template< typename... > class NewAction, template< typename... > class Base = normal >
      struct change_state_and_action
         : public change_state< Rule, NewState, change_both_helper< NewAction, Base >::template change_action >
      {
      };

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
