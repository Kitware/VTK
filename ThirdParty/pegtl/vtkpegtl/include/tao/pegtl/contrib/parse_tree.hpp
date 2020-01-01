// Copyright (c) 2017-2019 Dr. Colin Hirsch and Daniel Frey
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

#include "../config.hpp"
#include "../memory_input.hpp"
#include "../normal.hpp"
#include "../nothing.hpp"
#include "../parse.hpp"

#include "../analysis/counted.hpp"
#include "../analysis/generic.hpp"
#include "../internal/demangle.hpp"
#include "../internal/integer_sequence.hpp"
#include "../internal/iterator.hpp"

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

            // this one, if applicable, is more specialized than the above
            template< typename Selector, typename Node, typename... States >
            auto transform( std::unique_ptr< Node >& n, States&&... st ) noexcept( noexcept( Selector::transform( n, st... ) ) )
               -> decltype( Selector::transform( n, st... ), void() )
            {
               Selector::transform( n, st... );
            }

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

            template< analysis::rule_type Type, unsigned Count, template< typename... > class Selector >
            struct is_leaf< 0, analysis::counted< Type, Count >, Selector >
               : std::true_type
            {
            };

            template< analysis::rule_type Type, typename... Rules, template< typename... > class Selector >
            struct is_leaf< 0, analysis::generic< Type, Rules... >, Selector >
               : std::false_type
            {
            };

            template< analysis::rule_type Type, unsigned Count, typename... Rules, template< typename... > class Selector >
            struct is_leaf< 0, analysis::counted< Type, Count, Rules... >, Selector >
               : std::false_type
            {
            };

            template< bool... >
            struct bool_sequence;

            template< bool... Bs >
            struct is_all
               : std::is_same< bool_sequence< Bs..., true >, bool_sequence< true, Bs... > >
            {};

            template< bool... Bs >
            struct is_none
               : std::integral_constant< bool, !is_all< !Bs... >::value >
            {};

            template< unsigned Level, typename Rule, template< typename... > class Selector >
            using is_unselected_leaf = std::integral_constant< bool, !Selector< Rule >::value && is_leaf< Level, typename Rule::analyze_t, Selector >::value >;

            template< unsigned Level, analysis::rule_type Type, typename... Rules, template< typename... > class Selector >
            struct is_leaf< Level, analysis::generic< Type, Rules... >, Selector >
               : is_all< is_unselected_leaf< Level - 1, Rules, Selector >::value... >
            {
            };

            template< unsigned Level, analysis::rule_type Type, unsigned Count, typename... Rules, template< typename... > class Selector >
            struct is_leaf< Level, analysis::counted< Type, Count, Rules... >, Selector >
               : is_all< is_unselected_leaf< Level - 1, Rules, Selector >::value... >
            {
            };

            template< typename T >
            struct control
            {
               template< typename Input, typename Tuple, std::size_t... Is >
               static void start_impl( const Input& in, const Tuple& t, TAO_PEGTL_NAMESPACE::internal::index_sequence< Is... > /*unused*/ ) noexcept( noexcept( T::start( in, std::get< sizeof...( Is ) >( t ), std::get< Is >( t )... ) ) )
               {
                  T::start( in, std::get< sizeof...( Is ) >( t ), std::get< Is >( t )... );
                  (void)t;
               }

               template< typename Input, typename... States >
               static void start( const Input& in, States&&... st ) noexcept( noexcept( start_impl( in, std::tie( st... ), TAO_PEGTL_NAMESPACE::internal::make_index_sequence< sizeof...( st ) - 1 >() ) ) )
               {
                  start_impl( in, std::tie( st... ), TAO_PEGTL_NAMESPACE::internal::make_index_sequence< sizeof...( st ) - 1 >() );
               }

               template< typename Input, typename Tuple, std::size_t... Is >
               static void success_impl( const Input& in, const Tuple& t, TAO_PEGTL_NAMESPACE::internal::index_sequence< Is... > /*unused*/ ) noexcept( noexcept( T::success( in, std::get< sizeof...( Is ) >( t ), std::get< Is >( t )... ) ) )
               {
                  T::success( in, std::get< sizeof...( Is ) >( t ), std::get< Is >( t )... );
                  (void)t;
               }

               template< typename Input, typename... States >
               static void success( const Input& in, States&&... st ) noexcept( noexcept( success_impl( in, std::tie( st... ), TAO_PEGTL_NAMESPACE::internal::make_index_sequence< sizeof...( st ) - 1 >() ) ) )
               {
                  success_impl( in, std::tie( st... ), TAO_PEGTL_NAMESPACE::internal::make_index_sequence< sizeof...( st ) - 1 >() );
               }

               template< typename Input, typename Tuple, std::size_t... Is >
               static void failure_impl( const Input& in, const Tuple& t, TAO_PEGTL_NAMESPACE::internal::index_sequence< Is... > /*unused*/ ) noexcept( noexcept( T::failure( in, std::get< sizeof...( Is ) >( t ), std::get< Is >( t )... ) ) )
               {
                  T::failure( in, std::get< sizeof...( Is ) >( t ), std::get< Is >( t )... );
                  (void)t;
               }

               template< typename Input, typename... States >
               static void failure( const Input& in, States&&... st ) noexcept( noexcept( failure_impl( in, std::tie( st... ), TAO_PEGTL_NAMESPACE::internal::make_index_sequence< sizeof...( st ) - 1 >() ) ) )
               {
                  failure_impl( in, std::tie( st... ), TAO_PEGTL_NAMESPACE::internal::make_index_sequence< sizeof...( st ) - 1 >() );
               }

               template< typename Input, typename Tuple, std::size_t... Is >
               static void raise_impl( const Input& in, const Tuple& t, TAO_PEGTL_NAMESPACE::internal::index_sequence< Is... > /*unused*/ ) noexcept( noexcept( T::raise( in, std::get< Is >( t )... ) ) )
               {
                  T::raise( in, std::get< Is >( t )... );
                  (void)t;
               }

               template< typename Input, typename... States >
               static void raise( const Input& in, States&&... st ) noexcept( noexcept( raise_impl( in, std::tie( st... ), TAO_PEGTL_NAMESPACE::internal::make_index_sequence< sizeof...( st ) - 1 >() ) ) )
               {
                  raise_impl( in, std::tie( st... ), TAO_PEGTL_NAMESPACE::internal::make_index_sequence< sizeof...( st ) - 1 >() );
               }

               template< template< typename... > class Action, typename Iterator, typename Input, typename Tuple, std::size_t... Is >
               static auto apply_impl( const Iterator& begin, const Input& in, const Tuple& t, TAO_PEGTL_NAMESPACE::internal::index_sequence< Is... > /*unused*/ ) noexcept( noexcept( T::template apply< Action >( begin, in, std::get< Is >( t )... ) ) )
                  -> decltype( T::template apply< Action >( begin, in, std::get< Is >( t )... ) )
               {
                  return T::template apply< Action >( begin, in, std::get< Is >( t )... );
               }

               template< template< typename... > class Action, typename Iterator, typename Input, typename... States >
               static auto apply( const Iterator& begin, const Input& in, States&&... st ) noexcept( noexcept( apply_impl< Action >( begin, in, std::tie( st... ), TAO_PEGTL_NAMESPACE::internal::make_index_sequence< sizeof...( st ) - 1 >() ) ) )
                  -> decltype( apply_impl< Action >( begin, in, std::tie( st... ), TAO_PEGTL_NAMESPACE::internal::make_index_sequence< sizeof...( st ) - 1 >() ) )
               {
                  return apply_impl< Action >( begin, in, std::tie( st... ), TAO_PEGTL_NAMESPACE::internal::make_index_sequence< sizeof...( st ) - 1 >() );
               }

               template< template< typename... > class Action, typename Input, typename Tuple, std::size_t... Is >
               static auto apply0_impl( const Input& in, const Tuple& t, TAO_PEGTL_NAMESPACE::internal::index_sequence< Is... > /*unused*/ ) noexcept( noexcept( T::template apply0< Action >( in, std::get< Is >( t )... ) ) )
                  -> decltype( T::template apply0< Action >( in, std::get< Is >( t )... ) )
               {
                  return T::template apply0< Action >( in, std::get< Is >( t )... );
               }

               template< template< typename... > class Action, typename Input, typename... States >
               static auto apply0( const Input& in, States&&... st ) noexcept( noexcept( apply0_impl< Action >( in, std::tie( st... ), TAO_PEGTL_NAMESPACE::internal::make_index_sequence< sizeof...( st ) - 1 >() ) ) )
                  -> decltype( apply0_impl< Action >( in, std::tie( st... ), TAO_PEGTL_NAMESPACE::internal::make_index_sequence< sizeof...( st ) - 1 >() ) )
               {
                  return apply0_impl< Action >( in, std::tie( st... ), TAO_PEGTL_NAMESPACE::internal::make_index_sequence< sizeof...( st ) - 1 >() );
               }

               template< apply_mode A,
                         rewind_mode M,
                         template< typename... >
                         class Action,
                         template< typename... >
                         class Control,
                         typename Input,
                         typename... States >
               static bool match( Input& in, States&&... st )
               {
                  return T::template match< A, M, Action, Control >( in, st... );
               }
            };

            template< typename Node, template< typename... > class Selector, template< typename... > class Control >
            struct make_control
            {
               template< typename Rule, bool, bool >
               struct state_handler;

               template< typename Rule >
               using type = control< state_handler< Rule, Selector< Rule >::value, is_leaf< 8, typename Rule::analyze_t, Selector >::value > >;
            };

            template< typename Node, template< typename... > class Selector, template< typename... > class Control >
            template< typename Rule >
            struct make_control< Node, Selector, Control >::state_handler< Rule, false, true >
               : Control< Rule >
            {
               template< typename Input, typename... States >
               static void start( const Input& in, state< Node >& /*unused*/, States&&... st ) noexcept( noexcept( Control< Rule >::start( in, st... ) ) )
               {
                  Control< Rule >::start( in, st... );
               }

               template< typename Input, typename... States >
               static void success( const Input& in, state< Node >& /*unused*/, States&&... st ) noexcept( noexcept( Control< Rule >::success( in, st... ) ) )
               {
                  Control< Rule >::success( in, st... );
               }

               template< typename Input, typename... States >
               static void failure( const Input& in, state< Node >& /*unused*/, States&&... st ) noexcept( noexcept( Control< Rule >::failure( in, st... ) ) )
               {
                  Control< Rule >::failure( in, st... );
               }
            };

            template< typename Node, template< typename... > class Selector, template< typename... > class Control >
            template< typename Rule >
            struct make_control< Node, Selector, Control >::state_handler< Rule, false, false >
               : Control< Rule >
            {
               template< typename Input, typename... States >
               static void start( const Input& in, state< Node >& state, States&&... st )
               {
                  Control< Rule >::start( in, st... );
                  state.emplace_back();
               }

               template< typename Input, typename... States >
               static void success( const Input& in, state< Node >& state, States&&... st )
               {
                  Control< Rule >::success( in, st... );
                  auto n = std::move( state.back() );
                  state.pop_back();
                  for( auto& c : n->children ) {
                     state.back()->children.emplace_back( std::move( c ) );
                  }
               }

               template< typename Input, typename... States >
               static void failure( const Input& in, state< Node >& state, States&&... st ) noexcept( noexcept( Control< Rule >::failure( in, st... ) ) )
               {
                  Control< Rule >::failure( in, st... );
                  state.pop_back();
               }
            };

            template< typename Node, template< typename... > class Selector, template< typename... > class Control >
            template< typename Rule, bool B >
            struct make_control< Node, Selector, Control >::state_handler< Rule, true, B >
               : Control< Rule >
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
                  transform< Selector< Rule > >( n, st... );
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
