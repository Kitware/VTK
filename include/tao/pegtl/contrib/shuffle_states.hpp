// Copyright (c) 2020 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef TAO_PEGTL_CONTRIB_SHUFFLE_STATES_HPP
#define TAO_PEGTL_CONTRIB_SHUFFLE_STATES_HPP

#include <tuple>
#include <utility>

#include "../config.hpp"
#include "../internal/integer_sequence.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< std::size_t N >
         struct rotate_left
         {
            template< std::size_t I, std::size_t S >
            using type = std::integral_constant< std::size_t, ( I + N ) % S >;
         };

         template< std::size_t N >
         struct rotate_right
         {
            template< std::size_t I, std::size_t S >
            using type = std::integral_constant< std::size_t, ( I + S - N ) % S >;
         };

         struct reverse
         {
            template< std::size_t I, std::size_t S >
            using type = std::integral_constant< std::size_t, ( S - 1 ) - I >;
         };

      }  // namespace internal

      // Applies 'Shuffle' to the states of start(), success(), failure(), raise(), apply(), and apply0()
      template< typename Base, typename Shuffle >
      struct shuffle_states
         : Base
      {
         template< typename Input, typename Tuple, std::size_t... Is >
         static void start_impl( const Input& in, const Tuple& t, internal::index_sequence< Is... > /*unused*/ ) noexcept( noexcept( Base::start( in, std::get< Shuffle::template type< Is, sizeof...( Is ) >::value >( t )... ) ) )
         {
            Base::start( in, std::get< Shuffle::template type< Is, sizeof...( Is ) >::value >( t )... );
         }

         template< typename Input, typename... States >
         static void start( const Input& in, States&&... st ) noexcept( noexcept( start_impl( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) >() ) ) )
         {
            start_impl( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) >() );
         }

         template< typename Input, typename State >
         static void start( const Input& in, State&& st ) noexcept( noexcept( Base::start( in, st ) ) )
         {
            Base::start( in, st );
         }

         template< typename Input, typename Tuple, std::size_t... Is >
         static void success_impl( const Input& in, const Tuple& t, internal::index_sequence< Is... > /*unused*/ ) noexcept( noexcept( Base::success( in, std::get< Shuffle::template type< Is, sizeof...( Is ) >::value >( t )... ) ) )
         {
            Base::success( in, std::get< Shuffle::template type< Is, sizeof...( Is ) >::value >( t )... );
         }

         template< typename Input, typename... States >
         static void success( const Input& in, States&&... st ) noexcept( noexcept( success_impl( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) >() ) ) )
         {
            success_impl( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) >() );
         }

         template< typename Input, typename State >
         static void success( const Input& in, State&& st ) noexcept( noexcept( Base::success( in, st ) ) )
         {
            Base::success( in, st );
         }

         template< typename Input, typename Tuple, std::size_t... Is >
         static void failure_impl( const Input& in, const Tuple& t, internal::index_sequence< Is... > /*unused*/ ) noexcept( noexcept( Base::failure( in, std::get< Shuffle::template type< Is, sizeof...( Is ) >::value >( t )... ) ) )
         {
            Base::failure( in, std::get< Shuffle::template type< Is, sizeof...( Is ) >::value >( t )... );
         }

         template< typename Input, typename... States >
         static void failure( const Input& in, States&&... st ) noexcept( noexcept( failure_impl( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) >() ) ) )
         {
            failure_impl( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) >() );
         }

         template< typename Input, typename State >
         static void failure( const Input& in, State&& st ) noexcept( noexcept( Base::failure( in, st ) ) )
         {
            Base::failure( in, st );
         }

         template< typename Input, typename Tuple, std::size_t... Is >
         static void raise_impl( const Input& in, const Tuple& t, internal::index_sequence< Is... > /*unused*/ )
         {
            Base::raise( in, std::get< Shuffle::template type< Is, sizeof...( Is ) >::value >( t )... );
         }

         template< typename Input, typename... States >
         static void raise( const Input& in, States&&... st )
         {
            raise_impl( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) >() );
         }

         template< typename Input, typename State >
         static void raise( const Input& in, State&& st )
         {
            Base::raise( in, st );
         }

         template< template< typename... > class Action, typename Iterator, typename Input, typename Tuple, std::size_t... Is >
         static auto apply_impl( const Iterator& begin, const Input& in, const Tuple& t, internal::index_sequence< Is... > /*unused*/ ) noexcept( noexcept( Base::template apply< Action >( begin, in, std::get< Shuffle::template type< Is, sizeof...( Is ) >::value >( t )... ) ) )
            -> decltype( Base::template apply< Action >( begin, in, std::get< Shuffle::template type< Is, sizeof...( Is ) >::value >( t )... ) )
         {
            return Base::template apply< Action >( begin, in, std::get< Shuffle::template type< Is, sizeof...( Is ) >::value >( t )... );
         }

         template< template< typename... > class Action, typename Iterator, typename Input, typename... States >
         static auto apply( const Iterator& begin, const Input& in, States&&... st ) noexcept( noexcept( apply_impl< Action >( begin, in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) >() ) ) )
            -> decltype( apply_impl< Action >( begin, in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) >() ) )
         {
            return apply_impl< Action >( begin, in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) >() );
         }

         template< template< typename... > class Action, typename Iterator, typename Input, typename State >
         static auto apply( const Iterator& begin, const Input& in, State&& st ) noexcept( noexcept( Base::template apply< Action >( begin, in, st ) ) )
            -> decltype( Base::template apply< Action >( begin, in, st ) )
         {
            return Base::template apply< Action >( begin, in, st );
         }

         template< template< typename... > class Action, typename Input, typename Tuple, std::size_t... Is >
         static auto apply0_impl( const Input& in, const Tuple& t, internal::index_sequence< Is... > /*unused*/ ) noexcept( noexcept( Base::template apply0< Action >( in, std::get< Shuffle::template type< Is, sizeof...( Is ) >::value >( t )... ) ) )
            -> decltype( Base::template apply0< Action >( in, std::get< Shuffle::template type< Is, sizeof...( Is ) >::value >( t )... ) )
         {
            return Base::template apply0< Action >( in, std::get< Shuffle::template type< Is, sizeof...( Is ) >::value >( t )... );
         }

         template< template< typename... > class Action, typename Input, typename... States >
         static auto apply0( const Input& in, States&&... st ) noexcept( noexcept( apply0_impl< Action >( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) >() ) ) )
            -> decltype( apply0_impl< Action >( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) >() ) )
         {
            return apply0_impl< Action >( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) >() );
         }

         template< template< typename... > class Action, typename Input, typename State >
         static auto apply0( const Input& in, State&& st ) noexcept( noexcept( Base::template apply0< Action >( in, st ) ) )
            -> decltype( Base::template apply0< Action >( in, st ) )
         {
            return Base::template apply0< Action >( in, st );
         }
      };

      template< typename Base, std::size_t N = 1 >
      using rotate_states_left = shuffle_states< Base, internal::rotate_left< N > >;

      template< typename Base, std::size_t N = 1 >
      using rotate_states_right = shuffle_states< Base, internal::rotate_right< N > >;

      template< typename Base >
      using reverse_states = shuffle_states< Base, internal::reverse >;

   }  // namespace TAO_PEGTL_NAMESPACE

}  // namespace tao

#endif
