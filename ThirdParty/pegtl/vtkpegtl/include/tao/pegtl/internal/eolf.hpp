// Copyright (c) 2014-2020 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_INTERNAL_EOLF_HPP
#define TAO_PEGTL_INTERNAL_EOLF_HPP

#include "../config.hpp"

#include "skip_control.hpp"

#include "../analysis/generic.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         struct eolf
         {
            using analyze_t = analysis::generic< analysis::rule_type::opt >;

            template< typename Input >
            static bool match( Input& in ) noexcept( noexcept( Input::eol_t::match( in ) ) )
            {
               const auto p = Input::eol_t::match( in );
               return p.first || ( !p.second );
            }
         };

         template<>
         struct skip_control< eolf > : std::true_type
         {
         };

      }  // namespace internal

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
