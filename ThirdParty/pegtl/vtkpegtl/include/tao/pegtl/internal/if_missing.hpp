// Copyright (c) 2019-2020 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_INTERNAL_IF_MISSING_HPP
#define TAO_PEGTL_INTERNAL_IF_MISSING_HPP

#include "../config.hpp"
#include "../rewind_mode.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< bool >
         struct if_missing;

         template<>
         struct if_missing< true >
         {
            template< typename Control,
                      template< typename... >
                      class Action,
                      typename Input,
                      typename... States >
            static void apply( Input& in, States&&... st )
            {
               auto m = in.template mark< rewind_mode::required >();
               Control::template apply< Action >( m.iterator(), in, st... );
            }

            template< typename Control,
                      template< typename... >
                      class Action,
                      typename Input,
                      typename... States >
            static void apply0( Input& in, States&&... st )
            {
               Control::template apply0< Action >( in, st... );
            }
         };

         template<>
         struct if_missing< false >
         {
            template< typename Control,
                      template< typename... >
                      class Action,
                      typename Input,
                      typename... States >
            static void apply( Input& /*unused*/, States&&... /*unused*/ )
            {
            }

            template< typename Control,
                      template< typename... >
                      class Action,
                      typename Input,
                      typename... States >
            static void apply0( Input& /*unused*/, States&&... /*unused*/ )
            {
            }
         };

      }  // namespace internal

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
