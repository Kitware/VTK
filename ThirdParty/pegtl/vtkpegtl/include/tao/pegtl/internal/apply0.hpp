// Copyright (c) 2017-2018 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_INTERNAL_APPLY0_HPP
#define TAO_PEGTL_INTERNAL_APPLY0_HPP

#include "../config.hpp"

#include "apply0_single.hpp"
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
         struct apply0_impl;

         template<>
         struct apply0_impl< apply_mode::ACTION >
         {
            template< typename... States >
            static bool match( States&&... /*unused*/ ) noexcept
            {
               return true;
            }
         };

         template< typename... Actions >
         struct apply0_impl< apply_mode::ACTION, Actions... >
         {
            template< typename... States >
            static bool match( States&&... st )
            {
#ifdef __cpp_fold_expressions
               return ( apply0_single< Actions >::match( st... ) && ... );
#else
               bool result = true;
               using swallow = bool[];
               (void)swallow{ result = result && apply0_single< Actions >::match( st... )... };
               return result;
#endif
            }
         };

         template< typename... Actions >
         struct apply0_impl< apply_mode::NOTHING, Actions... >
         {
            template< typename... States >
            static bool match( States&&... /*unused*/ ) noexcept
            {
               return true;
            }
         };

         template< typename... Actions >
         struct apply0
         {
            using analyze_t = analysis::counted< analysis::rule_type::ANY, 0 >;

            template< apply_mode A,
                      rewind_mode M,
                      template< typename... > class Action,
                      template< typename... > class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& /*unused*/, States&&... st )
            {
               return apply0_impl< A, Actions... >::match( st... );
            }
         };

         template< typename... Actions >
         struct skip_control< apply0< Actions... > > : std::true_type
         {
         };

      }  // namespace internal

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
