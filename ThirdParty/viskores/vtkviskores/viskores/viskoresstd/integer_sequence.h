//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_std_integer_sequence_h
#define viskores_std_integer_sequence_h

#include <viskores/internal/Configure.h>

#include <viskores/StaticAssert.h>

#include <utility>

#if defined(__cpp_lib_integer_sequence)
#define VISKORES_USE_STD_INTEGER_SEQUENCE
#elif (__cplusplus >= 201402L)
#define VISKORES_USE_STD_INTEGER_SEQUENCE
#elif defined(VISKORES_MSVC)
#define VISKORES_USE_STD_INTEGER_SEQUENCE
#endif

#if (__cplusplus >= 201402L)
#define VISKORES_USE_STD_MAKE_INTEGER_SEQUENCE
#elif defined(VISKORES_MSVC) && (_MSC_FULL_VER >= 190023918)
#define VISKORES_USE_STD_MAKE_INTEGER_SEQUENCE
#endif

namespace viskoresstd
{

#ifndef VISKORES_USE_STD_INTEGER_SEQUENCE

template <typename T, T... Ns>
struct integer_sequence
{
  using value_type = T;

  static constexpr std::size_t size() noexcept { return sizeof...(Ns); }
};

template <std::size_t... Ns>
using index_sequence = integer_sequence<std::size_t, Ns...>;

#else // VISKORES_USE_STD_INTEGER_SEQUENCE

using std::index_sequence;
using std::integer_sequence;

#endif // VISKORES_USE_STD_INTEGER_SEQUENCE

#ifndef VISKORES_USE_STD_MAKE_INTEGER_SEQUENCE

namespace detail
{

// Implementation note: ideally these implementation classes would define "Num"
// as the type for the sequence (i.e. T). However, compilers have trouble
// resolving template partial specialization for a number whose type is one of
// the other template parameters. (Most compilers allow you to specify them in
// an integral_constant, but versions of GCC fail at that, too.) Instead, we are
// using std::size_t for the num, which should be large enough.
using SeqSizeT = std::size_t;

template <typename T, SeqSizeT Num>
struct MakeSequenceImpl;

template <typename Sequence>
struct DoubleSequence;

template <typename T, T... Ns>
struct DoubleSequence<viskoresstd::integer_sequence<T, Ns...>>
{
  using type = viskoresstd::integer_sequence<T, Ns..., T(sizeof...(Ns)) + Ns...>;
};

template <typename Sequence1, typename Sequence2>
struct CombineSequences;

template <typename T, T... N1s, T... N2s>
struct CombineSequences<viskoresstd::integer_sequence<T, N1s...>,
                        viskoresstd::integer_sequence<T, N2s...>>
{
  using type = viskoresstd::integer_sequence<T, N1s..., T(sizeof...(N1s)) + N2s...>;
};

template <bool CanDouble, SeqSizeT Num, typename Sequence>
struct ExpandSequence;

template <typename T, SeqSizeT Num, T... Ns>
struct ExpandSequence<true, Num, viskoresstd::integer_sequence<T, Ns...>>
{
  static constexpr SeqSizeT OldSize = sizeof...(Ns);
  static constexpr SeqSizeT RemainingAfter = Num - OldSize;
  static constexpr bool CanDoubleNext = RemainingAfter >= OldSize * 2;
  using type = typename ExpandSequence<
    CanDoubleNext,
    RemainingAfter,
    typename DoubleSequence<viskoresstd::integer_sequence<T, Ns...>>::type>::type;
};

template <typename T, SeqSizeT Num, T... Ns>
struct ExpandSequence<false, Num, viskoresstd::integer_sequence<T, Ns...>>
{
  using type = typename CombineSequences<viskoresstd::integer_sequence<T, Ns...>,
                                         typename MakeSequenceImpl<T, Num>::type>::type;
};

template <typename T>
struct MakeSequenceImpl<T, 0>
{
  using type = viskoresstd::integer_sequence<T>;
};
template <typename T>
struct MakeSequenceImpl<T, 1>
{
  using type = viskoresstd::integer_sequence<T, 0>;
};
template <typename T>
struct MakeSequenceImpl<T, 2>
{
  using type = viskoresstd::integer_sequence<T, 0, 1>;
};
template <typename T>
struct MakeSequenceImpl<T, 3>
{
  using type = viskoresstd::integer_sequence<T, 0, 1, 2>;
};
template <typename T>
struct MakeSequenceImpl<T, 4>
{
  using type = viskoresstd::integer_sequence<T, 0, 1, 2, 3>;
};
template <typename T>
struct MakeSequenceImpl<T, 5>
{
  using type = viskoresstd::integer_sequence<T, 0, 1, 2, 3, 4>;
};
template <typename T>
struct MakeSequenceImpl<T, 6>
{
  using type = viskoresstd::integer_sequence<T, 0, 1, 2, 3, 4, 5>;
};
template <typename T>
struct MakeSequenceImpl<T, 7>
{
  using type = viskoresstd::integer_sequence<T, 0, 1, 2, 3, 4, 5, 6>;
};

template <typename T>
struct MakeSequenceImpl<T, 8>
{
  using type = viskoresstd::integer_sequence<T, 0, 1, 2, 3, 4, 5, 6, 7>;
};
template <typename T>
struct MakeSequenceImpl<T, 9>
{
  using type = viskoresstd::integer_sequence<T, 0, 1, 2, 3, 4, 5, 6, 7, 8>;
};
template <typename T>
struct MakeSequenceImpl<T, 10>
{
  using type = viskoresstd::integer_sequence<T, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9>;
};
template <typename T>
struct MakeSequenceImpl<T, 11>
{
  using type = viskoresstd::integer_sequence<T, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10>;
};
template <typename T>
struct MakeSequenceImpl<T, 12>
{
  using type = viskoresstd::integer_sequence<T, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11>;
};
template <typename T>
struct MakeSequenceImpl<T, 13>
{
  using type = viskoresstd::integer_sequence<T, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12>;
};
template <typename T>
struct MakeSequenceImpl<T, 14>
{
  using type = viskoresstd::integer_sequence<T, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13>;
};
template <typename T>
struct MakeSequenceImpl<T, 15>
{
  using type = viskoresstd::integer_sequence<T, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14>;
};
template <typename T>
struct MakeSequenceImpl<T, 16>
{
  using type =
    viskoresstd::integer_sequence<T, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15>;
};

template <typename T, SeqSizeT Num>
struct MakeSequenceImpl
{
  VISKORES_STATIC_ASSERT(Num >= 16);
  VISKORES_STATIC_ASSERT_MSG(Num < (1 << 20), "Making an unexpectedly long integer sequence.");
  using type =
    typename ExpandSequence<(Num >= 32), Num - 16, typename MakeSequenceImpl<T, 16>::type>::type;
};

} // namespace detail

template <typename T, T N>
using make_integer_sequence =
  typename detail::MakeSequenceImpl<T, static_cast<detail::SeqSizeT>(N)>::type;

template <std::size_t N>
using make_index_sequence = make_integer_sequence<std::size_t, N>;

#else // VISKORES_USE_STD_MAKE_INTEGER_SEQUENCE

using std::make_index_sequence;
using std::make_integer_sequence;

#endif // VISKORES_USE_STD_MAKE_INTEGER_SEQUENCE

} // namespace viskoresstd

#ifdef VISKORES_USE_STD_INTEGER_SEQUENCE
#undef VISKORES_USE_STD_INTEGER_SEQUENCE
#endif

#ifdef VISKORES_USE_STD_MAKE_INTEGER_SEQUENCE
#undef VISKORES_USE_STD_MAKE_INTEGER_SEQUENCE
#endif

#endif //viskores_std_integer_sequence_h
