// Copyright (c) 2020 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_CONTRIB_REMOVE_LAST_STATES_HPP
#define TAO_PEGTL_CONTRIB_REMOVE_LAST_STATES_HPP

#include <tuple>
#include <utility>

#include "../config.hpp"
#include "../internal/integer_sequence.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      // Remove the last N states of start(), success(), failure(), raise(), apply(), and apply0()
      template< typename Base, std::size_t N >
      struct remove_last_states
         : Base
      {
         template< typename Input, typename Tuple, std::size_t... Is >
         static void start_impl( const Input& in, const Tuple& t, internal::index_sequence< Is... > /*unused*/ ) noexcept( noexcept( Base::start( in, std::get< Is >( t )... ) ) )
         {
            Base::start( in, std::get< Is >( t )... );
         }

         template< typename Input, typename... States >
         static void start( const Input& in, States&&... st ) noexcept( noexcept( start_impl( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) - N >() ) ) )
         {
            start_impl( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) - N >() );
         }

         template< typename Input, typename Tuple, std::size_t... Is >
         static void success_impl( const Input& in, const Tuple& t, internal::index_sequence< Is... > /*unused*/ ) noexcept( noexcept( Base::success( in, std::get< Is >( t )... ) ) )
         {
            Base::success( in, std::get< Is >( t )... );
         }

         template< typename Input, typename... States >
         static void success( const Input& in, States&&... st ) noexcept( noexcept( success_impl( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) - N >() ) ) )
         {
            success_impl( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) - N >() );
         }

         template< typename Input, typename Tuple, std::size_t... Is >
         static void failure_impl( const Input& in, const Tuple& t, internal::index_sequence< Is... > /*unused*/ ) noexcept( noexcept( Base::failure( in, std::get< Is >( t )... ) ) )
         {
            Base::failure( in, std::get< Is >( t )... );
         }

         template< typename Input, typename... States >
         static void failure( const Input& in, States&&... st ) noexcept( noexcept( failure_impl( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) - N >() ) ) )
         {
            failure_impl( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) - N >() );
         }

         template< typename Input, typename Tuple, std::size_t... Is >
         static void raise_impl( const Input& in, const Tuple& t, internal::index_sequence< Is... > /*unused*/ )
         {
            Base::raise( in, std::get< Is >( t )... );
         }

         template< typename Input, typename... States >
         static void raise( const Input& in, States&&... st )
         {
            raise_impl( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) - N >() );
         }

         template< template< typename... > class Action, typename Iterator, typename Input, typename Tuple, std::size_t... Is >
         static auto apply_impl( const Iterator& begin, const Input& in, const Tuple& t, internal::index_sequence< Is... > /*unused*/ ) noexcept( noexcept( Base::template apply< Action >( begin, in, std::get< Is >( t )... ) ) )
            -> decltype( Base::template apply< Action >( begin, in, std::get< Is >( t )... ) )
         {
            return Base::template apply< Action >( begin, in, std::get< Is >( t )... );
         }

         template< template< typename... > class Action, typename Iterator, typename Input, typename... States >
         static auto apply( const Iterator& begin, const Input& in, States&&... st ) noexcept( noexcept( apply_impl< Action >( begin, in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) - N >() ) ) )
            -> decltype( apply_impl< Action >( begin, in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) - N >() ) )
         {
            return apply_impl< Action >( begin, in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) - N >() );
         }

         template< template< typename... > class Action, typename Input, typename Tuple, std::size_t... Is >
         static auto apply0_impl( const Input& in, const Tuple& t, internal::index_sequence< Is... > /*unused*/ ) noexcept( noexcept( Base::template apply0< Action >( in, std::get< Is >( t )... ) ) )
            -> decltype( Base::template apply0< Action >( in, std::get< Is >( t )... ) )
         {
            return Base::template apply0< Action >( in, std::get< Is >( t )... );
         }

         template< template< typename... > class Action, typename Input, typename... States >
         static auto apply0( const Input& in, States&&... st ) noexcept( noexcept( apply0_impl< Action >( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) - N >() ) ) )
            -> decltype( apply0_impl< Action >( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) - N >() ) )
         {
            return apply0_impl< Action >( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) - N >() );
         }
      };

      template< typename Base >
      using remove_last_state = remove_last_states< Base, 1 >;

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
