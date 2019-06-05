// Copyright (c) 2019 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_CONTRIB_PARSE_TREE_TO_DOT_HPP
#define TAO_PEGTL_CONTRIB_PARSE_TREE_TO_DOT_HPP

#include <cassert>
#include <ostream>
#include <string>

#include "parse_tree.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace parse_tree
      {
         namespace internal
         {
            void print_dot_node( std::ostream& os, const parse_tree::node& n, const std::string& s )
            {
               if( n.has_content() ) {
                  os << "  x" << &n << " [ label=\"" << s << "\\n\\\"" << n.string() << "\\\"\" ]\n";
               }
               else {
                  os << "  x" << &n << " [ label=\"" << s << "\" ]\n";
               }
               if( !n.children.empty() ) {
                  os << "  x" << &n << " -> { ";
                  for( auto& up : n.children ) {
                     os << "x" << up.get() << ( ( up == n.children.back() ) ? " }\n" : ", " );
                  }
                  for( auto& up : n.children ) {
                     print_dot_node( os, *up, up->name() );
                  }
               }
            }

         }  // namespace internal

         void print_dot( std::ostream& os, const parse_tree::node& n )
         {
            assert( n.is_root() );
            os << "digraph parse_tree\n{\n";
            internal::print_dot_node( os, n, "ROOT" );
            os << "}\n";
         }

      }  // namespace parse_tree

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
