// Copyright (c) 2014-2018 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_ANALYSIS_COUNTED_HPP
#define TAO_PEGTL_ANALYSIS_COUNTED_HPP

#include "../config.hpp"

#include "generic.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace analysis
      {
         template< rule_type Type, unsigned Count, typename... Rules >
         struct counted
            : generic< ( Count != 0 ) ? Type : rule_type::OPT, Rules... >
         {
         };

      }  // namespace analysis

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
