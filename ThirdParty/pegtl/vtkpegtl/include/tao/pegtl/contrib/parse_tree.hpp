// Copyright (c) 2017-2018 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_CONTRIB_PARSE_TREE_HPP
#define TAO_PEGTL_CONTRIB_PARSE_TREE_HPP

#include <cassert>
#include <memory>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <vector>

#include "../config.hpp"
#include "../normal.hpp"
#include "../nothing.hpp"
#include "../parse.hpp"

#include "../internal/demangle.hpp"
#include "../internal/iterator.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace parse_tree
      {
         struct node
         {
            std::vector< std::unique_ptr< node > > children;

            const std::type_info* id_ = nullptr;
            internal::iterator begin_;
            internal::iterator end_;
            std::string source_;

            // each node will be default constructed
            node() = default;

            // no copy/move is necessary
            // (nodes are always owned/handled by a std::unique_ptr)
            node( const node& ) = delete;
            node( node&& ) = delete;

            ~node() = default;

            // no assignment either
            node& operator=( const node& ) = delete;
            node& operator=( node&& ) = delete;

            bool is_root() const noexcept
            {
               return id_ == nullptr;
            }

            std::string name() const
            {
               assert( !is_root() );
               return TAO_PEGTL_NAMESPACE::internal::demangle( id_->name() );
            }

            position begin() const
            {
               return position( begin_, source_ );
            }

            position end() const
            {
               return position( end_, source_ );
            }

            const std::string& source() const noexcept
            {
               return source_;
            }

            void remove_content() noexcept
            {
               end_.reset();
            }

            bool has_content() const noexcept
            {
               return end_.data != nullptr;
            }

            std::string content() const
            {
               assert( has_content() );
               return std::string( begin_.data, end_.data );
            }

            // all non-root nodes are initialized by calling this method
            template< typename Rule, typename Input >
            void start( const Input& in )
            {
               begin_ = in.iterator();
               source_ = in.source();
            }

            // if parsing of the rule succeeded, this method is called
            template< typename Rule, typename Input >
            void success( const Input& in )
            {
               id_ = &typeid( Rule );
               end_ = in.iterator();
               // idea: check for source == in.source()?
            }

            // if parsing succeeded and the (optional) transform call
            // did not discard the node, it is appended to its parent.
            // note that "child" is the node whose Rule just succeeded
            // and *this is the parent where the node should be appended.
            void append( std::unique_ptr< node > child )
            {
               assert( child );
               children.emplace_back( std::move( child ) );
            }
         };

         namespace internal
         {
            template< typename Node >
            struct state
            {
               std::vector< std::unique_ptr< Node > > stack;

               state()
               {
                  emplace_back();
               }

               void emplace_back()
               {
                  stack.emplace_back( std::unique_ptr< Node >( new Node ) );  // NOLINT: std::make_unique requires C++14
               }

               std::unique_ptr< Node >& back() noexcept
               {
                  return stack.back();
               }

               void pop_back() noexcept
               {
                  return stack.pop_back();
               }
            };

            template< typename Node, typename S, typename = void >
            struct transform
            {
               static void call( std::unique_ptr< Node >& /*unused*/ ) noexcept
               {
               }
            };

            template< typename Node, typename S >
            struct transform< Node, S, decltype( S::transform( std::declval< std::unique_ptr< Node >& >() ), void() ) >
            {
               static void call( std::unique_ptr< Node >& n ) noexcept( noexcept( S::transform( n ) ) )
               {
                  S::transform( n );
               }
            };

            template< template< typename > class S >
            struct make_control
            {
               template< typename Rule, bool = S< Rule >::value >
               struct control;

               template< typename Rule >
               using type = control< Rule >;
            };

            template< template< typename > class S >
            template< typename Rule >
            struct make_control< S >::control< Rule, false >
               : normal< Rule >
            {
            };

            template< template< typename > class S >
            template< typename Rule >
            struct make_control< S >::control< Rule, true >
               : normal< Rule >
            {
               template< typename Input, typename Node >
               static void start( const Input& in, state< Node >& st )
               {
                  st.emplace_back();
                  st.back()->template start< Rule >( in );
               }

               template< typename Input, typename Node >
               static void success( const Input& in, state< Node >& st )
               {
                  auto n = std::move( st.back() );
                  st.pop_back();
                  n->template success< Rule >( in );
                  transform< Node, S< Rule > >::call( n );
                  if( n ) {
                     st.back()->append( std::move( n ) );
                  }
               }

               template< typename Input, typename Node >
               static void failure( const Input& /*unused*/, state< Node >& st ) noexcept
               {
                  st.pop_back();
               }
            };

            template< typename >
            struct store_all : std::true_type
            {
            };

         }  // namespace internal

         // some nodes don't need to store their content,
         // use this helper as a base class to derive your specialization from.
         struct remove_content : std::true_type
         {
            template< typename Node >
            static void transform( std::unique_ptr< Node >& n ) noexcept( noexcept( n->remove_content() ) )
            {
               n->remove_content();
            }
         };

         template< typename Rule, typename Node, template< typename > class S = internal::store_all, typename Input >
         std::unique_ptr< Node > parse( Input& in )
         {
            internal::state< Node > st;
            if( !TAO_PEGTL_NAMESPACE::parse< Rule, nothing, internal::make_control< S >::template type >( in, st ) ) {
               return nullptr;
            }
            assert( st.stack.size() == 1 );
            return std::move( st.back() );
         }

         template< typename Rule, template< typename > class S = internal::store_all, typename Input >
         std::unique_ptr< node > parse( Input& in )
         {
            return parse< Rule, node, S >( in );
         }

      }  // namespace parse_tree

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
