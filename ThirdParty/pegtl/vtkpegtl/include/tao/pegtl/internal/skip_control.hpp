// Copyright (c) 2014-2020 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_INTERNAL_SKIP_CONTROL_HPP
#define TAO_PEGTL_INTERNAL_SKIP_CONTROL_HPP

#include <type_traits>

#include "../config.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         // This class is a simple tagging mechanism.
         // By default, skip_control< Rule >::value
         // is 'false'. Each internal (!) rule that should
         // be hidden from the control and action class'
         // callbacks simply specializes skip_control<>
         // to return 'true' for the above expression.

         template< typename Rule >
         struct skip_control : std::false_type
         {
         };

      }  // namespace internal

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
