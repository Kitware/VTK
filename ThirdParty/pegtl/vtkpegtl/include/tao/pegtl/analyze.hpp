// Copyright (c) 2014-2018 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_ANALYZE_HPP
#define TAO_PEGTL_ANALYZE_HPP

#include "config.hpp"

#include "analysis/analyze_cycles.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      template< typename Rule >
      std::size_t analyze( const bool verbose = true )
      {
         return analysis::analyze_cycles< Rule >( verbose ).problems();
      }

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
