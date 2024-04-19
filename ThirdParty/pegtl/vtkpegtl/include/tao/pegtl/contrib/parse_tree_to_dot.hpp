// Copyright (c) 2019-2020 Dr. Colin Hirsch and Daniel Frey
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
            inline void escape( std::ostream& os, const char* p, const std::size_t s )
            {
               static const char* h = "0123456789abcdef";

               const char* l = p;
               const char* const e = p + s;
               while( p != e ) {
                  const unsigned char c = *p;
                  if( c == '\\' ) {
                     os.write( l, p - l );
                     l = ++p;
                     os << "\\\\";
                  }
                  else if( c == '"' ) {
                     os.write( l, p - l );
                     l = ++p;
                     os << "\\\"";
                  }
                  else if( c < 32 ) {
                     os.write( l, p - l );
                     l = ++p;
                     switch( c ) {
                        case '\b':
                           os << "\\b";
                           break;
                        case '\f':
                           os << "\\f";
                           break;
                        case '\n':
                           os << "\\n";
                           break;
                        case '\r':
                           os << "\\r";
                           break;
                        case '\t':
                           os << "\\t";
                           break;
                        default:
                           os << "\\u00" << h[ ( c & 0xf0 ) >> 4 ] << h[ c & 0x0f ];
                     }
                  }
                  else if( c == 127 ) {
                     os.write( l, p - l );
                     l = ++p;
                     os << "\\u007f";
                  }
                  else {
                     ++p;
                  }
               }
               os.write( l, p - l );
            }

            inline void escape( std::ostream& os, const std::string& s )
            {
               escape( os, s.data(), s.size() );
            }

            template< typename Node >
            void print_dot_node( std::ostream& os, const Node& n, const std::string& s )
            {
               os << "  x" << &n << " [ label=\"";
               escape( os, s );
               if( n.has_content() ) {
                  os << "\\n";
                  escape( os, n.m_begin.data, n.m_end.data - n.m_begin.data );
               }
               os << "\" ]\n";
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

         template< typename Node >
         void print_dot( std::ostream& os, const Node& n )
         {
            os << "digraph parse_tree\n{\n";
            internal::print_dot_node( os, n, n.is_root() ? "ROOT" : n.name() );
            os << "}\n";
         }

      }  // namespace parse_tree

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
