// Copyright (c) 2016-2018 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_INTERNAL_EOL_HPP
#define TAO_PEGTL_INTERNAL_EOL_HPP

#include "../config.hpp"

#include "skip_control.hpp"

#include "../analysis/generic.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         struct eol
         {
            using analyze_t = analysis::generic< analysis::rule_type::ANY >;

            template< typename Input >
            static bool match( Input& in ) noexcept( noexcept( Input::eol_t::match( in ) ) )
            {
               return Input::eol_t::match( in ).first;
            }
         };

         template<>
         struct skip_control< eol > : std::true_type
         {
         };

      }  // namespace internal

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
