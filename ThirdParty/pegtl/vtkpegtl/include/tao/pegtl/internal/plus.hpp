// Copyright (c) 2014-2018 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_INTERNAL_PLUS_HPP
#define TAO_PEGTL_INTERNAL_PLUS_HPP

#include <type_traits>

#include "../config.hpp"

#include "duseltronik.hpp"
#include "opt.hpp"
#include "seq.hpp"
#include "skip_control.hpp"
#include "star.hpp"

#include "../apply_mode.hpp"
#include "../rewind_mode.hpp"

#include "../analysis/generic.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         // While plus<> could easily be implemented with
         // seq< Rule, Rules ..., star< Rule, Rules ... > > we
         // provide an explicit implementation to optimise away
         // the otherwise created input mark.

         template< typename Rule, typename... Rules >
         struct plus
         {
            using analyze_t = analysis::generic< analysis::rule_type::SEQ, Rule, Rules..., opt< plus > >;

            template< apply_mode A,
                      rewind_mode M,
                      template< typename... > class Action,
                      template< typename... > class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, States&&... st )
            {
               return seq< Rule, Rules... >::template match< A, M, Action, Control >( in, st... ) && star< Rule, Rules... >::template match< A, M, Action, Control >( in, st... );
            }
         };

         template< typename Rule, typename... Rules >
         struct skip_control< plus< Rule, Rules... > > : std::true_type
         {
         };

      }  // namespace internal

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
