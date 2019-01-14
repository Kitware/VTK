// Copyright (c) 2014-2018 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_INTERNAL_TRIVIAL_HPP
#define TAO_PEGTL_INTERNAL_TRIVIAL_HPP

#include "../config.hpp"

#include "skip_control.hpp"

#include "../analysis/counted.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< bool Result >
         struct trivial
         {
            using analyze_t = analysis::counted< analysis::rule_type::ANY, unsigned( !Result ) >;

            template< typename Input >
            static bool match( Input& /*unused*/ ) noexcept
            {
               return Result;
            }
         };

         template< bool Result >
         struct skip_control< trivial< Result > > : std::true_type
         {
         };

      }  // namespace internal

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
