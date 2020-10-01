// Copyright (c) 2014-2020 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_ANALYSIS_COUNTED_HPP
#define TAO_PEGTL_ANALYSIS_COUNTED_HPP

#include "../config.hpp"

#include <cstddef>

#include "generic.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace analysis
      {
         template< rule_type Type, std::size_t Count, typename... Rules >
         struct counted
            : generic< ( Count != 0 ) ? Type : rule_type::opt, Rules... >
         {
         };

      }  // namespace analysis

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
