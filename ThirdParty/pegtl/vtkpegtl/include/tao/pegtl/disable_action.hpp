// Copyright (c) 2019 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_DISABLE_ACTION_HPP
#define TAO_PEGTL_DISABLE_ACTION_HPP

#include "apply_mode.hpp"
#include "config.hpp"
#include "match.hpp"
#include "nothing.hpp"
#include "rewind_mode.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      struct disable_action
         : maybe_nothing
      {
         template< typename Rule,
                   apply_mode A,
                   rewind_mode M,
                   template< typename... >
                   class Action,
                   template< typename... >
                   class Control,
                   typename Input,
                   typename... States >
         static bool match( Input& in, States&&... st )
         {
            return TAO_PEGTL_NAMESPACE::match< Rule, apply_mode::nothing, M, Action, Control >( in, st... );
         }
      };

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif