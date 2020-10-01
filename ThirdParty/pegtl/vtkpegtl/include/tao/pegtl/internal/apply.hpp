// Copyright (c) 2017-2020 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_INTERNAL_APPLY_HPP
#define TAO_PEGTL_INTERNAL_APPLY_HPP

#include "../config.hpp"

#include "apply_single.hpp"
#include "skip_control.hpp"

#include "../analysis/counted.hpp"
#include "../apply_mode.hpp"
#include "../rewind_mode.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< apply_mode A, typename... Actions >
         struct apply_impl;

         template<>
         struct apply_impl< apply_mode::action >
         {
            template< typename Input, typename... States >
            static bool match( Input& /*unused*/, States&&... /*unused*/ )
            {
               return true;
            }
         };

         template< typename... Actions >
         struct apply_impl< apply_mode::action, Actions... >
         {
            template< typename Input, typename... States >
            static bool match( Input& in, States&&... st )
            {
               using action_t = typename Input::action_t;
               const action_t i2( in.iterator(), in );  // No data -- range is from begin to begin.
#ifdef __cpp_fold_expressions
               return ( apply_single< Actions >::match( i2, st... ) && ... );
#else
               bool result = true;
               using swallow = bool[];
               (void)swallow{ result = result && apply_single< Actions >::match( i2, st... )... };
               return result;
#endif
            }
         };

         template< typename... Actions >
         struct apply_impl< apply_mode::nothing, Actions... >
         {
            template< typename Input, typename... States >
            static bool match( Input& /*unused*/, States&&... /*unused*/ )
            {
               return true;
            }
         };

         template< typename... Actions >
         struct apply
         {
            using analyze_t = analysis::counted< analysis::rule_type::any, 0 >;

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
               return apply_impl< A, Actions... >::match( in, st... );
            }
         };

         template< typename... Actions >
         struct skip_control< apply< Actions... > > : std::true_type
         {
         };

      }  // namespace internal

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
