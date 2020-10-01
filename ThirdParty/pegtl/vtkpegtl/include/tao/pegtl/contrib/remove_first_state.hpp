// Copyright (c) 2019-2020 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_CONTRIB_REMOVE_FIRST_STATE_HPP
#define TAO_PEGTL_CONTRIB_REMOVE_FIRST_STATE_HPP

#include "../config.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      // Applies to start(), success(), failure(), raise(), apply(), and apply0():
      // The first state is removed when the call is forwarded to Base.
      template< typename Base >
      struct remove_first_state
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

         template< template< typename... > class Action, typename Iterator, typename Input, typename State, typename... States >
         static auto apply( const Iterator& begin, const Input& in, State&& /*unused*/, States&&... st ) noexcept( noexcept( Base::template apply< Action >( begin, in, st... ) ) )
            -> decltype( Base::template apply< Action >( begin, in, st... ) )
         {
            return Base::template apply< Action >( begin, in, st... );
         }

         template< template< typename... > class Action, typename Input, typename State, typename... States >
         static auto apply0( const Input& in, State&& /*unused*/, States&&... st ) noexcept( noexcept( Base::template apply0< Action >( in, st... ) ) )
            -> decltype( Base::template apply0< Action >( in, st... ) )
         {
            return Base::template apply0< Action >( in, st... );
         }
      };

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
