// Copyright (c) 2014-2020 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_INTERNAL_AT_HPP
#define TAO_PEGTL_INTERNAL_AT_HPP

#include "../config.hpp"

#include "seq.hpp"
#include "skip_control.hpp"
#include "trivial.hpp"

#include "../apply_mode.hpp"
#include "../rewind_mode.hpp"

#include "../analysis/generic.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename... Rules >
         struct at
            : at< seq< Rules... > >
         {
         };

         template<>
         struct at<>
            : trivial< true >
         {
         };

         template< typename Rule >
         struct at< Rule >
         {
            using analyze_t = analysis::generic< analysis::rule_type::opt, Rule >;

            template< apply_mode,
                      rewind_mode,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, States&&... st )
            {
               const auto m = in.template mark< rewind_mode::required >();
               return Control< Rule >::template match< apply_mode::nothing, rewind_mode::active, Action, Control >( in, st... );
            }
         };

         template< typename... Rules >
         struct skip_control< at< Rules... > > : std::true_type
         {
         };

      }  // namespace internal

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
