// Copyright (c) 2019 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_CONTRIB_REMOVE_FIRST_STATE_HPP
#define TAO_PEGTL_CONTRIB_REMOVE_FIRST_STATE_HPP

#include "../config.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      // NOTE: The naming of the following classes might still change.

      template< typename Base >
      struct remove_first_state_after_match
         : Base
      {
         template< typename Input, typename State, typename... States >
         static void start( const Input& in, State&& /*unused*/, States&&... st ) noexcept( noexcept( Base::start( in, st... ) ) )
         {
            Base::start( in, st... );
         }

         template< typename Input, typename State, typename... States >
         static void success( const Input& in, State&& /*unused*/, States&&... st ) noexcept( noexcept( Base::success( in, st... ) ) )
         {
            Base::success( in, st... );
         }

         template< typename Input, typename State, typename... States >
         static void failure( const Input& in, State&& /*unused*/, States&&... st ) noexcept( noexcept( Base::failure( in, st... ) ) )
         {
            Base::failure( in, st... );
         }

         template< typename Input, typename State, typename... States >
         static void raise( const Input& in, State&& /*unused*/, States&&... st )
         {
            Base::raise( in, st... );
         }

         template< template< typename... > class Action,
                   typename Iterator,
                   typename Input,
                   typename State,
                   typename... States >
         static auto apply( const Iterator& begin, const Input& in, State&& /*unused*/, States&&... st ) noexcept( noexcept( Base::template apply< Action >( begin, in, st... ) ) )
            -> decltype( Base::template apply< Action >( begin, in, st... ) )
         {
            return Base::template apply< Action >( begin, in, st... );
         }

         template< template< typename... > class Action,
                   typename Input,
                   typename State,
                   typename... States >
         static auto apply0( const Input& in, State&& /*unused*/, States&&... st ) noexcept( noexcept( Base::template apply0< Action >( in, st... ) ) )
            -> decltype( Base::template apply0< Action >( in, st... ) )
         {
            return Base::template apply0< Action >( in, st... );
         }
      };

      template< typename Rule, template< typename... > class Control >
      struct remove_self_and_first_state
         : Control< Rule >
      {
         template< apply_mode A,
                   rewind_mode M,
                   template< typename... >
                   class Action,
                   template< typename... >
                   class,
                   typename Input,
                   typename State,
                   typename... States >
         static bool match( Input& in, State&& /*unused*/, States&&... st )
         {
            return Control< Rule >::template match< A, M, Action, Control >( in, st... );
         }
      };

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
