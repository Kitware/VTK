// Copyright (c) 2014-2018 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_ANALYSIS_INSERT_RULES_HPP
#define TAO_PEGTL_ANALYSIS_INSERT_RULES_HPP

#include "../config.hpp"

#include "grammar_info.hpp"
#include "rule_info.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace analysis
      {
         template< typename... >
         struct insert_rules;

         template<>
         struct insert_rules<>
         {
            static void insert( grammar_info& /*unused*/, rule_info& /*unused*/ )
            {
            }
         };

         template< typename Rule, typename... Rules >
         struct insert_rules< Rule, Rules... >
         {
            static void insert( grammar_info& g, rule_info& r )
            {
               r.rules.push_back( Rule::analyze_t::template insert< Rule >( g ) );
               insert_rules< Rules... >::insert( g, r );
            }
         };

      }  // namespace analysis

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
