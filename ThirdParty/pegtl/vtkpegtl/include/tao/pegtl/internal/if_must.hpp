// Copyright (c) 2014-2020 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_INTERNAL_IF_MUST_HPP
#define TAO_PEGTL_INTERNAL_IF_MUST_HPP

#include "../config.hpp"

#include "must.hpp"
#include "skip_control.hpp"
#include "trivial.hpp"

#include "../apply_mode.hpp"
#include "../rewind_mode.hpp"

#include "../analysis/counted.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< bool Default, typename Cond, typename... Rules >
         struct if_must
         {
            using analyze_t = analysis::counted< analysis::rule_type::seq, Default ? 0 : 1, Cond, must< Rules... > >;

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
               if( Control< Cond >::template match< A, M, Action, Control >( in, st... ) ) {
                  Control< must< Rules... > >::template match< A, M, Action, Control >( in, st... );
                  return true;
               }
               return Default;
            }
         };

         template< bool Default, typename Cond, typename... Rules >
         struct skip_control< if_must< Default, Cond, Rules... > > : std::true_type
         {
         };

      }  // namespace internal

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
