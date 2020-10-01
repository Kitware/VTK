// Copyright (c) 2014-2020 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_INTERNAL_MUST_HPP
#define TAO_PEGTL_INTERNAL_MUST_HPP

#include "../config.hpp"

#include "raise.hpp"
#include "seq.hpp"
#include "skip_control.hpp"

#include "../apply_mode.hpp"
#include "../rewind_mode.hpp"

#include "../analysis/generic.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         // The general case applies must<> to each of the
         // rules in the 'Rules' parameter pack individually.

         template< typename... Rules >
         struct must
            : seq< must< Rules >... >
         {
         };

         // While in theory the implementation for a single rule could
         // be simplified to must< Rule > = sor< Rule, raise< Rule > >, this
         // would result in some unnecessary run-time overhead.

         template< typename Rule >
         struct must< Rule >
         {
            using analyze_t = typename Rule::analyze_t;

            template< apply_mode A,
                      rewind_mode,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, States&&... st )
            {
               if( !Control< Rule >::template match< A, rewind_mode::dontcare, Action, Control >( in, st... ) ) {
                  raise< Rule >::template match< A, rewind_mode::dontcare, Action, Control >( in, st... );
               }
               return true;
            }
         };

         template< typename... Rules >
         struct skip_control< must< Rules... > > : std::true_type
         {
         };

      }  // namespace internal

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
