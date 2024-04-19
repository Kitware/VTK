// Copyright (c) 2014-2020 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_ANALYSIS_GENERIC_HPP
#define TAO_PEGTL_ANALYSIS_GENERIC_HPP

#include "../config.hpp"

#include "grammar_info.hpp"
#include "rule_type.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace analysis
      {
         template< rule_type Type, typename... Rules >
         struct generic
         {
            template< typename Name >
            static std::string insert( grammar_info& g )
            {
               const auto r = g.insert< Name >( Type );
               if( r.second ) {
#ifdef __cpp_fold_expressions
                  ( r.first->second.rules.push_back( Rules::analyze_t::template insert< Rules >( g ) ), ... );
#else
                  using swallow = bool[];
                  (void)swallow{ ( r.first->second.rules.push_back( Rules::analyze_t::template insert< Rules >( g ) ), true )..., true };
#endif
               }
               return r.first->first;
            }
         };

      }  // namespace analysis

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
