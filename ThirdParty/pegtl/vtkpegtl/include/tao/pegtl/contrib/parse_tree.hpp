// Copyright (c) 2017-2020 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_CONTRIB_PARSE_TREE_HPP
#define TAO_PEGTL_CONTRIB_PARSE_TREE_HPP

#include <cassert>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <utility>
#include <vector>

#include "remove_first_state.hpp"
#include "shuffle_states.hpp"

#include "../apply_mode.hpp"
#include "../config.hpp"
#include "../memory_input.hpp"
#include "../normal.hpp"
#include "../nothing.hpp"
#include "../parse.hpp"
#include "../rewind_mode.hpp"

#include "../analysis/counted.hpp"
#include "../analysis/generic.hpp"
#include "../internal/demangle.hpp"
#include "../internal/integer_sequence.hpp"
#include "../internal/iterator.hpp"
#include "../internal/skip_control.hpp"
#include "../internal/try_catch_type.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace parse_tree
      {
         template< typename T >
         struct basic_node
         {
            using node_t = T;
            using children_t = std::vector< std::unique_ptr< node_t > >;
            children_t children;

            std::type_index id = std::type_index( typeid( void ) );
            std::string source;

            TAO_PEGTL_NAMESPACE::internal::iterator m_begin;
            TAO_PEGTL_NAMESPACE::internal::iterator m_end;

            // each node will be default constructed
            basic_node() = default;

            // no copy/move is necessary
            // (nodes are always owned/handled by a std::unique_ptr)
            basic_node( const basic_node& ) = delete;
            basic_node( basic_node&& ) = delete;

            ~basic_node() = default;

            // no assignment either
            basic_node& operator=( const basic_node& ) = delete;
            basic_node& operator=( basic_node&& ) = delete;

            bool is_root() const noexcept
            {
               return id == typeid( void );
            }

            template< typename U >
            bool is() const noexcept
            {
               return id == typeid( U );
            }

            std::string name() const
            {
               assert( !is_root() );
               return TAO_PEGTL_NAMESPACE::internal::demangle( id.name() );
            }

            position begin() const
            {
               return position( m_begin, source );
            }

            position end() const
            {
               return position( m_end, source );
            }

            bool has_content() const noexcept
            {
               return m_end.data != nullptr;
            }

            std::string string() const
            {
               assert( has_content() );
               return std::string( m_begin.data, m_end.data );
            }

            // Compatibility, remove with 3.0.0
            std::string content() const
            {
               return string();
            }

            template< tracking_mode P = tracking_mode::eager, typename Eol = eol::lf_crlf >
            memory_input< P, Eol > as_memory_input() const
            {
               assert( has_content() );
               return { m_begin.data, m_end.data, source, m_begin.byte, m_begin.line, m_begin.byte_in_line };
            }

            template< typename... States >
            void remove_content( States&&... /*unused*/ ) noexcept
            {
               m_end.reset();
            }

            // all non-root nodes are initialized by calling this method
            template< typename Rule, typename Input, typename... States >
            void start( const Input& in, States&&... /*unused*/ )
            {
               id = typeid( Rule );
               source = in.source();
               m_begin = TAO_PEGTL_NAMESPACE::internal::iterator( in.iterator() );
            }

            // if parsing of the rule succeeded, this method is called
            template< typename Rule, typename Input, typename... States >
            void success( const Input& in, States&&... /*unused*/ ) noexcept
            {
               m_end = TAO_PEGTL_NAMESPACE::internal::iterator( in.iterator() );
            }

            // if parsing of the rule failed, this method is called
            template< typename Rule, typename Input, typename... States >
            void failure( const Input& /*unused*/, States&&... /*unused*/ ) noexcept
            {
            }

            // if parsing succeeded and the (optional) transform call
            // did not discard the node, it is appended to its parent.
            // note that "child" is the node whose Rule just succeeded
            // and "*this" is the parent where the node should be appended.
            template< typename... States >
            void emplace_back( std::unique_ptr< node_t >&& child, States&&... /*unused*/ )
            {
               assert( child );
               children.emplace_back( std::move( child ) );
            }
         };

         struct node
            : basic_node< node >
         {
         };

         namespace internal
         {
            template< typename >
            struct is_try_catch_type
               : std::false_type
            {
            };

            template< typename Exception, typename... Rules >
            struct is_try_catch_type< TAO_PEGTL_NAMESPACE::internal::try_catch_type< Exception, Rules... > >
               : std::true_type
            {
            };

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
                  stack.emplace_back( std::unique_ptr< Node >( new Node ) );
               }

               std::unique_ptr< Node >& back() noexcept
               {
                  assert( !stack.empty() );
                  return stack.back();
               }

               void pop_back() noexcept
               {
                  assert( !stack.empty() );
                  return stack.pop_back();
               }
            };

            template< typename Selector, typename... Parameters >
            void transform( Parameters&&... /*unused*/ ) noexcept
            {
            }

            template< typename Selector, typename Input, typename Node, typename... States >
            auto transform( const Input& in, std::unique_ptr< Node >& n, States&&... st ) noexcept( noexcept( Selector::transform( in, n, st... ) ) )
               -> decltype( Selector::transform( in, n, st... ), void() )
            {
               Selector::transform( in, n, st... );
            }

            template< typename Selector, typename Input, typename Node, typename... States >
            auto transform( const Input& /*unused*/, std::unique_ptr< Node >& n, States&&... st ) noexcept( noexcept( Selector::transform( n, st... ) ) )
               -> decltype( Selector::transform( n, st... ), void() )
            {
               Selector::transform( n, st... );
            }

            template< typename Rule, template< typename... > class Selector >
            struct is_selected_node
               : std::integral_constant< bool, !TAO_PEGTL_NAMESPACE::internal::skip_control< Rule >::value && Selector< Rule >::value >
            {
            };

            template< unsigned Level, typename Analyse, template< typename... > class Selector >
            struct is_leaf
               : std::false_type
            {
            };

            template< analysis::rule_type Type, template< typename... > class Selector >
            struct is_leaf< 0, analysis::generic< Type >, Selector >
               : std::true_type
            {
            };

            template< analysis::rule_type Type, std::size_t Count, template< typename... > class Selector >
            struct is_leaf< 0, analysis::counted< Type, Count >, Selector >
               : std::true_type
            {
            };

            template< analysis::rule_type Type, typename... Rules, template< typename... > class Selector >
            struct is_leaf< 0, analysis::generic< Type, Rules... >, Selector >
               : std::false_type
            {
            };

            template< analysis::rule_type Type, std::size_t Count, typename... Rules, template< typename... > class Selector >
            struct is_leaf< 0, analysis::counted< Type, Count, Rules... >, Selector >
               : std::false_type
            {
            };

            template< bool... >
            struct bool_sequence;

            template< bool... Bs >
            struct is_all
               : std::is_same< bool_sequence< Bs..., true >, bool_sequence< true, Bs... > >
            {
            };

            template< bool... Bs >
            struct is_none
               : std::integral_constant< bool, !is_all< !Bs... >::value >
            {
            };

            template< unsigned Level, typename Rule, template< typename... > class Selector >
            using is_unselected_leaf = std::integral_constant< bool, !is_selected_node< Rule, Selector >::value && is_leaf< Level, typename Rule::analyze_t, Selector >::value >;

            template< unsigned Level, analysis::rule_type Type, typename... Rules, template< typename... > class Selector >
            struct is_leaf< Level, analysis::generic< Type, Rules... >, Selector >
               : is_all< is_unselected_leaf< Level - 1, Rules, Selector >::value... >
            {
            };

            template< unsigned Level, analysis::rule_type Type, std::size_t Count, typename... Rules, template< typename... > class Selector >
            struct is_leaf< Level, analysis::counted< Type, Count, Rules... >, Selector >
               : is_all< is_unselected_leaf< Level - 1, Rules, Selector >::value... >
            {
            };

            template< typename Node, template< typename... > class Selector, template< typename... > class Control >
            struct make_control
            {
               template< typename Rule, bool, bool >
               struct state_handler;

               template< typename Rule >
               using type = rotate_states_right< state_handler< Rule, is_selected_node< Rule, Selector >::value, is_leaf< 8, typename Rule::analyze_t, Selector >::value > >;
            };

            template< typename Node, template< typename... > class Selector, template< typename... > class Control >
            template< typename Rule >
            struct make_control< Node, Selector, Control >::state_handler< Rule, false, true >
               : remove_first_state< Control< Rule > >
            {
            };

            template< typename Node, template< typename... > class Selector, template< typename... > class Control >
            template< typename Rule >
            struct make_control< Node, Selector, Control >::state_handler< Rule, false, false >
               : remove_first_state< Control< Rule > >
            {
               template< apply_mode A,
                         rewind_mode M,
                         template< typename... >
                         class Action,
                         template< typename... >
                         class Control2,
                         typename Input,
                         typename... States >
               static bool match( Input& in, States&&... st )
               {
                  auto& state = std::get< sizeof...( st ) - 1 >( std::tie( st... ) );
                  if( is_try_catch_type< Rule >::value ) {
                     internal::state< Node > tmp;
                     tmp.emplace_back();
                     tmp.stack.swap( state.stack );
                     const bool result = Control< Rule >::template match< A, M, Action, Control2 >( in, st... );
                     tmp.stack.swap( state.stack );
                     if( result ) {
                        for( auto& c : tmp.back()->children ) {
                           state.back()->children.emplace_back( std::move( c ) );
                        }
                     }
                     return result;
                  }
                  state.emplace_back();
                  const bool result = Control< Rule >::template match< A, M, Action, Control2 >( in, st... );
                  if( result ) {
                     auto n = std::move( state.back() );
                     state.pop_back();
                     for( auto& c : n->children ) {
                        state.back()->children.emplace_back( std::move( c ) );
                     }
                  }
                  else {
                     state.pop_back();
                  }
                  return result;
               }
            };

            template< typename Node, template< typename... > class Selector, template< typename... > class Control >
            template< typename Rule, bool B >
            struct make_control< Node, Selector, Control >::state_handler< Rule, true, B >
               : remove_first_state< Control< Rule > >
            {
               template< typename Input, typename... States >
               static void start( const Input& in, state< Node >& state, States&&... st )
               {
                  Control< Rule >::start( in, st... );
                  state.emplace_back();
                  state.back()->template start< Rule >( in, st... );
               }

               template< typename Input, typename... States >
               static void success( const Input& in, state< Node >& state, States&&... st )
               {
                  Control< Rule >::success( in, st... );
                  auto n = std::move( state.back() );
                  state.pop_back();
                  n->template success< Rule >( in, st... );
                  transform< Selector< Rule > >( in, n, st... );
                  if( n ) {
                     state.back()->emplace_back( std::move( n ), st... );
                  }
               }

               template< typename Input, typename... States >
               static void failure( const Input& in, state< Node >& state, States&&... st ) noexcept( noexcept( Control< Rule >::failure( in, st... ) ) && noexcept( std::declval< Node& >().template failure< Rule >( in, st... ) ) )
               {
                  Control< Rule >::failure( in, st... );
                  state.back()->template failure< Rule >( in, st... );
                  state.pop_back();
               }
            };

            template< typename >
            using store_all = std::true_type;

            template< typename >
            struct selector;

            template<>
            struct selector< std::tuple<> >
            {
               using type = std::false_type;
            };

            template< typename T >
            struct selector< std::tuple< T > >
            {
               using type = typename T::type;
            };

            template< typename... Ts >
            struct selector< std::tuple< Ts... > >
            {
               static_assert( sizeof...( Ts ) == 0, "multiple matches found" );
            };

            template< typename Rule, typename Collection >
            using select_tuple = typename std::conditional< Collection::template contains< Rule >::value, std::tuple< Collection >, std::tuple<> >::type;

         }  // namespace internal

         template< typename Rule, typename... Collections >
         struct selector
            : internal::selector< decltype( std::tuple_cat( std::declval< internal::select_tuple< Rule, Collections > >()... ) ) >::type
         {};

         template< typename Base >
         struct apply
            : std::true_type
         {
            template< typename... Rules >
            struct on
            {
               using type = Base;

               template< typename Rule >
               using contains = internal::is_none< std::is_same< Rule, Rules >::value... >;
            };
         };

         struct store_content
            : apply< store_content >
         {};

         // some nodes don't need to store their content
         struct remove_content
            : apply< remove_content >
         {
            template< typename Node, typename... States >
            static void transform( std::unique_ptr< Node >& n, States&&... st ) noexcept( noexcept( n->Node::remove_content( st... ) ) )
            {
               n->remove_content( st... );
            }
         };

         // if a node has only one child, replace the node with its child, otherwise remove content
         struct fold_one
            : apply< fold_one >
         {
            template< typename Node, typename... States >
            static void transform( std::unique_ptr< Node >& n, States&&... st ) noexcept( noexcept( n->children.size(), n->Node::remove_content( st... ) ) )
            {
               if( n->children.size() == 1 ) {
                  n = std::move( n->children.front() );
               }
               else {
                  n->remove_content( st... );
               }
            }
         };

         // if a node has no children, discard the node, otherwise remove content
         struct discard_empty
            : apply< discard_empty >
         {
            template< typename Node, typename... States >
            static void transform( std::unique_ptr< Node >& n, States&&... st ) noexcept( noexcept( n->children.empty(), n->Node::remove_content( st... ) ) )
            {
               if( n->children.empty() ) {
                  n.reset();
               }
               else {
                  n->remove_content( st... );
               }
            }
         };

         template< typename Rule,
                   typename Node,
                   template< typename... > class Selector = internal::store_all,
                   template< typename... > class Action = nothing,
                   template< typename... > class Control = normal,
                   typename Input,
                   typename... States >
         std::unique_ptr< Node > parse( Input&& in, States&&... st )
         {
            internal::state< Node > state;
            if( !TAO_PEGTL_NAMESPACE::parse< Rule, Action, internal::make_control< Node, Selector, Control >::template type >( in, st..., state ) ) {
               return nullptr;
            }
            assert( state.stack.size() == 1 );
            return std::move( state.back() );
         }

         template< typename Rule,
                   template< typename... > class Selector = internal::store_all,
                   template< typename... > class Action = nothing,
                   template< typename... > class Control = normal,
                   typename Input,
                   typename... States >
         std::unique_ptr< node > parse( Input&& in, States&&... st )
         {
            return parse< Rule, node, Selector, Action, Control >( in, st... );
         }

      }  // namespace parse_tree

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
