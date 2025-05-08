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

/* The MIT License
   Copyright (c) 2016 Dinghua Li <voutcn@gmail.com>

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

#ifndef KXSORT_H__
#define KXSORT_H__

#include <algorithm>
#include <iterator>

namespace kx
{

static constexpr int kRadixBits = 8;
static constexpr size_t kInsertSortThreshold = 64;
static constexpr int kRadixMask = (1 << kRadixBits) - 1;
static constexpr int kRadixBin = 1 << kRadixBits;

//================= HELPING FUNCTIONS ====================

template <class T>
struct RadixTraitsUnsigned
{
  static constexpr int nBytes = sizeof(T);
  int kth_byte(const T& x, int k) { return (x >> (kRadixBits * k)) & kRadixMask; }
  bool compare(const T& x, const T& y) { return x < y; }
};

template <class T>
struct RadixTraitsSigned
{
  static constexpr int nBytes = sizeof(T);
  static const T kMSB = T(0x80) << ((sizeof(T) - 1) * 8);
  int kth_byte(const T& x, int k) { return ((x ^ kMSB) >> (kRadixBits * k)) & kRadixMask; }
  bool compare(const T& x, const T& y) { return x < y; }
};

template <class RandomIt, class ValueType, class RadixTraits>
inline void insert_sort_core_(RandomIt s, RandomIt e, RadixTraits radix_traits)
{
  for (RandomIt i = s + 1; i < e; ++i)
  {
    if (radix_traits.compare(*i, *(i - 1)))
    {
      RandomIt j;
      ValueType tmp = *i;
      *i = *(i - 1);
      for (j = i - 1; j > s && radix_traits.compare(tmp, *(j - 1)); --j)
      {
        *j = *(j - 1);
      }
      *j = tmp;
    }
  }
}

template <class RandomIt, class ValueType, class RadixTraits, int kWhichByte>
inline void radix_sort_core_(RandomIt s, RandomIt e, RadixTraits radix_traits)
{
  RandomIt last_[kRadixBin + 1];
  RandomIt* last = last_ + 1;
  size_t count[kRadixBin] = { 0 };

  for (RandomIt i = s; i < e; ++i)
  {
    ++count[radix_traits.kth_byte(*i, kWhichByte)];
  }

  last_[0] = last_[1] = s;

  for (int i = 1; i < kRadixBin; ++i)
  {
    last[i] = last[i - 1] + count[i - 1];
  }

  for (int i = 0; i < kRadixBin; ++i)
  {
    RandomIt end = last[i - 1] + count[i];
    if (end == e)
    {
      last[i] = e;
      break;
    }
    while (last[i] != end)
    {
      ValueType swapper = *last[i];
      int tag = radix_traits.kth_byte(swapper, kWhichByte);
      if (tag != i)
      {
        do
        {
          std::swap(swapper, *last[tag]++);
        } while ((tag = radix_traits.kth_byte(swapper, kWhichByte)) != i);
        *last[i] = swapper;
      }
      ++last[i];
    }
  }

  if (kWhichByte > 0)
  {
    for (int i = 0; i < kRadixBin; ++i)
    {
      if (count[i] > kInsertSortThreshold)
      {
        radix_sort_core_<RandomIt, ValueType, RadixTraits, (kWhichByte > 0 ? (kWhichByte - 1) : 0)>(
          last[i - 1], last[i], radix_traits);
      }
      else if (count[i] > 1)
      {
        insert_sort_core_<RandomIt, ValueType, RadixTraits>(last[i - 1], last[i], radix_traits);
      }
    }
  }
}

template <class RandomIt, class ValueType, class RadixTraits>
inline void radix_sort_entry_(RandomIt s, RandomIt e, ValueType*, RadixTraits radix_traits)
{
  if (e - s <= (int)kInsertSortThreshold)
    insert_sort_core_<RandomIt, ValueType, RadixTraits>(s, e, radix_traits);
  else
    radix_sort_core_<RandomIt, ValueType, RadixTraits, RadixTraits::nBytes - 1>(s, e, radix_traits);
}

template <class RandomIt, class ValueType>
inline void radix_sort_entry_(RandomIt s, RandomIt e, ValueType*)
{
  if (ValueType(-1) > ValueType(0))
  {
    radix_sort_entry_(s, e, (ValueType*)(0), RadixTraitsUnsigned<ValueType>());
  }
  else
  {
    radix_sort_entry_(s, e, (ValueType*)(0), RadixTraitsSigned<ValueType>());
  }
}

//================= INTERFACES ====================

template <class RandomIt, class RadixTraits>
inline void radix_sort(RandomIt s, RandomIt e, RadixTraits radix_traits)
{
  typename std::iterator_traits<RandomIt>::value_type* dummy(0);
  radix_sort_entry_(s, e, dummy, radix_traits);
}

template <class RandomIt>
inline void radix_sort(RandomIt s, RandomIt e)
{
  typename std::iterator_traits<RandomIt>::value_type* dummy(0);
  radix_sort_entry_(s, e, dummy);
}
}

#endif
