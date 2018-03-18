// Copyright (c) 2014-2018 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_NOTHING_HPP
#define TAO_PEGTL_NOTHING_HPP

#include <type_traits>

#include "config.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      template< typename Rule >
      struct nothing
      {
      };

      template< template< typename... > class Action, typename Rule >
      using is_nothing = std::is_base_of< nothing< Rule >, Action< Rule > >;

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
