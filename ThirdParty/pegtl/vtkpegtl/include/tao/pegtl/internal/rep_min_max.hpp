// Copyright (c) 2014-2020 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_INTERNAL_REP_MIN_MAX_HPP
#define TAO_PEGTL_INTERNAL_REP_MIN_MAX_HPP

#include <type_traits>

#include "../config.hpp"

#include "not_at.hpp"
#include "seq.hpp"
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
         template< unsigned Min, unsigned Max, typename... Rules >
         struct rep_min_max
            : rep_min_max< Min, Max, seq< Rules... > >
         {
         };

         template< unsigned Min, unsigned Max >
         struct rep_min_max< Min, Max >
            : trivial< false >
         {
            static_assert( Min <= Max, "invalid rep_min_max rule (maximum number of repetitions smaller than minimum)" );
         };

         template< typename Rule >
         struct rep_min_max< 0, 0, Rule >
            : not_at< Rule >
         {
         };

         template< unsigned Min, unsigned Max, typename Rule >
         struct rep_min_max< Min, Max, Rule >
         {
            using analyze_t = analysis::counted< analysis::rule_type::seq, Min, Rule >;

            static_assert( Min <= Max, "invalid rep_min_max rule (maximum number of repetitions smaller than minimum)" );

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
               auto m = in.template mark< M >();
               using m_t = decltype( m );

               for( unsigned i = 0; i != Min; ++i ) {
                  if( !Control< Rule >::template match< A, m_t::next_rewind_mode, Action, Control >( in, st... ) ) {
                     return false;
                  }
               }
               for( unsigned i = Min; i != Max; ++i ) {
                  if( !Control< Rule >::template match< A, rewind_mode::required, Action, Control >( in, st... ) ) {
                     return m( true );
                  }
               }
               return m( Control< not_at< Rule > >::template match< A, m_t::next_rewind_mode, Action, Control >( in, st... ) );  // NOTE that not_at<> will always rewind.
            }
         };

         template< unsigned Min, unsigned Max, typename... Rules >
         struct skip_control< rep_min_max< Min, Max, Rules... > > : std::true_type
         {
         };

      }  // namespace internal

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
