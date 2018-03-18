// Copyright (c) 2014-2018 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_INTERNAL_STAR_HPP
#define TAO_PEGTL_INTERNAL_STAR_HPP

#include <type_traits>

#include "../config.hpp"

#include "duseltronik.hpp"
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
         template< typename Rule, typename... Rules >
         struct star
         {
            using analyze_t = analysis::generic< analysis::rule_type::OPT, Rule, Rules..., star >;

            template< apply_mode A,
                      rewind_mode,
                      template< typename... > class Action,
                      template< typename... > class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, States&&... st )
            {
               while( seq< Rule, Rules... >::template match< A, rewind_mode::REQUIRED, Action, Control >( in, st... ) ) {
               }
               return true;
            }
         };

         template< typename Rule, typename... Rules >
         struct skip_control< star< Rule, Rules... > > : std::true_type
         {
         };

      }  // namespace internal

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
