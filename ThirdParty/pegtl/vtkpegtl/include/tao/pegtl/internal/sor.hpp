// Copyright (c) 2014-2020 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_INTERNAL_SOR_HPP
#define TAO_PEGTL_INTERNAL_SOR_HPP

#include "../config.hpp"

#include "integer_sequence.hpp"
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
         struct sor;

         template<>
         struct sor<>
            : trivial< false >
         {
         };

         template< typename... Rules >
         struct sor
            : sor< index_sequence_for< Rules... >, Rules... >
         {
         };

         template< std::size_t... Indices, typename... Rules >
         struct sor< index_sequence< Indices... >, Rules... >
         {
            using analyze_t = analysis::generic< analysis::rule_type::sor, Rules... >;

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
#ifdef __cpp_fold_expressions
               return ( Control< Rules >::template match < A, ( Indices == ( sizeof...( Rules ) - 1 ) ) ? M : rewind_mode::required, Action, Control > ( in, st... ) || ... );
#else
               bool result = false;
               using swallow = bool[];
               (void)swallow{ result = result || Control< Rules >::template match < A, ( Indices == ( sizeof...( Rules ) - 1 ) ) ? M : rewind_mode::required, Action, Control > ( in, st... )... };
               return result;
#endif
            }
         };

         template< typename... Rules >
         struct skip_control< sor< Rules... > > : std::true_type
         {
         };

      }  // namespace internal

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
