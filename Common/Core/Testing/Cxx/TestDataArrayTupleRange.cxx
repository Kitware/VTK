/*==============================================================================

  Program:   Visualization Toolkit
  Module:    TestDataArrayRange.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

==============================================================================*/

#include "vtkDataArrayRange.h"

#include "vtkAOSDataArrayTemplate.h"
#include "vtkDataArray.h"
#include "vtkFloatArray.h"
#include "vtkSOADataArrayTemplate.h"
#ifdef VTK_USE_SCALED_SOA_ARRAYS
#include "vtkScaledSOADataArrayTemplate.h"
#endif
#include "vtkVector.h"
#include "vtkVectorOperators.h"

#include <algorithm>
#include <numeric>
#include <type_traits>
#include <utility>

namespace
{

std::size_t NumErrors = 0;

#define TO_STRING(x) TO_STRING2(x)
#define TO_STRING2(x) #x
#define LOCATION() "line " TO_STRING(__LINE__) ""

#define CHECK_TYPEDEF(t1, t2)                                                                      \
  static_assert(std::is_same<typename std::decay<t1>::type, typename std::decay<t2>::type>{},      \
    "Type mismatch: '" #t1 "' not same as '" #t2 "' in " LOCATION())

#define CHECK_IS_BASE_TYPE_OF(t1, t2)                                                              \
  static_assert(std::is_base_of<typename std::decay<t1>::type, typename std::decay<t2>::type>{},   \
    "Type mismatch: '" #t1 "' not same as '" #t2 "' in " LOCATION())

// Various properties required by random access iterators:
#define CHECK_ITER_TYPE(type)                                                                      \
  static_assert(std::is_default_constructible<Iter>::value,                                        \
    "Iterator types must be default constructable at " LOCATION());                                \
  static_assert(std::is_copy_constructible<Iter>::value,                                           \
    "Iterator types must be copy constructible at " LOCATION());                                   \
  static_assert(std::is_copy_assignable<Iter>::value,                                              \
    "Iterator types must be copy assignable at " LOCATION());                                      \
  static_assert(                                                                                   \
    std::is_destructible<Iter>::value, "Iterator types must be destructable at " LOCATION())

#define LOG_ERROR(message)                                                                         \
  ++NumErrors;                                                                                     \
  std::cerr << NumErrors << ": " << message << "\n"

#define CHECK_TRUE(expr)                                                                           \
  do                                                                                               \
  {                                                                                                \
    if (!(expr))                                                                                   \
    {                                                                                              \
      LOG_ERROR("Expression not true: '" #expr << "' at " LOCATION());                             \
    }                                                                                              \
  } while (false)

#define CHECK_FALSE(expr)                                                                          \
  do                                                                                               \
  {                                                                                                \
    if ((expr))                                                                                    \
    {                                                                                              \
      LOG_ERROR("Expression expected to be false but is true: '" #expr << "' at " LOCATION());     \
    }                                                                                              \
  } while (false)

#define CHECK_EQUAL(v1, v2)                                                                        \
  do                                                                                               \
  {                                                                                                \
    if (!(v1 == v2))                                                                               \
    {                                                                                              \
      LOG_ERROR("Expressions not equal: '" #v1 "' (" << v1 << ") and '" #v2 "' (" << v2            \
                                                     << ") in " LOCATION());                       \
    }                                                                                              \
  } while (false)

#define CHECK_NOT_EQUAL(v1, v2)                                                                    \
  do                                                                                               \
  {                                                                                                \
    if (!(v1 != v2))                                                                               \
    {                                                                                              \
      LOG_ERROR("Expressions not equal: '" #v1 "' (" << v1 << ") and '" #v2 "' (" << v2            \
                                                     << ") in " LOCATION());                       \
    }                                                                                              \
  } while (false)

#define CHECK_EQUAL_NODUMP(v1, v2)                                                                 \
  do                                                                                               \
  {                                                                                                \
    if (!(v1 == v2))                                                                               \
    {                                                                                              \
      LOG_ERROR("Expressions not equal: '" #v1 "' and '" #v2 "' in " LOCATION());                  \
    }                                                                                              \
  } while (false)

#define CHECK_NOT_EQUAL_NODUMP(v1, v2)                                                             \
  do                                                                                               \
  {                                                                                                \
    if (!(v1 != v2))                                                                               \
    {                                                                                              \
      LOG_ERROR(                                                                                   \
        "Expressions should be unequal but aren't: '" #v1 "' and '" #v2 "' in " LOCATION());       \
    }                                                                                              \
  } while (false)

//==============================================================================
//==============================================================================
// Helpers:
//==============================================================================
//==============================================================================
template <typename Range>
void FillTupleRangeIota(Range range)
{
  using ComponentType = typename Range::ComponentType;

  ComponentType value{ 1 };
  for (typename Range::reference tuple : range)
  {
    std::iota(tuple.begin(), tuple.end(), value);
    value += static_cast<ComponentType>(tuple.size());
  }
}

template <typename Range>
void TestIota(Range range)
{
  using ComponentType = typename Range::ComponentType;

  auto numComps = range.GetTupleSize();
  auto beginTuple = range.GetBeginTupleId();
  auto endTuple = range.GetEndTupleId();

  auto startValue = static_cast<ComponentType>(beginTuple * numComps) + 1;
  auto endValue = static_cast<ComponentType>(endTuple * numComps) + 1;

  auto value = startValue;

  for (typename Range::const_reference tuple : range)
  {
    for (ComponentType comp : tuple)
    {
      CHECK_EQUAL(value, comp);
      ++value;
    }
  }

  CHECK_EQUAL(value, endValue);
}

//==============================================================================
//==============================================================================
// TupleRange:
//==============================================================================
//==============================================================================
template <typename ArrayType>
struct UnitTestTupleRangeAPI
{
  static constexpr vtk::ComponentIdType NumComps = 3;
  static constexpr vtk::TupleIdType NumTuples = 12;

  void operator()()
  {
    vtkNew<ArrayType> array;
    auto da = static_cast<vtkDataArray*>(array);
    array->SetNumberOfComponents(NumComps);

    this->TestEmptyRange(vtk::DataArrayTupleRange(array));
    this->TestEmptyRange(vtk::DataArrayTupleRange(da));
    this->TestEmptyRange(vtk::DataArrayTupleRange<NumComps>(array));
    this->TestEmptyRange(vtk::DataArrayTupleRange<NumComps>(da));

    array->SetNumberOfTuples(this->NumTuples);

    this->TestEmptyRange(vtk::DataArrayTupleRange(array, 4, 4));
    this->TestEmptyRange(vtk::DataArrayTupleRange(da, 4, 4));
    this->TestEmptyRange(vtk::DataArrayTupleRange<NumComps>(array, 4, 4));
    this->TestEmptyRange(vtk::DataArrayTupleRange<NumComps>(da, 4, 4));

    FillTupleRangeIota(vtk::DataArrayTupleRange<NumComps>(array));

    auto pStart = NumTuples / 4;
    auto pEnd = NumTuples / 4 * 3;

    { // Full, dynamic-size, real typed range
      auto range = vtk::DataArrayTupleRange(array);
      DispatchRangeTests<ArrayType, vtk::detail::DynamicTupleSize>(range, array, 0, NumTuples);
    }
    { // Full, dynamic-size, generic-typed range
      auto range = vtk::DataArrayTupleRange(da);
      DispatchRangeTests<vtkDataArray, vtk::detail::DynamicTupleSize>(range, array, 0, NumTuples);
    }
    { // Full, fixed-size, real typed range
      auto range = vtk::DataArrayTupleRange<NumComps>(array);
      DispatchRangeTests<ArrayType, NumComps>(range, array, 0, NumTuples);
    }
    { // Full, fixed-size, generic-typed range
      auto range = vtk::DataArrayTupleRange<NumComps>(da);
      DispatchRangeTests<vtkDataArray, NumComps>(range, array, 0, NumTuples);
    }
    { // Partial, dynamic-size, real typed range
      auto range = vtk::DataArrayTupleRange(array, pStart, pEnd);
      DispatchRangeTests<ArrayType, vtk::detail::DynamicTupleSize>(range, array, pStart, pEnd);
    }
    { // Partial, dynamic-size, generic-typed range
      auto range = vtk::DataArrayTupleRange(da, pStart, pEnd);
      DispatchRangeTests<vtkDataArray, vtk::detail::DynamicTupleSize>(range, array, pStart, pEnd);
    }
    { // Partial, fixed-size, real typed range
      auto range = vtk::DataArrayTupleRange<NumComps>(array, pStart, pEnd);
      DispatchRangeTests<ArrayType, NumComps>(range, array, pStart, pEnd);
    }
    { // Partial, fixed-size, generic-typed range
      auto range = vtk::DataArrayTupleRange<NumComps>(da, pStart, pEnd);
      DispatchRangeTests<vtkDataArray, NumComps>(range, array, pStart, pEnd);
    }
  }

  template <typename Range>
  void TestEmptyRange(Range range)
  {
    for (typename Range::const_reference tuple : range)
    {
      for (typename Range::ComponentType comp : tuple)
      {
        (void)comp;
        CHECK_TRUE(false && "This should not execute.");
      }
    }
  }

  template <typename RangeArrayType, vtk::ComponentIdType RangeTupleSize, typename Range>
  void DispatchRangeTests(
    Range range, RangeArrayType* array, vtk::TupleIdType start, vtk::TupleIdType end)
  {
    {
      TestRange<RangeArrayType, RangeTupleSize>(range, array, start, end);
      TestSubRange(range);
      TestDefaultInit(range);
    }

    {
      const Range& crange = range;
      TestRange<RangeArrayType, RangeTupleSize>(crange, array, start, end);
      TestSubRange(crange);
    }
  }

  template <typename RangeArrayType, vtk::ComponentIdType RangeTupleSize, typename Range>
  void TestRange(Range& range, RangeArrayType* array, vtk::TupleIdType start, vtk::TupleIdType end)
  {
    TestTypes<RangeArrayType, RangeTupleSize>(range);

    CHECK_EQUAL(range.GetArray(), array);
    CHECK_EQUAL(range.GetTupleSize(), NumComps);
    CHECK_EQUAL(range.GetBeginTupleId(), start);
    CHECK_EQUAL(range.GetEndTupleId(), end);
    CHECK_EQUAL(range.size(), end - start);
    CHECK_EQUAL(range.end() - range.begin(), range.size());
    CHECK_EQUAL(range.cend() - range.cbegin(), range.size());
    CHECK_EQUAL_NODUMP(*range.begin(), range[0]);
    CHECK_EQUAL_NODUMP(*(range.begin() + 1), range[1]);

    TestIota(range);
  }

  template <typename RangeArrayType, vtk::ComponentIdType RangeTupleSize, typename Range>
  void TestTypes(Range& range)
  {
    using ConstRange = typename std::add_const<Range>::type;
    using MutableRange = typename std::remove_const<Range>::type;
    (void)range; // decltype doesn't actually use the value.

    CHECK_TYPEDEF(
      typename Range::TupleIteratorType, decltype(std::declval<MutableRange>().begin()));
    CHECK_TYPEDEF(
      typename Range::ConstTupleIteratorType, decltype(std::declval<MutableRange>().cbegin()));
    CHECK_TYPEDEF(
      typename Range::ConstTupleIteratorType, decltype(std::declval<ConstRange>().begin()));
    CHECK_TYPEDEF(
      typename Range::ConstTupleIteratorType, decltype(std::declval<ConstRange>().cbegin()));
    CHECK_TYPEDEF(
      typename Range::TupleReferenceType, decltype(*std::declval<MutableRange>().begin()));
    CHECK_TYPEDEF(typename Range::TupleReferenceType, decltype(std::declval<MutableRange>()[0]));
    CHECK_TYPEDEF(
      typename Range::ConstTupleReferenceType, decltype(*std::declval<MutableRange>().cbegin()));
    CHECK_TYPEDEF(typename Range::ConstTupleReferenceType, decltype(std::declval<ConstRange>()[0]));
    CHECK_TYPEDEF(typename Range::ComponentIteratorType,
      decltype(std::declval<MutableRange>().begin()->begin()));
    CHECK_TYPEDEF(typename Range::ConstComponentIteratorType,
      decltype(std::declval<MutableRange>().cbegin()->begin()));
    CHECK_TYPEDEF(typename Range::ConstComponentIteratorType,
      decltype(std::declval<MutableRange>().begin()->cbegin()));
    CHECK_TYPEDEF(typename Range::ComponentReferenceType,
      decltype(*std::declval<MutableRange>().begin()->begin()));
    CHECK_TYPEDEF(
      typename Range::ComponentReferenceType, decltype(std::declval<MutableRange>()[0][0]));
    CHECK_TYPEDEF(typename Range::ConstComponentReferenceType,
      decltype(*std::declval<MutableRange>().cbegin()->begin()));
    CHECK_TYPEDEF(typename Range::ConstComponentReferenceType,
      decltype(*std::declval<MutableRange>().begin()->cbegin()));
    CHECK_TYPEDEF(
      typename Range::ConstComponentReferenceType, decltype(std::declval<ConstRange>()[0][0]));

    CHECK_IS_BASE_TYPE_OF(typename Range::ArrayType, RangeArrayType);
    CHECK_TYPEDEF(typename Range::ComponentType, vtk::GetAPIType<RangeArrayType>);
    CHECK_TYPEDEF(typename Range::size_type, vtk::TupleIdType);
    CHECK_TYPEDEF(typename Range::size_type, decltype(range.size()));
    CHECK_TYPEDEF(typename Range::reference, decltype(*std::declval<typename Range::iterator>()));
    CHECK_TYPEDEF(
      typename Range::const_reference, decltype(*std::declval<typename Range::const_iterator>()));
    CHECK_TYPEDEF(typename Range::reference, decltype(std::declval<MutableRange>()[0]));
    CHECK_TYPEDEF(typename Range::const_reference, decltype(std::declval<ConstRange>()[0]));
    CHECK_TYPEDEF(typename Range::iterator, decltype(std::declval<MutableRange>().begin()));
    CHECK_TYPEDEF(typename Range::iterator, decltype(std::declval<MutableRange>().end()));
    CHECK_TYPEDEF(typename Range::const_iterator, decltype(std::declval<ConstRange>().begin()));
    CHECK_TYPEDEF(typename Range::const_iterator, decltype(std::declval<ConstRange>().end()));
    CHECK_TYPEDEF(typename Range::const_iterator, decltype(range.cbegin()));
    CHECK_TYPEDEF(typename Range::const_iterator, decltype(range.cend()));
    CHECK_TYPEDEF(typename Range::ArrayType, decltype(*range.GetArray()));
    CHECK_TYPEDEF(vtk::TupleIdType, decltype(range.GetBeginTupleId()));
    CHECK_TYPEDEF(vtk::TupleIdType, decltype(range.GetEndTupleId()));

    static_assert(Range::TupleSizeTag == RangeTupleSize, "Range::TupleSizeTag incorrect.");
  }

  template <typename Range>
  void TestSubRange(Range& range)
  {
    auto range1 = range.GetSubRange(3, 9);
    CHECK_EQUAL(range1.GetBeginTupleId(), range.GetBeginTupleId() + 3);
    CHECK_EQUAL(range1.GetEndTupleId(), range.GetBeginTupleId() + 9);
    {
      auto subRange = range1.GetSubRange();
      CHECK_EQUAL(subRange.GetBeginTupleId(), range1.GetBeginTupleId());
      CHECK_EQUAL(subRange.GetEndTupleId(), range1.GetEndTupleId());
    }
    {
      auto subRange = range1.GetSubRange(2, 4);
      CHECK_EQUAL(subRange.GetBeginTupleId(), range1.GetBeginTupleId() + 2);
      CHECK_EQUAL(subRange.GetEndTupleId(), range1.GetBeginTupleId() + 4);
    }
    {
      auto subRange = range1.GetSubRange(1);
      CHECK_EQUAL(subRange.GetBeginTupleId(), range1.GetBeginTupleId() + 1);
      CHECK_EQUAL(subRange.GetEndTupleId(), range1.GetEndTupleId());
    }
    {
      auto subRange = range1.GetSubRange(0, 5);
      CHECK_EQUAL(subRange.GetBeginTupleId(), range1.GetBeginTupleId());
      CHECK_EQUAL(subRange.GetEndTupleId(), range1.GetBeginTupleId() + 5);
    }
    {
      auto subRange = range1.GetSubRange(0, 0);
      CHECK_EQUAL(subRange.GetBeginTupleId(), range1.GetBeginTupleId());
      CHECK_EQUAL(subRange.GetEndTupleId(), range1.GetBeginTupleId());
    }
  }

  template <typename Range>
  void TestDefaultInit(Range& range)
  {
    auto range1 = Range{};
    range1 = range;
    (void)range;
  }
};

//==============================================================================
//==============================================================================
// TupleIterators:
//==============================================================================
//==============================================================================
template <typename ArrayType>
struct UnitTestTupleIteratorAPI
{
  static constexpr vtk::ComponentIdType NumComps = 3;
  static constexpr vtk::TupleIdType NumTuples = 12;

  void operator()()
  {
    vtkNew<ArrayType> array;
    array->SetNumberOfComponents(NumComps);
    array->SetNumberOfTuples(NumTuples);
    FillTupleRangeIota(vtk::DataArrayTupleRange<NumComps>(array));

    auto da = static_cast<vtkDataArray*>(array);

    { // Full, dynamic-size, real typed range
      auto range = vtk::DataArrayTupleRange(array);
      DispatchRangeTests(range);
    }
    { // Full, dynamic-size, generic-typed range
      auto range = vtk::DataArrayTupleRange(da);
      DispatchRangeTests(range);
    }
    { // Full, fixed-size, real typed range
      auto range = vtk::DataArrayTupleRange<NumComps>(array);
      DispatchRangeTests(range);
    }
    { // Full, fixed-size, generic-typed range
      auto range = vtk::DataArrayTupleRange<NumComps>(da);
      DispatchRangeTests(range);
    }
  }

  template <typename Range>
  void DispatchRangeTests(Range range)
  {
    {
      TestTupleIterator(range);
      TestConstTupleIterator(range);
    }

    {
      const Range& crange = range;
      TestConstTupleIterator(crange);
    }
  }

  template <typename Range>
  void TestTupleIterator(Range& range)
  {
    TestConstCopy(range);
    TestConstAssign(range);
    TestIterSwap(range);
  }

  template <typename Range>
  void TestConstTupleIterator(Range& range)
  {
    TestTypes(range);
    TestCopy(range);
    TestAssign(range);
    TestTraversal(range);
    TestIndexing(range);
    TestDeref(range);
    TestComparison(range);
    TestSwap(range);
  }

  template <typename Range>
  void TestTypes(Range& range)
  {
    using Iter = decltype(range.begin());
    (void)range; // decltype doesn't actually use values

    CHECK_TYPEDEF(typename Iter::iterator_category, std::random_access_iterator_tag);
    CHECK_TYPEDEF(typename Iter::reference, decltype(*std::declval<Iter>()));
    CHECK_TYPEDEF(typename Iter::reference, decltype(std::declval<Iter>()[0]));
    CHECK_TYPEDEF(typename Iter::pointer, decltype(std::declval<Iter>().operator->()));
    CHECK_TYPEDEF(
      typename Iter::difference_type, decltype(std::declval<Iter>() - std::declval<Iter>()));

    // Check requirements of random-access iterators:
    CHECK_ITER_TYPE(Iter);
  }

  template <typename Range>
  void TestConstCopy(Range& range)
  {
    // This should only get called with non-const ranges:
    static_assert(!std::is_const<Range>::value, "Expected mutable range.");

    // We should be able to implicitly cast and compare mutable iterators to
    // const ones:
    typename Range::iterator iter{ range.begin() };
    typename Range::const_iterator citer{ iter };
    CHECK_EQUAL_NODUMP(iter, citer);
  }

  template <typename Range>
  void TestConstAssign(Range& range)
  {
    // This should only get called with non-const ranges:
    static_assert(!std::is_const<Range>::value, "Expected mutable range.");

    // We should be able to implicitly cast and compare mutable objects to
    // const ones:
    typename Range::iterator iter{ range.begin() };
    typename Range::const_iterator citer{ range.cend() };

    citer = iter;
    CHECK_EQUAL_NODUMP(iter, citer);
  }

  template <typename Range>
  void TestCopy(Range& range)
  {
    auto iter = this->GetTestingIter(range);

    using IterType = decltype(iter);
    IterType iter2(iter);

    CHECK_EQUAL_NODUMP(iter, iter2);
    CHECK_EQUAL_NODUMP(*iter, *iter2);
  }

  template <typename Range>
  void TestAssign(Range& range)
  {
    auto iter = this->GetTestingIter(range);

    using IterType = decltype(iter);
    IterType iter2 = range.begin();

    CHECK_NOT_EQUAL_NODUMP(iter, iter2);
    CHECK_NOT_EQUAL_NODUMP(*iter, *iter2);

    IterType iter3 = iter2 = iter;

    CHECK_EQUAL_NODUMP(iter, iter2);
    CHECK_EQUAL_NODUMP(*iter, *iter2);
    CHECK_EQUAL_NODUMP(iter, iter3);
    CHECK_EQUAL_NODUMP(*iter, *iter3);
  }

  template <typename Range>
  void TestTraversal(Range& range)
  {
    { // operator++ (prefix)
      auto iter1 = this->GetTestingIter(range);
      auto iter2 = iter1;
      auto iter3 = ++iter2;

      CHECK_NOT_EQUAL_NODUMP(iter1, iter2);
      CHECK_NOT_EQUAL_NODUMP(iter1, iter3);
      CHECK_EQUAL_NODUMP(iter2, iter3);
      CHECK_EQUAL(iter2 - iter1, 1);
    }

    { // operator++ (postfix)
      auto iter1 = this->GetTestingIter(range);
      auto iter2 = iter1;
      auto iter3 = iter2++;

      CHECK_NOT_EQUAL_NODUMP(iter1, iter2);
      CHECK_EQUAL_NODUMP(iter1, iter3);
      CHECK_NOT_EQUAL_NODUMP(iter2, iter3);
      CHECK_EQUAL(iter2 - iter1, 1);
    }

    { // operator-- (prefix)
      auto iter1 = this->GetTestingIter(range);
      auto iter2 = iter1;
      auto iter3 = --iter2;

      CHECK_NOT_EQUAL_NODUMP(iter1, iter2);
      CHECK_NOT_EQUAL_NODUMP(iter1, iter3);
      CHECK_EQUAL_NODUMP(iter2, iter3);
      CHECK_EQUAL(iter2 - iter1, -1);
    }

    { // operator-- (postfix)
      auto iter1 = this->GetTestingIter(range);
      auto iter2 = iter1;
      auto iter3 = iter2--;

      CHECK_NOT_EQUAL_NODUMP(iter1, iter2);
      CHECK_EQUAL_NODUMP(iter1, iter3);
      CHECK_NOT_EQUAL_NODUMP(iter2, iter3);
      CHECK_EQUAL(iter2 - iter1, -1);
    }

    { // operator +=
      auto iter1 = this->GetTestingIter(range);
      auto iter2 = iter1;
      iter2 += 2;

      CHECK_NOT_EQUAL_NODUMP(iter1, iter2);
      CHECK_EQUAL(iter2 - iter1, 2);
    }

    { // operator -=
      auto iter1 = this->GetTestingIter(range);
      auto iter2 = iter1;
      iter2 -= 2;

      CHECK_NOT_EQUAL_NODUMP(iter1, iter2);
      CHECK_EQUAL(iter2 - iter1, -2);
    }

    { // operator + (it, off)
      auto iter1 = this->GetTestingIter(range);
      auto iter2 = iter1 + 2;

      CHECK_NOT_EQUAL_NODUMP(iter1, iter2);
      CHECK_EQUAL(iter2 - iter1, 2);
    }

    { // operator + (off, it)
      auto iter1 = this->GetTestingIter(range);
      auto iter2 = 2 + iter1;

      CHECK_NOT_EQUAL_NODUMP(iter1, iter2);
      CHECK_EQUAL(iter2 - iter1, 2);
    }

    { // operator - (it, off)
      auto iter1 = this->GetTestingIter(range);
      auto iter2 = iter1 - 2;

      CHECK_NOT_EQUAL_NODUMP(iter1, iter2);
      CHECK_EQUAL(iter2 - iter1, -2);
    }

    { // operator - (it, it)
      auto iter1 = this->GetTestingIter(range);
      auto iter2 = iter1;
      CHECK_EQUAL(iter2 - iter1, 0);

      iter2++;
      CHECK_EQUAL(iter2 - iter1, 1);

      iter2--;
      CHECK_EQUAL(iter2 - iter1, 0);

      --iter2;
      CHECK_EQUAL(iter2 - iter1, -1);

      iter2 += 3;
      CHECK_EQUAL(iter2 - iter1, 2);
    }
  }

  template <typename Range>
  void TestIndexing(Range& range)
  {
    auto beginIter = range.begin(); // Always points to the first tuple
    auto lagIter = beginIter;       // Points to the i-1 tuple

    using IndexT = typename Range::size_type;

    IndexT i = 0;
    for (auto tupleRef : range)
    {
      if (i == 0)
      {
        CHECK_EQUAL_NODUMP(tupleRef, lagIter[0]);
      }
      else
      {
        CHECK_EQUAL_NODUMP(tupleRef, lagIter[1]);
        ++lagIter;
      }
      CHECK_EQUAL_NODUMP(tupleRef, beginIter[i]);
      ++i;
    }
  }

  template <typename Range>
  void TestDeref(Range& range)
  {
    { // operator*
      auto iter = this->GetTestingIter(range);
      auto iter2 = iter;

      CHECK_EQUAL_NODUMP((*iter), (*iter2));

      ++iter2;
      CHECK_NOT_EQUAL_NODUMP((*iter), (*iter2));
    }

    { // operator->
      auto iter = this->GetTestingIter(range);
      auto iter2 = iter;

      CHECK_EQUAL_NODUMP(iter->begin(), iter2->begin());
      CHECK_EQUAL_NODUMP(*iter, *iter2);

      ++iter2;
      CHECK_NOT_EQUAL_NODUMP(*iter, *iter2);
    }
  }

  template <typename Range>
  void TestComparison(Range& range)
  {
    { // operator ==
      auto iter1 = this->GetTestingIter(range);
      auto iter2 = iter1;

      CHECK_TRUE(iter1 == iter2);
      ++iter2;
      CHECK_FALSE(iter1 == iter2);
    }

    { // operator !=
      auto iter1 = this->GetTestingIter(range);
      auto iter2 = iter1;

      CHECK_FALSE(iter1 != iter2);
      ++iter2;
      CHECK_TRUE(iter1 != iter2);
    }

    { // operator <
      auto iter1 = this->GetTestingIter(range);
      auto iter2 = iter1 + 1;

      CHECK_TRUE(iter1 < iter2);
      CHECK_FALSE(iter2 < iter1);
    }

    { // operator >
      auto iter1 = this->GetTestingIter(range);
      auto iter2 = iter1 - 1;

      CHECK_TRUE(iter1 > iter2);
      CHECK_FALSE(iter2 > iter1);
    }

    { // operator <=
      auto iter1 = this->GetTestingIter(range);
      auto iter2 = iter1;

      CHECK_TRUE(iter1 <= iter2);
      CHECK_TRUE(iter2 <= iter1);
      ++iter2;
      CHECK_TRUE(iter1 <= iter2);
      CHECK_FALSE(iter2 <= iter1);
    }

    { // operator >=
      auto iter1 = this->GetTestingIter(range);
      auto iter2 = iter1;

      CHECK_TRUE(iter1 >= iter2);
      CHECK_TRUE(iter2 >= iter1);
      --iter2;
      CHECK_TRUE(iter1 >= iter2);
      CHECK_FALSE(iter2 >= iter1);
    }
  }

  template <typename Range>
  void TestSwap(Range& range)
  {
    auto iter1 = this->GetTestingIter(range);
    auto iter2 = iter1 + 1;

    auto iterA = iter1;
    auto iterB = iter2;

    CHECK_NOT_EQUAL_NODUMP(iterA, iterB);
    CHECK_EQUAL_NODUMP(iterA, iter1);
    CHECK_EQUAL_NODUMP(iterB, iter2);

    { // ADL swap:
      using namespace std;
      swap(iterA, iterB);
    }

    CHECK_NOT_EQUAL_NODUMP(iterA, iter1);
    CHECK_NOT_EQUAL_NODUMP(iterB, iter2);
    CHECK_EQUAL_NODUMP(iterA, iter2);
    CHECK_EQUAL_NODUMP(iterB, iter1);
  }

  template <typename Range>
  void TestIterSwap(Range& range)
  {
    using ComponentType = typename Range::ComponentType;

    auto iter1 = this->GetTestingIter(range);
    auto iter2 = iter1 + 1;

    CHECK_FALSE(std::equal(iter1->begin(), iter1->end(), iter2->begin()));

    std::vector<ComponentType> tuple1{ iter1->begin(), iter1->end() };
    std::vector<ComponentType> tuple2{ iter2->begin(), iter2->end() };

    CHECK_TRUE(std::equal(iter1->begin(), iter1->end(), tuple1.begin()));
    CHECK_TRUE(std::equal(iter2->begin(), iter2->end(), tuple2.begin()));

    std::iter_swap(iter1, iter2);

    CHECK_TRUE(std::equal(iter1->begin(), iter1->end(), tuple2.begin()));
    CHECK_TRUE(std::equal(iter2->begin(), iter2->end(), tuple1.begin()));

    // Put things back how we found them:
    std::iter_swap(iter1, iter2);

    CHECK_TRUE(std::equal(iter1->begin(), iter1->end(), tuple1.begin()));
    CHECK_TRUE(std::equal(iter2->begin(), iter2->end(), tuple2.begin()));
  }

  // Guaranteed to be valid and surrounded by at least 2 other valid iterators
  // in each direction.
  template <typename Range>
  static auto GetTestingIter(Range& range) -> decltype(range.begin())
  {
    return range.begin() + (range.size() / 2);
  }
};

//==============================================================================
//==============================================================================
// TupleReference:
//==============================================================================
//==============================================================================
template <typename ArrayType>
struct UnitTestTupleReferenceAPI
{
  static constexpr vtk::ComponentIdType NumComps = 3;
  static constexpr vtk::TupleIdType NumTuples = 12;

  void operator()()
  {
    vtkNew<ArrayType> array;
    array->SetNumberOfComponents(NumComps);
    array->SetNumberOfTuples(NumTuples);
    FillTupleRangeIota(vtk::DataArrayTupleRange<NumComps>(array));

    auto da = static_cast<vtkDataArray*>(array);

    { // Full, dynamic-size, real typed range
      auto range = vtk::DataArrayTupleRange(array);
      DispatchRangeTests(range);
    }
    { // Full, dynamic-size, generic-typed range
      auto range = vtk::DataArrayTupleRange(da);
      DispatchRangeTests(range);
    }
    { // Full, fixed-size, real typed range
      auto range = vtk::DataArrayTupleRange<NumComps>(array);
      DispatchRangeTests(range);
    }
    { // Full, fixed-size, generic-typed range
      auto range = vtk::DataArrayTupleRange<NumComps>(da);
      DispatchRangeTests(range);
    }
  }

  template <typename Range>
  void DispatchRangeTests(Range range)
  {
    {
      TestTupleReference(range);
      TestConstTupleReference(range);
      TestMutableTypes(range); // This test is ONLY for non-const types.
    }

    {
      const Range& crange = range;
      TestConstTupleReference(crange);
      TestConstTypes(crange); // This test is ONLY for const types.
    }
  }

  template <typename Range>
  void TestTupleReference(Range& range)
  {
    TestConstCopy(range);
    TestConstAssign(range);
    TestAssign(range);
    TestIndexing(range);
    TestSwap(range);
    TestFill(range);
    TestIters(range);
    TestArrayAccess(range);
  }

  template <typename Range>
  void TestConstTupleReference(Range& range)
  {
    TestTypes(range);
    TestComparison(range);
    TestIndexingConst(range);
    TestSize(range);
    TestItersConst(range);
    TestArrayAccessConst(range);
  }

  template <typename Range>
  void TestTypes(Range& range)
  {
    using RangeArrayType = typename Range::ArrayType;
    using Ref = typename std::decay<decltype(*this->GetTestingIterator(range))>::type;
    (void)range; // decltype doesn't actually use values

    CHECK_TYPEDEF(vtk::GetAPIType<RangeArrayType>, typename Ref::value_type);
    CHECK_TYPEDEF(typename Ref::size_type, vtk::ComponentIdType);
    CHECK_TYPEDEF(typename Ref::iterator, decltype(std::declval<Ref>().begin()));
    CHECK_TYPEDEF(typename Ref::iterator, decltype(std::declval<Ref>().end()));
    CHECK_TYPEDEF(typename Ref::const_iterator, decltype(std::declval<Ref>().cbegin()));
    CHECK_TYPEDEF(typename Ref::const_iterator, decltype(std::declval<Ref>().cend()));
    CHECK_TYPEDEF(typename Ref::size_type, decltype(std::declval<Ref>().size()));
  }

  template <typename Range>
  void TestMutableTypes(Range& range)
  {
    using Ref = typename std::decay<decltype(*this->GetTestingIterator(range))>::type;
    using MutableRef = typename std::remove_const<Ref>::type;
    using ConstRef = typename std::add_const<Ref>::type;
    (void)range; // decltype doesn't actually use values

    CHECK_TYPEDEF(typename Ref::reference, decltype(std::declval<MutableRef>()[0]));
    CHECK_TYPEDEF(typename Ref::const_reference, decltype(std::declval<ConstRef>()[0]));
  }

  template <typename Range>
  void TestConstTypes(Range& range)
  {
    using Ref = typename std::decay<decltype(*this->GetTestingIterator(range))>::type;
    (void)range; // decltype doesn't actually use values

    CHECK_TYPEDEF(typename Ref::const_reference, decltype(std::declval<Ref>()[0]));
  }

  template <typename Range>
  void TestConstCopy(Range& range)
  {
    // This should only get called with non-const ranges:
    static_assert(!std::is_const<Range>::value, "Expected mutable range.");

    // We should be able to implicitly cast and compare mutable iterators to
    // const ones:
    typename Range::reference ref{ range[0] };
    typename Range::const_reference cref{ ref };
    CHECK_EQUAL_NODUMP(ref, cref);
  }

  template <typename Range>
  void TestConstAssign(Range& range)
  {
    // This should only get called with non-const ranges:
    static_assert(!std::is_const<Range>::value, "Expected mutable range.");

    // We should be able to implicitly cast and compare mutable iterators to
    // const ones:
    typename Range::reference ref{ range[0] };
    typename Range::const_reference cref{ range[1] };

    auto data = this->StoreTuple(ref);

    ref = cref;
    CHECK_EQUAL_NODUMP(ref, cref);

    this->RestoreTuple(ref, data);
  }

  template <typename Range>
  void TestAssign(Range& range)
  {
    typename Range::reference tuple1 = range[0];
    typename Range::reference tuple2 = range[1];
    const typename Range::const_reference tuple3 = range[2];

    auto data1 = this->StoreTuple(tuple1);
    auto data2 = this->StoreTuple(tuple2);
    auto data3 = this->StoreTuple(tuple3);

    CHECK_TRUE(this->CompareTuple(tuple1, data1));
    CHECK_TRUE(this->CompareTuple(tuple2, data2));
    CHECK_TRUE(this->CompareTuple(tuple3, data3));

    CHECK_FALSE(tuple1 == tuple2);
    CHECK_FALSE(tuple2 == tuple3);
    CHECK_FALSE(tuple1 == tuple3);

    tuple1 = tuple2 = tuple3;

    CHECK_TRUE(tuple1 == tuple2);
    CHECK_TRUE(tuple2 == tuple3);
    CHECK_TRUE(tuple1 == tuple3);

    CHECK_TRUE(this->CompareTuple(tuple1, data3));
    CHECK_TRUE(this->CompareTuple(tuple2, data3));
    CHECK_TRUE(this->CompareTuple(tuple3, data3));

    this->RestoreTuple(tuple1, data1);
    this->RestoreTuple(tuple2, data2);

    CHECK_TRUE(this->CompareTuple(tuple1, data1));
    CHECK_TRUE(this->CompareTuple(tuple2, data2));
    CHECK_TRUE(this->CompareTuple(tuple3, data3));
  }

  template <typename Range>
  void TestSwap(Range& range)
  {
    typename Range::reference tuple1 = range[0];
    typename Range::reference tuple2 = range[1];

    auto data1 = this->StoreTuple(tuple1);
    auto data2 = this->StoreTuple(tuple2);

    CHECK_TRUE(this->CompareTuple(tuple1, data1));
    CHECK_TRUE(this->CompareTuple(tuple2, data2));
    CHECK_FALSE(this->CompareTuple(tuple1, data2));
    CHECK_FALSE(this->CompareTuple(tuple2, data1));

    tuple1.swap(tuple2);

    CHECK_FALSE(this->CompareTuple(tuple1, data1));
    CHECK_FALSE(this->CompareTuple(tuple2, data2));
    CHECK_TRUE(this->CompareTuple(tuple1, data2));
    CHECK_TRUE(this->CompareTuple(tuple2, data1));

    tuple2.swap(tuple1);

    CHECK_TRUE(this->CompareTuple(tuple1, data1));
    CHECK_TRUE(this->CompareTuple(tuple2, data2));
    CHECK_FALSE(this->CompareTuple(tuple1, data2));
    CHECK_FALSE(this->CompareTuple(tuple2, data1));

    {
      using std::swap;
      swap(tuple1, tuple2);
    }

    CHECK_FALSE(this->CompareTuple(tuple1, data1));
    CHECK_FALSE(this->CompareTuple(tuple2, data2));
    CHECK_TRUE(this->CompareTuple(tuple1, data2));
    CHECK_TRUE(this->CompareTuple(tuple2, data1));

    {
      using std::swap;
      swap(tuple1, tuple2);
    }

    CHECK_TRUE(this->CompareTuple(tuple1, data1));
    CHECK_TRUE(this->CompareTuple(tuple2, data2));
    CHECK_FALSE(this->CompareTuple(tuple1, data2));
    CHECK_FALSE(this->CompareTuple(tuple2, data1));

    this->RestoreTuple(tuple1, data1);
    this->RestoreTuple(tuple2, data2);

    CHECK_TRUE(this->CompareTuple(tuple1, data1));
    CHECK_TRUE(this->CompareTuple(tuple2, data2));
  }

  template <typename Range>
  void TestFill(Range& range)
  {
    using ComponentType = typename Range::ComponentType;

    auto iter = this->GetTestingIterator(range);
    typename Range::reference tuple = *iter;
    auto data = this->StoreTuple(tuple);

    ComponentType fillValue = static_cast<ComponentType>(0);
    auto isFilled = [&](ComponentType val) -> bool { return val == fillValue; };

    do
    {
      tuple.fill(fillValue);
      CHECK_TRUE(std::all_of(tuple.begin(), tuple.end(), isFilled));
      ++fillValue;
    } while (fillValue < 32);

    this->RestoreTuple(tuple, data);
  }

  template <typename Range>
  void TestComparison(Range& range)
  {
    auto iter1a = this->GetTestingIterator(range, 0);
    auto iter1b = this->GetTestingIterator(range, 0);
    auto iter2 = this->GetTestingIterator(range, 1);
    auto tuple1a = *iter1a;
    auto tuple1b = *iter1b;
    auto tuple2 = *iter2;

    CHECK_EQUAL_NODUMP(tuple1a, tuple1b);
    CHECK_EQUAL_NODUMP(tuple1b, tuple1a);
    CHECK_NOT_EQUAL_NODUMP(tuple1a, tuple2);
    CHECK_NOT_EQUAL_NODUMP(tuple1b, tuple2);
    CHECK_NOT_EQUAL_NODUMP(tuple2, tuple1a);
    CHECK_NOT_EQUAL_NODUMP(tuple2, tuple1b);
  }

  template <typename Range>
  void TestIndexing(Range& range)
  {
    // NOTE Since these are proxy iterators, they don't play nicely with
    // auto/auto&. Similar to vector<bool>, the actual value types may not
    // exist in memory -- when the ArrayType is vtkDataArray, the doubles
    // returns are usually cast from the underlying ValueTypes on the fly.
    //
    // This means we must explicitly use the value_type, reference or
    // const_reference typedefs on containers, or
    // std::iterator_traits<IterT>::value_type /
    // std::iterator_traits<IterT>::reference on iterators when calling
    // operator[].

    using ConstRange = typename std::add_const<Range>::type;

    auto iter = this->GetTestingIterator(range, 0);
    auto citer = this->GetTestingIterator(const_cast<ConstRange&>(range), 1);

    auto tuple = *iter;
    auto ctuple = *citer;

    auto data1 = this->StoreTuple(tuple);

    using TupleT = decltype(tuple);
    using CTupleT = decltype(ctuple);
    using TIndex = typename TupleT::size_type;

    CHECK_NOT_EQUAL(tuple.size(), 0);
    CHECK_EQUAL(tuple.size(), ctuple.size());

    { // Read non-const:
      TIndex i = 0;
      for (typename TupleT::reference ref : tuple)
      {
        CHECK_EQUAL(tuple[i++], ref);
      }
      CHECK_EQUAL(i, tuple.size());

      i = 0;
      for (typename TupleT::const_reference cref : tuple)
      {
        CHECK_EQUAL(tuple[i++], cref);
      }
      CHECK_EQUAL(i, tuple.size());

      i = 0;
      for (typename TupleT::value_type val : tuple)
      {
        CHECK_EQUAL(tuple[i++], val);
      }
      CHECK_EQUAL(i, tuple.size());
    }

    { // Read const:
      TIndex i = 0;
      for (typename CTupleT::const_reference cref : ctuple)
      {
        CHECK_EQUAL(ctuple[i++], cref);
      }
      CHECK_EQUAL(i, ctuple.size());

      i = 0;
      for (typename CTupleT::value_type val : ctuple)
      {
        CHECK_EQUAL(ctuple[i++], val);
      }
      CHECK_EQUAL(i, ctuple.size());
    }

    { // Write:
      TIndex i = 0;
      for (typename TupleT::const_reference cref : ctuple)
      {
        tuple[i++] = cref;
      }
      CHECK_EQUAL(i, ctuple.size());
      CHECK_EQUAL_NODUMP(tuple, ctuple);
      this->RestoreTuple(tuple, data1);
    }

    this->RestoreTuple(tuple, data1);
  }

  template <typename Range>
  void TestIndexingConst(Range& range)
  {
    // NOTE Since these are proxy iterators, they don't play nicely with
    // auto/auto&. Similar to vector<bool>, the actual value types may not
    // exist in memory -- when the ArrayType is vtkDataArray, the doubles
    // returns are usually cast from the underlying ValueTypes on the fly.
    //
    // This means we must explicitly use the value_type, reference or
    // const_reference typedefs on containers, or
    // std::iterator_traits<IterT>::value_type /
    // std::iterator_traits<IterT>::reference on iterators when calling
    // operator[].

    auto iter = this->GetTestingIterator(range, 0);
    auto tuple = *iter;

    using TupleT = decltype(tuple);
    using TIndex = typename TupleT::size_type;

    CHECK_NOT_EQUAL(tuple.size(), 0);

    { // Read:
      TIndex i = 0;
      for (typename TupleT::const_reference cref : tuple)
      {
        CHECK_EQUAL(tuple[i++], cref);
      }
      CHECK_EQUAL(i, tuple.size());

      i = 0;
      for (typename TupleT::value_type val : tuple)
      {
        CHECK_EQUAL(tuple[i++], val);
      }
      CHECK_EQUAL(i, tuple.size());
    }
  }

  template <typename Range>
  void TestSize(Range& range)
  {
    CHECK_EQUAL(range.size(), range.end() - range.begin());
  }

  template <typename Range>
  void TestIters(Range& range)
  {
    using ComponentType = typename Range::ComponentType;

    auto iter1 = this->GetTestingIterator(range, 0);
    auto iter2 = this->GetTestingIterator(range, 1);
    typename Range::reference tuple1 = *iter1;
    const typename Range::const_reference tuple2 = *iter2;

    auto data1 = this->StoreTuple(tuple1);

    const ComponentType startValue1 = *tuple1.begin();
    const ComponentType startValue2 = *tuple2.begin();
    const ComponentType endValue1 = startValue1 + range.GetTupleSize();
    const ComponentType endValue2 = startValue2 + range.GetTupleSize();

    using TupleT = typename std::decay<decltype(tuple1)>::type;
    using CTupleT = typename std::decay<decltype(tuple2)>::type;

    typename TupleT::iterator b1 = tuple1.begin();
    typename TupleT::iterator e1 = tuple1.end();
    typename TupleT::const_iterator b1c = tuple1.cbegin();
    typename TupleT::const_iterator e1c = tuple1.cend();

    typename CTupleT::const_iterator b2 = tuple2.begin();
    typename CTupleT::const_iterator e2 = tuple2.end();
    typename CTupleT::const_iterator b2c = tuple2.cbegin();
    typename CTupleT::const_iterator e2c = tuple2.cend();

    CHECK_NOT_EQUAL(tuple1.size(), 0);
    CHECK_EQUAL(tuple1.size(), tuple2.size());
    CHECK_EQUAL(tuple1.size(), e1 - b1);
    CHECK_EQUAL(tuple1.size(), e1c - b1c);
    CHECK_EQUAL(tuple2.size(), e2 - b2);
    CHECK_EQUAL(tuple2.size(), e2c - b2c);

    // Read:
    {
      ComponentType value = startValue1;
      for (auto it = b1; it != e1; ++it)
      {
        CHECK_EQUAL(*it, value);
        ++value;
      }
      CHECK_EQUAL(value, endValue1);
    }
    {
      ComponentType value = startValue1;
      for (auto it = b1c; it != e1c; ++it)
      {
        CHECK_EQUAL(*it, value);
        ++value;
      }
      CHECK_EQUAL(value, endValue1);
    }
    {
      ComponentType value = startValue2;
      for (auto it = b2; it != e2; ++it)
      {
        CHECK_EQUAL(*it, value);
        ++value;
      }
      CHECK_EQUAL(value, endValue2);
    }
    {
      ComponentType value = startValue2;
      for (auto it = b2c; it != e2c; ++it)
      {
        CHECK_EQUAL(*it, value);
        ++value;
      }
      CHECK_EQUAL(value, endValue2);
    }

    // Write:
    {
      auto in = b2;
      auto out = b1;
      while (in < e2 && out < e1)
      {
        *out++ = *in++;
      }
      CHECK_EQUAL_NODUMP(tuple1, tuple2);
      this->RestoreTuple(tuple1, data1);
    }
    {
      auto in = b2c;
      auto out = b1;
      while (in < e2c && out < e1)
      {
        *out++ = *in++;
      }
      CHECK_EQUAL_NODUMP(tuple1, tuple2);
      this->RestoreTuple(tuple1, data1);
    }

    this->RestoreTuple(tuple1, data1);
  }

  template <typename Range>
  void TestItersConst(Range& range)
  {
    using ComponentType = typename Range::ComponentType;

    auto iter1 = this->GetTestingIterator(range, 0);
    auto iter2 = this->GetTestingIterator(range, 1);
    typename Range::const_reference tuple1 = *iter1;
    const typename Range::const_reference tuple2 = *iter2;

    const ComponentType startValue1 = *tuple1.begin();
    const ComponentType startValue2 = *tuple2.begin();
    const ComponentType endValue1 = startValue1 + range.GetTupleSize();
    const ComponentType endValue2 = startValue2 + range.GetTupleSize();

    using TupleT = typename std::decay<decltype(tuple1)>::type;
    using CTupleT = typename std::decay<decltype(tuple2)>::type;

    typename TupleT::iterator b1 = tuple1.begin();
    typename TupleT::iterator e1 = tuple1.end();
    typename TupleT::const_iterator b1c = tuple1.cbegin();
    typename TupleT::const_iterator e1c = tuple1.cend();

    typename CTupleT::const_iterator b2 = tuple2.begin();
    typename CTupleT::const_iterator e2 = tuple2.end();
    typename CTupleT::const_iterator b2c = tuple2.cbegin();
    typename CTupleT::const_iterator e2c = tuple2.cend();

    CHECK_NOT_EQUAL(tuple1.size(), 0);
    CHECK_EQUAL(tuple1.size(), tuple2.size());
    CHECK_EQUAL(tuple1.size(), e1 - b1);
    CHECK_EQUAL(tuple1.size(), e1c - b1c);
    CHECK_EQUAL(tuple2.size(), e2 - b2);
    CHECK_EQUAL(tuple2.size(), e2c - b2c);

    // Read:
    {
      ComponentType value = startValue1;
      for (auto it = b1; it != e1; ++it)
      {
        CHECK_EQUAL(*it, value);
        ++value;
      }
      CHECK_EQUAL(value, endValue1);
    }
    {
      ComponentType value = startValue1;
      for (auto it = b1c; it != e1c; ++it)
      {
        CHECK_EQUAL(*it, value);
        ++value;
      }
      CHECK_EQUAL(value, endValue1);
    }
    {
      ComponentType value = startValue2;
      for (auto it = b2; it != e2; ++it)
      {
        CHECK_EQUAL(*it, value);
        ++value;
      }
      CHECK_EQUAL(value, endValue2);
    }
    {
      ComponentType value = startValue2;
      for (auto it = b2c; it != e2c; ++it)
      {
        CHECK_EQUAL(*it, value);
        ++value;
      }
      CHECK_EQUAL(value, endValue2);
    }
  }

  template <typename Range>
  void TestArrayAccess(Range& range)
  {
    auto iter1 = this->GetTestingIterator(range, 0);
    auto iter2 = this->GetTestingIterator(range, 1);
    auto iter3 = this->GetTestingIterator(range, 2);

    typename Range::reference tuple1Ref = *iter1;
    typename Range::reference tuple2Ref = *iter2;
    const typename Range::const_reference tuple3Ref = *iter3;

    auto d1 = this->StoreTuple(tuple1Ref);
    auto d2 = this->StoreTuple(tuple2Ref);
    auto d3 = this->StoreTuple(tuple3Ref);

    using VectorType = vtkVector<typename Range::ComponentType, NumComps>;

    VectorType v1;
    VectorType v2;
    VectorType v3;

    tuple1Ref.GetTuple(v1.GetData());
    tuple2Ref.GetTuple(v2.GetData());
    tuple3Ref.GetTuple(v3.GetData());

    CHECK_TRUE(this->CompareTuple(v1.GetData(), v1.GetData() + NumComps, d1));
    CHECK_TRUE(this->CompareTuple(v2.GetData(), v2.GetData() + NumComps, d2));
    CHECK_TRUE(this->CompareTuple(v3.GetData(), v3.GetData() + NumComps, d3));

    CHECK_FALSE(v1 == v2);
    CHECK_FALSE(v2 == v3);
    CHECK_FALSE(v1 == v3);

    v1 = v2 = v3;

    // TupleValues are equal:
    CHECK_TRUE(v1 == v2);
    CHECK_TRUE(v2 == v3);
    CHECK_TRUE(v1 == v3);
    CHECK_TRUE(this->CompareTuple(v1.GetData(), v1.GetData() + NumComps, d3));
    CHECK_TRUE(this->CompareTuple(v2.GetData(), v2.GetData() + NumComps, d3));
    CHECK_TRUE(this->CompareTuple(v3.GetData(), v3.GetData() + NumComps, d3));

    // But references are not
    CHECK_TRUE(this->CompareTuple(tuple1Ref, d1));
    CHECK_TRUE(this->CompareTuple(tuple2Ref, d2));
    CHECK_TRUE(this->CompareTuple(tuple3Ref, d3));

    // Write a tuple to the references to modify the array:
    tuple1Ref.SetTuple(v3.GetData());
    tuple2Ref.SetTuple(v3.GetData());

    // References are now equal
    CHECK_TRUE(this->CompareTuple(tuple1Ref, d3));
    CHECK_TRUE(this->CompareTuple(tuple2Ref, d3));
    CHECK_TRUE(this->CompareTuple(tuple3Ref, d3));

    this->RestoreTuple(tuple1Ref, d1);
    this->RestoreTuple(tuple2Ref, d2);

    CHECK_TRUE(this->CompareTuple(tuple1Ref, d1));
    CHECK_TRUE(this->CompareTuple(tuple2Ref, d2));
    CHECK_TRUE(this->CompareTuple(tuple3Ref, d3));
  }

  template <typename Range>
  void TestArrayAccessConst(Range& range)
  {
    auto iter1 = this->GetTestingIterator(range, 0);
    auto iter2 = this->GetTestingIterator(range, 1);
    auto iter3 = this->GetTestingIterator(range, 2);

    typename Range::const_reference tuple1Ref = *iter1;
    typename Range::const_reference tuple2Ref = *iter2;
    const typename Range::const_reference tuple3Ref = *iter3;

    auto d1 = this->StoreTuple(tuple1Ref);
    auto d2 = this->StoreTuple(tuple2Ref);
    auto d3 = this->StoreTuple(tuple3Ref);

    using VectorType = vtkVector<typename Range::ComponentType, NumComps>;

    VectorType v1;
    VectorType v2;
    VectorType v3;

    tuple1Ref.GetTuple(v1.GetData());
    tuple2Ref.GetTuple(v2.GetData());
    tuple3Ref.GetTuple(v3.GetData());

    CHECK_TRUE(this->CompareTuple(v1.GetData(), v1.GetData() + NumComps, d1));
    CHECK_TRUE(this->CompareTuple(v2.GetData(), v2.GetData() + NumComps, d2));
    CHECK_TRUE(this->CompareTuple(v3.GetData(), v3.GetData() + NumComps, d3));

    CHECK_FALSE(v1 == v2);
    CHECK_FALSE(v2 == v3);
    CHECK_FALSE(v1 == v3);

    v1 = v2 = v3;

    // TupleValues are equal:
    CHECK_TRUE(v1 == v2);
    CHECK_TRUE(v2 == v3);
    CHECK_TRUE(v1 == v3);
    CHECK_TRUE(this->CompareTuple(v1.GetData(), v1.GetData() + NumComps, d3));
    CHECK_TRUE(this->CompareTuple(v2.GetData(), v2.GetData() + NumComps, d3));
    CHECK_TRUE(this->CompareTuple(v3.GetData(), v3.GetData() + NumComps, d3));

    // But references are not
    CHECK_TRUE(this->CompareTuple(tuple1Ref, d1));
    CHECK_TRUE(this->CompareTuple(tuple2Ref, d2));
    CHECK_TRUE(this->CompareTuple(tuple3Ref, d3));
  }

  // Returns a valid reference. tupleOffset allows different tuples to be
  // obtained. The valid range for offset is (-2, 2).
  template <typename Range>
  static auto GetTestingIterator(Range& range, vtk::TupleIdType tupleOffset = 0)
    -> decltype(range.begin())
  {
    return range.begin() + (range.size() / 2) + tupleOffset;
  }

  template <typename RefType>
  static auto StoreTuple(const RefType& ref) -> std::vector<typename RefType::value_type>
  {
    return std::vector<typename RefType::value_type>{ ref.begin(), ref.end() };
  }

  template <typename RefType>
  static void RestoreTuple(RefType& ref, const std::vector<typename RefType::value_type>& data)
  {
    std::copy(data.begin(), data.end(), ref.begin());
  }

  template <typename RefType>
  static bool CompareTuple(
    const RefType& ref, const std::vector<typename RefType::value_type>& data)
  {
    return static_cast<std::size_t>(ref.size()) == data.size() &&
      std::equal(data.begin(), data.end(), ref.begin());
  }

  template <typename IterType>
  static bool CompareTuple(IterType begin, IterType end,
    const std::vector<typename std::iterator_traits<IterType>::value_type>& data)
  {
    return static_cast<std::size_t>(std::distance(begin, end)) == data.size() &&
      std::equal(data.begin(), data.end(), begin);
  }
};

template <typename ArrayType>
struct UnitTestComponentIteratorAPI
{
  static constexpr vtk::ComponentIdType NumComps = 3;
  static constexpr vtk::TupleIdType NumTuples = 12;

  void operator()()
  {
    vtkNew<ArrayType> array;
    array->SetNumberOfComponents(NumComps);
    array->SetNumberOfTuples(NumTuples);
    FillTupleRangeIota(vtk::DataArrayTupleRange<NumComps>(array));

    auto da = static_cast<vtkDataArray*>(array);

    { // Full, dynamic-size, real typed range
      auto range = vtk::DataArrayTupleRange(array);
      DispatchRangeTests(range);
    }
    { // Full, dynamic-size, generic-typed range
      auto range = vtk::DataArrayTupleRange(da);
      DispatchRangeTests(range);
    }
    { // Full, fixed-size, real typed range
      auto range = vtk::DataArrayTupleRange<NumComps>(array);
      DispatchRangeTests(range);
    }
    { // Full, fixed-size, generic-typed range
      auto range = vtk::DataArrayTupleRange<NumComps>(da);
      DispatchRangeTests(range);
    }
  }

  template <typename Range>
  void DispatchRangeTests(Range range)
  {
    {
      TestComponentIterator(range);
      TestConstComponentIterator(range);
    }

    {
      const Range& crange = range;
      TestConstComponentIterator(crange);
    }
  }

  template <typename Range>
  void TestTypes(Range& range)
  {
    using Iter = decltype(this->GetTestingIterRange(range)->begin());
    (void)range;

    CHECK_ITER_TYPE(Iter);
  }

  template <typename Range>
  void TestComponentIterator(Range& range)
  {
    TestConstCopy(range);
    TestConstAssign(range);
    TestDeref(range);
    TestIndexing(range);
    TestIterSwap(range);
  }

  template <typename Range>
  void TestConstComponentIterator(Range& range)
  {
    TestTypes(range);
    TestCopy(range);
    TestAssign(range);
    TestTraversal(range);
    TestDerefConst(range);
    TestComparison(range);
    TestIndexingConst(range);
    TestSwap(range);
  }

  template <typename Range>
  void TestConstCopy(Range& range)
  {
    // This should only get called with non-const ranges:
    static_assert(!std::is_const<Range>::value, "Expected mutable range.");

    const Range crange = range;

    auto tuple = this->GetTestingIterRange(range);
    auto ctuple = this->GetTestingIterRange(crange);

    using IterT = decltype(tuple->begin());
    using CIterT = decltype(ctuple->begin());

    // We should be able to implicitly cast and compare mutable objects to
    // const ones:
    IterT iter{ tuple->begin() };
    CIterT citer{ iter };
    CHECK_EQUAL_NODUMP(iter, citer);
  }

  template <typename Range>
  void TestConstAssign(Range& range)
  {
    // This should only get called with non-const ranges:
    static_assert(!std::is_const<Range>::value, "Expected mutable range.");

    const Range crange = range;

    auto tuple = this->GetTestingIterRange(range);
    auto ctuple = this->GetTestingIterRange(crange);

    using IterT = decltype(tuple->begin());
    using CIterT = decltype(ctuple->begin());

    // We should be able to implicitly cast and compare mutable objects to
    // const ones:
    IterT iter{ tuple->begin() };
    CIterT citer{ ctuple->cend() };

    citer = iter;
    CHECK_EQUAL_NODUMP(iter, citer);
  }

  template <typename Range>
  void TestCopy(Range& range)
  {
    auto tuple = this->GetTestingIterRange(range);
    auto iter = tuple->begin();

    using IterType = decltype(iter);
    IterType iter2{ iter };

    CHECK_EQUAL_NODUMP(iter, iter2);
  }

  template <typename Range>
  void TestAssign(Range& range)
  {
    auto tuple = this->GetTestingIterRange(range);
    auto iter = tuple->begin();

    auto iter2 = iter + 1;
    auto iter3 = iter + 2;

    CHECK_NOT_EQUAL_NODUMP(iter, iter2);
    CHECK_NOT_EQUAL_NODUMP(iter, iter3);
    CHECK_NOT_EQUAL_NODUMP(iter2, iter3);

    iter2 = iter3 = iter;

    CHECK_EQUAL_NODUMP(iter, iter2);
    CHECK_EQUAL_NODUMP(iter, iter3);
    CHECK_EQUAL_NODUMP(iter2, iter3);
  }

  template <typename Range>
  void TestTraversal(Range& range)
  {
    { // operator++ (prefix)
      auto iter1 = this->GetTestingIter(range);
      auto iter2 = iter1;
      auto iter3 = ++iter2;

      CHECK_NOT_EQUAL_NODUMP(iter1, iter2);
      CHECK_NOT_EQUAL_NODUMP(iter1, iter3);
      CHECK_EQUAL_NODUMP(iter2, iter3);
      CHECK_EQUAL(iter2 - iter1, 1);
    }

    { // operator++ (postfix)
      auto iter1 = this->GetTestingIter(range);
      auto iter2 = iter1;
      auto iter3 = iter2++;

      CHECK_NOT_EQUAL_NODUMP(iter1, iter2);
      CHECK_EQUAL_NODUMP(iter1, iter3);
      CHECK_NOT_EQUAL_NODUMP(iter2, iter3);
      CHECK_EQUAL(iter2 - iter1, 1);
    }

    { // operator-- (prefix)
      auto iter1 = this->GetTestingIter(range);
      auto iter2 = iter1;
      auto iter3 = --iter2;

      CHECK_NOT_EQUAL_NODUMP(iter1, iter2);
      CHECK_NOT_EQUAL_NODUMP(iter1, iter3);
      CHECK_EQUAL_NODUMP(iter2, iter3);
      CHECK_EQUAL(iter2 - iter1, -1);
    }

    { // operator-- (postfix)
      auto iter1 = this->GetTestingIter(range);
      auto iter2 = iter1;
      auto iter3 = iter2--;

      CHECK_NOT_EQUAL_NODUMP(iter1, iter2);
      CHECK_EQUAL_NODUMP(iter1, iter3);
      CHECK_NOT_EQUAL_NODUMP(iter2, iter3);
      CHECK_EQUAL(iter2 - iter1, -1);
    }

    { // operator +=
      auto iter1 = this->GetTestingIter(range) - 1;
      auto iter2 = iter1 + 2;

      CHECK_NOT_EQUAL_NODUMP(iter1, iter2);
      CHECK_EQUAL(iter2 - iter1, 2);

      iter1 += 2;

      CHECK_EQUAL_NODUMP(iter1, iter2);
    }

    { // operator -=
      auto iter1 = this->GetTestingIter(range) + 1;
      auto iter2 = iter1 - 2;

      CHECK_NOT_EQUAL_NODUMP(iter1, iter2);
      CHECK_EQUAL(iter2 - iter1, -2);

      iter1 -= 2;

      CHECK_EQUAL_NODUMP(iter1, iter2);
    }

    { // operator + (it, off)
      auto iter1 = this->GetTestingIter(range) - 1;
      auto iter2 = iter1 + 2;

      CHECK_NOT_EQUAL_NODUMP(iter1, iter2);
      CHECK_EQUAL(iter2 - iter1, 2);
    }

    { // operator + (off, it)
      auto iter1 = this->GetTestingIter(range) - 1;
      auto iter2 = 2 + iter1;

      CHECK_NOT_EQUAL_NODUMP(iter1, iter2);
      CHECK_EQUAL(iter2 - iter1, 2);
    }

    { // operator - (it, off)
      auto iter1 = this->GetTestingIter(range) + 1;
      auto iter2 = iter1 - 2;

      CHECK_NOT_EQUAL_NODUMP(iter1, iter2);
      CHECK_EQUAL(iter2 - iter1, -2);
    }

    { // operator - (it, it)
      auto iter1 = this->GetTestingIter(range);
      auto iter2 = iter1;
      CHECK_EQUAL(iter2 - iter1, 0);

      iter2++;
      CHECK_EQUAL(iter2 - iter1, 1);

      iter2--;
      CHECK_EQUAL(iter2 - iter1, 0);

      --iter2;
      CHECK_EQUAL(iter2 - iter1, -1);

      iter1++;
      CHECK_EQUAL(iter2 - iter1, -2);
    }
  }

  template <typename Range>
  void TestComparison(Range& range)
  {
    { // operator ==
      auto iter1 = this->GetTestingIter(range);
      auto iter2 = iter1;

      CHECK_TRUE(iter1 == iter2);
      ++iter2;
      CHECK_FALSE(iter1 == iter2);
    }

    { // operator !=
      auto iter1 = this->GetTestingIter(range);
      auto iter2 = iter1;

      CHECK_FALSE(iter1 != iter2);
      ++iter2;
      CHECK_TRUE(iter1 != iter2);
    }

    { // operator <
      auto iter1 = this->GetTestingIter(range);
      auto iter2 = iter1 + 1;

      CHECK_TRUE(iter1 < iter2);
      CHECK_FALSE(iter2 < iter1);
    }

    { // operator >
      auto iter1 = this->GetTestingIter(range);
      auto iter2 = iter1 - 1;

      CHECK_TRUE(iter1 > iter2);
      CHECK_FALSE(iter2 > iter1);
    }

    { // operator <=
      auto iter1 = this->GetTestingIter(range);
      auto iter2 = iter1;

      CHECK_TRUE(iter1 <= iter2);
      CHECK_TRUE(iter2 <= iter1);
      ++iter2;
      CHECK_TRUE(iter1 <= iter2);
      CHECK_FALSE(iter2 <= iter1);
    }

    { // operator >=
      auto iter1 = this->GetTestingIter(range);
      auto iter2 = iter1;

      CHECK_TRUE(iter1 >= iter2);
      CHECK_TRUE(iter2 >= iter1);
      --iter2;
      CHECK_TRUE(iter1 >= iter2);
      CHECK_FALSE(iter2 >= iter1);
    }
  }

  template <typename Range>
  void TestDerefConst(Range& range)
  {
    using ComponentType = typename Range::ComponentType;

    auto tuple = this->GetTestingIterRange(range);

    ComponentType value = *tuple->begin();
    for (auto it = tuple->begin(); it < tuple->end(); ++it)
    {
      CHECK_EQUAL(value++, *it);
    }
  }

  template <typename Range>
  void TestDeref(Range& range)
  {
    using ComponentType = typename Range::ComponentType;

    auto tupleIter = this->GetTestingIterRange(range);
    auto tuple = *tupleIter;

    ComponentType initialValue = *tuple.begin();

    for (auto it = tuple.begin(); it < tuple.end(); ++it)
    {
      *it = 10;
    }

    using TupleType = decltype(tuple);

    for (typename TupleType::reference comp : tuple)
    {
      CHECK_EQUAL(comp, 10);
    }

    for (typename TupleType::value_type comp : tuple)
    {
      comp = 16;
      (void)comp; // silence set-but-not-used
    }

    for (auto it = tuple.begin(); it < tuple.end(); ++it)
    {
      CHECK_EQUAL(*it, 10); // Still 10
    }

    for (typename TupleType::reference comp : tuple)
    {
      comp = 16;
    }

    for (auto it = tuple.begin(); it < tuple.end(); ++it)
    {
      CHECK_EQUAL(*it, 16);
    }

    // Restore:
    for (auto it = tuple.begin(); it < tuple.end(); ++it)
    {
      *it = initialValue++;
    }
  }

  template <typename Range>
  void TestIndexingConst(Range& range)
  {
    using ComponentType = typename Range::ComponentType;

    auto tupleIter = this->GetTestingIterRange(range);
    auto tuple = *tupleIter;

    using TupleType = decltype(tuple);
    using IndexType = typename TupleType::size_type;

    auto compIter = tuple.begin();
    ComponentType comp = *compIter;
    for (IndexType i = 0; i < tuple.size(); ++i)
    {
      typename TupleType::value_type val = compIter[i];
      typename TupleType::const_reference cref = compIter[i];
      CHECK_EQUAL(comp, val);
      CHECK_EQUAL(comp, cref);
      ++comp;
    }
  }

  template <typename Range>
  void TestIndexing(Range& range)
  {
    using ComponentType = typename Range::ComponentType;

    auto tupleIter = this->GetTestingIterRange(range);
    auto tuple = *tupleIter;

    using TupleType = decltype(tuple);
    using IndexType = typename TupleType::size_type;

    auto compIter = tuple.begin();
    ComponentType compSave = *compIter;
    ComponentType comp = *compIter;
    for (IndexType i = 0; i < tuple.size(); ++i)
    {
      typename TupleType::value_type val = compIter[i];
      typename TupleType::reference ref = compIter[i];
      typename TupleType::const_reference cref = compIter[i];
      CHECK_EQUAL(comp, val);
      CHECK_EQUAL(comp, ref);
      CHECK_EQUAL(comp, cref);
      ++comp;

      // Set the ref to 10 to check that assignment works:
      ref = 10;
    }

    // Check that the assignment worked:
    for (IndexType i = 0; i < tuple.size(); ++i)
    {
      typename TupleType::value_type val = compIter[i];
      typename TupleType::reference ref = compIter[i];
      typename TupleType::const_reference cref = compIter[i];
      CHECK_EQUAL(10, val);
      CHECK_EQUAL(10, ref);
      CHECK_EQUAL(10, cref);

      // Restore the component:
      ref = compSave++;
    }
  }

  template <typename Range>
  void TestSwap(Range& range)
  {
    auto iter = this->GetTestingIter(range);
    auto iter1 = iter;
    auto iter2 = iter1 + 1;

    CHECK_TRUE(iter1 < iter2);
    CHECK_FALSE(iter2 < iter1);
    CHECK_TRUE(iter1 + 1 == iter2);
    CHECK_TRUE(iter == iter1);

    {
      using namespace std;
      swap(iter1, iter2);
    }

    CHECK_FALSE(iter1 < iter2);
    CHECK_TRUE(iter2 < iter1);
    CHECK_TRUE(iter2 + 1 == iter1);
    CHECK_TRUE(iter == iter2);

    {
      using namespace std;
      swap(iter1, iter2);
    }

    CHECK_TRUE(iter1 < iter2);
    CHECK_FALSE(iter2 < iter1);
    CHECK_TRUE(iter1 + 1 == iter2);
    CHECK_TRUE(iter == iter1);
  }

  template <typename Range>
  void TestIterSwap(Range& range)
  {
    using ComponentType = typename Range::ComponentType;

    auto iter = this->GetTestingIter(range);
    auto iter1 = iter;
    auto iter2 = iter1 + 1;

    ComponentType val1 = *iter1;
    ComponentType val2 = *iter2;

    CHECK_TRUE(iter1 < iter2);
    CHECK_FALSE(iter2 < iter1);
    CHECK_TRUE(iter1 + 1 == iter2);
    CHECK_TRUE(iter == iter1);
    CHECK_EQUAL(val1, *iter1);
    CHECK_EQUAL(val2, *iter2);

    std::iter_swap(iter1, iter2);

    CHECK_TRUE(iter1 < iter2);
    CHECK_FALSE(iter2 < iter1);
    CHECK_TRUE(iter1 + 1 == iter2);
    CHECK_TRUE(iter == iter1);
    CHECK_EQUAL(val1, *iter2);
    CHECK_EQUAL(val2, *iter1);

    std::iter_swap(iter1, iter2);

    CHECK_TRUE(iter1 < iter2);
    CHECK_FALSE(iter2 < iter1);
    CHECK_TRUE(iter1 + 1 == iter2);
    CHECK_TRUE(iter == iter1);
    CHECK_EQUAL(val1, *iter1);
    CHECK_EQUAL(val2, *iter2);
  }

  // Returns an iterator range for a single tuple's components. tupleOffset
  // allows different tuples to be obtained. The valid range for offset is
  // (-2, 2).
  template <typename Range>
  static auto GetTestingIterRange(Range& range, vtk::TupleIdType tupleOffset = 0)
    -> decltype(range.begin())
  {
    return (range.begin() + (range.size() / 2)) + tupleOffset;
  }

  // Returns an iterator. tupleOffset allows iterators from different tuples to
  // be obtained. The returned iterator +/- 1 position is guaranteed valid.
  template <typename Range>
  static auto GetTestingIter(Range& range, vtk::TupleIdType tupleOffset = 0)
    -> decltype(range.begin()->begin())
  {
    return (*(range.begin() + (range.size() / 2) + tupleOffset)).begin() + 1;
  }
};

template <typename ArrayType>
struct UnitTestComponentReferenceAPI
{
  static constexpr vtk::ComponentIdType NumComps = 9;
  static constexpr vtk::TupleIdType NumTuples = 5;

  void operator()()
  {
    vtkNew<ArrayType> array;
    array->SetNumberOfComponents(NumComps);
    array->SetNumberOfTuples(NumTuples);
    FillTupleRangeIota(vtk::DataArrayTupleRange<NumComps>(array));

    auto da = static_cast<vtkDataArray*>(array);

    { // Full, dynamic-size, real typed range
      auto range = vtk::DataArrayTupleRange(array);
      DispatchRangeTests(range);
    }
    { // Full, dynamic-size, generic-typed range
      auto range = vtk::DataArrayTupleRange(da);
      DispatchRangeTests(range);
    }
    { // Full, fixed-size, real typed range
      auto range = vtk::DataArrayTupleRange<NumComps>(array);
      DispatchRangeTests(range);
    }
    { // Full, fixed-size, generic-typed range
      auto range = vtk::DataArrayTupleRange<NumComps>(da);
      DispatchRangeTests(range);
    }
  }

  template <typename Range>
  void DispatchRangeTests(Range range)
  {
    {
      TestComponentReference(range);
      TestConstComponentReference(range);
    }

    {
      const Range& crange = range;
      TestConstComponentReference(crange);
    }
  }

  template <typename Range>
  void TestComponentReference(Range& range)
  {
    TestCopy(range);
    TestAssign(range);
    TestSwap(range);
    TestMath(range);
  }

  template <typename Range>
  void TestConstComponentReference(Range& range)
  {
    TestComparison(range);
    TestConstMath(range);
  }

  template <typename Range>
  void TestCopy(Range& range)
  {
    using APIType = typename Range::ComponentType;
    using RefType = typename Range::ComponentReferenceType;

    RefType ref1 = this->GetTestCompRef(range, 0);
    const APIType val = ref1;

    RefType ref1Copy{ ref1 };
    CHECK_EQUAL_NODUMP(ref1, ref1Copy);
    CHECK_EQUAL_NODUMP(val, ref1Copy);

    ref1Copy = val - 1;
    CHECK_EQUAL_NODUMP(ref1, ref1Copy);
    CHECK_EQUAL_NODUMP(ref1Copy, val - 1);
    CHECK_EQUAL_NODUMP(ref1, val - 1);

    ref1 = val;
    CHECK_EQUAL_NODUMP(ref1, ref1Copy);
    CHECK_EQUAL_NODUMP(ref1Copy, val);
    CHECK_EQUAL_NODUMP(ref1, val);
  }

  template <typename Range>
  void TestAssign(Range& range)
  {
    using APIType = typename Range::ComponentType;
    using RefType = typename Range::ComponentReferenceType;

    RefType ref1 = this->GetTestCompRef(range, 0);
    const APIType val = ref1;

    RefType ref1Copy{ ref1 };
    CHECK_EQUAL_NODUMP(ref1, ref1Copy);
    CHECK_EQUAL_NODUMP(val, ref1Copy);

    ref1Copy = val - 1;
    CHECK_EQUAL_NODUMP(ref1, ref1Copy);
    CHECK_EQUAL_NODUMP(ref1Copy, val - 1);
    CHECK_EQUAL_NODUMP(ref1, val - 1);

    ref1 = val;
    CHECK_EQUAL_NODUMP(ref1, ref1Copy);
    CHECK_EQUAL_NODUMP(ref1Copy, val);
    CHECK_EQUAL_NODUMP(ref1, val);

    auto ref2 = this->GetTestCompRef(range, 1);
    CHECK_EQUAL_NODUMP(ref2, val + 1);
    CHECK_NOT_EQUAL_NODUMP(ref1, ref2);
    CHECK_NOT_EQUAL_NODUMP(ref1Copy, ref2);
    CHECK_NOT_EQUAL_NODUMP(val, ref2);

    ref1 = ref2;
    CHECK_EQUAL_NODUMP(ref1, ref2);
    CHECK_EQUAL_NODUMP(ref1Copy, ref2);
    CHECK_EQUAL_NODUMP(ref1, val + 1);
    CHECK_EQUAL_NODUMP(ref1Copy, val + 1);

    ref1 = val;
    CHECK_EQUAL_NODUMP(ref1, ref1Copy);
    CHECK_EQUAL_NODUMP(ref1Copy, val);
    CHECK_EQUAL_NODUMP(ref1, val);
    CHECK_EQUAL_NODUMP(ref2, val + 1);
  }

  template <typename Range>
  void TestSwap(Range& range)
  {
    using APIType = typename Range::ComponentType;

    auto ref1 = this->GetTestCompRef(range, 0);
    const APIType val1 = ref1;

    APIType val2 = val1 + 1;

    using std::swap;
    swap(ref1, val2);

    CHECK_EQUAL_NODUMP(ref1, val1 + 1);
    CHECK_EQUAL_NODUMP(val1, val2);

    swap(val2, ref1);

    CHECK_EQUAL_NODUMP(ref1, val1);
    CHECK_EQUAL_NODUMP(val2, val1 + 1);

    auto ref2 = this->GetTestCompRef(range, 1);
    CHECK_EQUAL_NODUMP(ref2, val2);

    swap(ref1, ref2);

    CHECK_EQUAL_NODUMP(ref1, val2);
    CHECK_EQUAL_NODUMP(ref2, val1);

    swap(ref2, ref1);

    CHECK_EQUAL_NODUMP(ref1, val1);
    CHECK_EQUAL_NODUMP(ref2, val2);
  }

  template <typename Range>
  void TestMath(Range& range)
  {
    // Testing mutable math only. Const math is tested in TestConstMath
    using APIType = typename Range::ComponentType;
    using RefType = typename Range::ComponentReferenceType;

    RefType ref1 = this->GetTestCompRef(range, 0);
    RefType ref2 = this->GetTestCompRef(range, 1);
    const APIType val1 = ref1;
    const APIType val2 = ref2;

    const APIType one = static_cast<APIType>(1);
    const APIType two = static_cast<APIType>(2);
    const APIType bignum = static_cast<APIType>(120); // must fit in int8

    // +=
    {
      auto v = (ref1 += one);
      CHECK_EQUAL_NODUMP(ref1, v);
      CHECK_EQUAL_NODUMP(ref1, val1 + one);
      ref1 = val1;
    }
    {
      APIType tmp = one;
      auto v = (tmp += ref1);
      CHECK_EQUAL_NODUMP(ref1, val1);
      CHECK_EQUAL_NODUMP(tmp, val1 + one);
      CHECK_EQUAL_NODUMP(v, val1 + one);
    }
    {
      auto v = (ref1 += ref2);
      CHECK_EQUAL_NODUMP(ref1, val1 + val2);
      CHECK_EQUAL_NODUMP(ref2, val2);
      CHECK_EQUAL_NODUMP(v, val1 + val2);
      ref1 = val1;
    }

    // -=
    {
      auto v = (ref1 -= one);
      CHECK_EQUAL_NODUMP(ref1, v);
      CHECK_EQUAL_NODUMP(ref1, val1 - one);
      ref1 = val1;
    }
    {
      APIType tmp = bignum;
      auto v = (tmp -= ref1);
      CHECK_EQUAL_NODUMP(ref1, val1);
      CHECK_EQUAL_NODUMP(tmp, bignum - val1);
      CHECK_EQUAL_NODUMP(v, bignum - val1);
    }
    {
      auto v = (ref1 -= ref2);
      CHECK_EQUAL_NODUMP(ref1, val1 - val2);
      CHECK_EQUAL_NODUMP(ref2, val2);
      CHECK_EQUAL_NODUMP(v, val1 - val2);
      ref1 = val1;
    }

    // *=
    {
      auto v = (ref1 *= two);
      CHECK_EQUAL_NODUMP(ref1, v);
      CHECK_EQUAL_NODUMP(ref1, val1 * two);
      ref1 = val1;
    }
    {
      APIType tmp = two;
      auto v = (tmp *= ref1);
      CHECK_EQUAL_NODUMP(ref1, val1);
      CHECK_EQUAL_NODUMP(tmp, val1 * two);
      CHECK_EQUAL_NODUMP(v, val1 * two);
    }
    {
      auto v = (ref1 *= ref2);
      CHECK_EQUAL_NODUMP(ref1, val1 * val2);
      CHECK_EQUAL_NODUMP(ref2, val2);
      CHECK_EQUAL_NODUMP(v, val1 * val2);
      ref1 = val1;
    }

    // /=
    {
      auto v = (ref1 /= two);
      CHECK_EQUAL_NODUMP(ref1, v);
      CHECK_EQUAL_NODUMP(ref1, val1 / two);
      ref1 = val1;
    }
    {
      APIType tmp = bignum;
      auto v = (tmp /= ref1);
      CHECK_EQUAL_NODUMP(ref1, val1);
      CHECK_EQUAL_NODUMP(tmp, bignum / val1);
      CHECK_EQUAL_NODUMP(v, bignum / val1);
    }
    {
      auto v = (ref1 /= ref2);
      // Use a tolerance test to account for rounding errors.
      CHECK_TRUE(std::fabs(ref1 - APIType{ val1 / val2 }) < 1e-5);
      CHECK_EQUAL_NODUMP(ref2, val2);
      CHECK_TRUE(std::fabs(v - APIType{ val1 / val2 }) < 1e-5);
      ref1 = val1;
    }

    // ++ (pre)
    {
      auto v = ++ref1;
      CHECK_EQUAL_NODUMP(ref1, val1 + one);
      CHECK_EQUAL_NODUMP(v, val1 + one);
      ref1 = val1;
    }

    // ++ (post)
    {
      auto v = ref1++;
      CHECK_EQUAL_NODUMP(ref1, val1 + one);
      CHECK_EQUAL_NODUMP(v, val1);
      ref1 = val1;
    }

    // -- (pre)
    {
      auto v = --ref1;
      CHECK_EQUAL_NODUMP(ref1, val1 - one);
      CHECK_EQUAL_NODUMP(v, val1 - one);
      ref1 = val1;
    }

    // -- (post)
    {
      auto v = ref1--;
      CHECK_EQUAL_NODUMP(ref1, val1 - one);
      CHECK_EQUAL_NODUMP(v, val1);
      ref1 = val1;
    }
  }

  template <typename Range>
  void TestComparison(Range& range)
  {
    using APIType = typename Range::ComponentType;

    auto ref1 = this->GetTestCompRef(range, 0);
    auto refTmp = this->GetTestCompRef(range, 0); // same as ref1
    auto ref2 = this->GetTestCompRef(range, 1);
    const APIType val1 = ref1;
    const APIType val2 = ref2;

    const APIType one = static_cast<APIType>(1);
    const APIType bignum = static_cast<APIType>(120); // must fit in int8

    // ==
    CHECK_TRUE(ref1 == val1);
    CHECK_TRUE(ref1 == refTmp);
    CHECK_FALSE(ref1 == val2);
    CHECK_FALSE(ref2 == refTmp);

    // !=
    CHECK_FALSE(ref1 != val1);
    CHECK_FALSE(ref1 != refTmp);
    CHECK_TRUE(ref1 != val2);
    CHECK_TRUE(ref2 != refTmp);

    // <
    CHECK_TRUE(ref1 < bignum);
    CHECK_TRUE(one < ref1);
    CHECK_TRUE(ref1 < ref2);
    CHECK_TRUE(refTmp < ref2);
    CHECK_FALSE(bignum < ref1);
    CHECK_FALSE(ref1 < one);
    CHECK_FALSE(ref2 < ref1);
    CHECK_FALSE(ref2 < refTmp);
    CHECK_FALSE(ref1 < refTmp);
    CHECK_FALSE(ref1 < val1);
    CHECK_FALSE(val1 < ref1);

    // >
    CHECK_FALSE(ref1 > bignum);
    CHECK_FALSE(one > ref1);
    CHECK_FALSE(ref1 > ref2);
    CHECK_FALSE(refTmp > ref2);
    CHECK_TRUE(bignum > ref1);
    CHECK_TRUE(ref1 > one);
    CHECK_TRUE(ref2 > ref1);
    CHECK_TRUE(ref2 > refTmp);
    CHECK_FALSE(ref1 > refTmp);
    CHECK_FALSE(ref1 > val1);
    CHECK_FALSE(val1 > ref1);

    // <=
    CHECK_TRUE(ref1 <= bignum);
    CHECK_TRUE(one <= ref1);
    CHECK_TRUE(ref1 <= ref2);
    CHECK_TRUE(refTmp <= ref2);
    CHECK_FALSE(bignum <= ref1);
    CHECK_FALSE(ref1 <= one);
    CHECK_FALSE(ref2 <= ref1);
    CHECK_FALSE(ref2 <= refTmp);
    CHECK_TRUE(ref1 <= refTmp);
    CHECK_TRUE(ref1 <= val1);
    CHECK_TRUE(val1 <= ref1);

    // >=
    CHECK_FALSE(ref1 >= bignum);
    CHECK_FALSE(one >= ref1);
    CHECK_FALSE(ref1 >= ref2);
    CHECK_FALSE(refTmp >= ref2);
    CHECK_TRUE(bignum >= ref1);
    CHECK_TRUE(ref1 >= one);
    CHECK_TRUE(ref2 >= ref1);
    CHECK_TRUE(ref2 >= refTmp);
    CHECK_TRUE(ref1 >= refTmp);
    CHECK_TRUE(ref1 >= val1);
    CHECK_TRUE(val1 >= ref1);
  }

  template <typename Range>
  void TestConstMath(Range& range)
  {
    // Testing const math only. Mutable math is tested in TestMath
    using APIType = typename Range::ComponentType;
    using CRefType = typename Range::ConstComponentReferenceType;

    const CRefType ref1 = this->GetTestCompRef(range, 0);
    const CRefType ref2 = this->GetTestCompRef(range, 1);
    const APIType val1 = ref1;
    const APIType val2 = ref2;

    const APIType one = static_cast<APIType>(1);
    const APIType two = static_cast<APIType>(2);
    const APIType bignum = static_cast<APIType>(120); // must fit in int8

    // +
    {
      auto v = (ref1 + one);
      CHECK_EQUAL_NODUMP(ref1, val1);
      CHECK_EQUAL_NODUMP(v, val1 + one);
    }
    {
      auto v = (one + ref1);
      CHECK_EQUAL_NODUMP(ref1, val1);
      CHECK_EQUAL_NODUMP(v, val1 + one);
    }
    {
      auto v = (ref1 + ref2);
      CHECK_EQUAL_NODUMP(ref1, val1);
      CHECK_EQUAL_NODUMP(ref2, val2);
      CHECK_EQUAL_NODUMP(v, val1 + val2);
    }

    // -
    {
      auto v = (ref1 - one);
      CHECK_EQUAL_NODUMP(ref1, val1);
      CHECK_EQUAL_NODUMP(v, val1 - one);
    }
    {
      auto v = (bignum - ref1);
      CHECK_EQUAL_NODUMP(ref1, val1);
      CHECK_EQUAL_NODUMP(v, bignum - val1);
    }
    {
      auto v = (ref1 - ref2);
      CHECK_EQUAL_NODUMP(ref1, val1);
      CHECK_EQUAL_NODUMP(ref2, val2);
      CHECK_EQUAL_NODUMP(v, val1 - val2);
    }

    // *
    {
      auto v = (ref1 * two);
      CHECK_EQUAL_NODUMP(ref1, val1);
      CHECK_EQUAL_NODUMP(v, val1 * two);
    }
    {
      auto v = (two * ref1);
      CHECK_EQUAL_NODUMP(ref1, val1);
      CHECK_EQUAL_NODUMP(v, val1 * two);
    }
    {
      auto v = (ref1 * ref2);
      CHECK_EQUAL_NODUMP(ref1, val1);
      CHECK_EQUAL_NODUMP(ref2, val2);
      CHECK_EQUAL_NODUMP(v, val1 * val2);
    }

    // /
    {
      auto v = (ref1 / two);
      CHECK_EQUAL_NODUMP(ref1, val1);
      CHECK_EQUAL_NODUMP(v, val1 / two);
    }
    {
      auto v = (bignum / ref1);
      CHECK_EQUAL_NODUMP(ref1, val1);
      CHECK_EQUAL_NODUMP(v, bignum / val1);
    }
    {
      auto v = (ref1 / ref2);
      CHECK_EQUAL_NODUMP(ref1, val1);
      CHECK_EQUAL_NODUMP(ref2, val2);
      CHECK_EQUAL_NODUMP(v, val1 / val2);
    }
  }

  // Return a component reference. Valid offsets range from (-4, 4), and
  // values increase with offset.
  template <typename Range>
  auto GetTestCompRef(Range& range, vtk::ComponentIdType offset)
    -> decltype(std::declval<Range>()[0][0])
  {
    assert(offset >= -4 && offset <= 4);

    return range[3][4 + offset];
  }
};

struct UnitTestEdgeCases
{
  static constexpr vtk::ComponentIdType NumComps = 3;
  static constexpr vtk::TupleIdType NumTuples = 12;

  void operator()()
  {
    TestSpecializations();

    std::cerr << "SOA<float> <--> AOS<float>\n";
    DispatchTupleCompat<vtkSOADataArrayTemplate<float>, vtkAOSDataArrayTemplate<float> >();

    std::cerr << "AOS<float> <--> SOA<float>\n";
    DispatchTupleCompat<vtkAOSDataArrayTemplate<float>, vtkSOADataArrayTemplate<float> >();

    std::cerr << "SOA<double> <--> AOS<float>\n";
    DispatchTupleCompat<vtkSOADataArrayTemplate<double>, vtkAOSDataArrayTemplate<float> >();

    std::cerr << "AOS<float> <--> SOA<double>\n";
    DispatchTupleCompat<vtkAOSDataArrayTemplate<float>, vtkSOADataArrayTemplate<double> >();

    std::cerr << "SOA<int> <--> AOS<float>\n";
    DispatchTupleCompat<vtkSOADataArrayTemplate<int>, vtkAOSDataArrayTemplate<float> >();

    std::cerr << "AOS<float> <--> SOA<int>\n";
    DispatchTupleCompat<vtkAOSDataArrayTemplate<float>, vtkSOADataArrayTemplate<int> >();

#ifdef VTK_USE_SCALED_SOA_ARRAYS
    std::cerr << "ScaleSOA<float> <--> AOS<float>\n";
    DispatchTupleCompat<vtkScaledSOADataArrayTemplate<float>, vtkAOSDataArrayTemplate<float> >();

    std::cerr << "AOS<float> <--> ScaleSOA<float>\n";
    DispatchTupleCompat<vtkAOSDataArrayTemplate<float>, vtkScaledSOADataArrayTemplate<float> >();

    std::cerr << "ScaleSOA<double> <--> AOS<float>\n";
    DispatchTupleCompat<vtkScaledSOADataArrayTemplate<double>, vtkAOSDataArrayTemplate<float> >();

    std::cerr << "AOS<float> <--> ScaleSOA<double>\n";
    DispatchTupleCompat<vtkAOSDataArrayTemplate<float>, vtkScaledSOADataArrayTemplate<double> >();

    std::cerr << "ScaleSOA<int> <--> AOS<float>\n";
    DispatchTupleCompat<vtkScaledSOADataArrayTemplate<int>, vtkAOSDataArrayTemplate<float> >();

    std::cerr << "AOS<float> <--> ScaleSOA<int>\n";
    DispatchTupleCompat<vtkAOSDataArrayTemplate<float>, vtkScaledSOADataArrayTemplate<int> >();
#endif
  }

  static void TestSpecializations()
  {
    // Specializations are disabled when iterator debugging is enabled:
#ifndef VTK_DEBUG_RANGE_ITERATORS
    // These should use the objects in vtkDataArrayTupleRange_AOS.h, which
    // end up using ValueType* pointers for component iterators.
    TestAOSSpecialization<vtkAOSDataArrayTemplate<float> >();
    TestAOSSpecialization<vtkFloatArray>();
#endif
  }

#ifndef VTK_DEBUG_RANGE_ITERATORS
  template <typename ArrayType>
  static void TestAOSSpecialization()
  {
    using ValueType = vtk::GetAPIType<ArrayType>;
    using RangeType = decltype(vtk::DataArrayTupleRange(std::declval<ArrayType*>()));
    using CompIterType = decltype(std::declval<RangeType>().begin()->begin());

    static_assert(std::is_same<ValueType*, typename std::decay<CompIterType>::type>::value,
      "AOS specialization not used!");
  }
#endif

  template <typename ArrayType>
  static void PrepArray(ArrayType* array)
  {
    array->SetNumberOfComponents(NumComps);
    array->SetNumberOfTuples(NumTuples);
    FillTupleRangeIota(vtk::DataArrayTupleRange<NumComps>(array));
  }

  template <typename ArrayT1, typename ArrayT2>
  void DispatchTupleCompat()
  {
    vtkNew<ArrayT1> storage1;
    vtkNew<ArrayT2> storage2;
    this->PrepArray(static_cast<ArrayT1*>(storage1));
    this->PrepArray(static_cast<ArrayT2*>(storage2));

    ArrayT1* a1 = storage1;
    ArrayT2* a2 = storage2;
    vtkDataArray* da1 = a1;
    vtkDataArray* da2 = a2;

    // Generate ranges:
    // - derived and vtkDataArray pointers
    // - dynamic and fixed tuple sizes
    // - mutable and const tuple sizes

    auto aRange1 = vtk::DataArrayTupleRange(a1);
    auto aRange2 = vtk::DataArrayTupleRange(a2);
    auto daRange1 = vtk::DataArrayTupleRange(da1);
    auto daRange2 = vtk::DataArrayTupleRange(da2);

    auto aFixedRange1 = vtk::DataArrayTupleRange<NumComps>(a1);
    auto aFixedRange2 = vtk::DataArrayTupleRange<NumComps>(a2);
    auto daFixedRange1 = vtk::DataArrayTupleRange<NumComps>(da1);
    auto daFixedRange2 = vtk::DataArrayTupleRange<NumComps>(da2);

    using ARange1Type = decltype(aRange1);
    using ARange2Type = decltype(aRange2);
    using DARange1Type = decltype(daRange1);
    using DARange2Type = decltype(daRange2);

    using AFixedRange1Type = decltype(aFixedRange1);
    using AFixedRange2Type = decltype(aFixedRange2);
    using DAFixedRange1Type = decltype(daFixedRange1);
    using DAFixedRange2Type = decltype(daFixedRange2);

    const ARange1Type caRange1 = aRange1;
    const ARange2Type caRange2 = aRange2;
    const DARange1Type cdaRange1 = daRange1;
    const DARange2Type cdaRange2 = daRange2;

    const AFixedRange1Type caFixedRange1 = aFixedRange1;
    const AFixedRange2Type caFixedRange2 = aFixedRange2;
    const DAFixedRange1Type cdaFixedRange1 = daFixedRange1;
    const DAFixedRange2Type cdaFixedRange2 = daFixedRange2;

    this->LaunchTests(aRange1, aRange2);
    this->LaunchTests(aRange1, daRange2);
    this->LaunchTests(aRange1, aFixedRange2);
    this->LaunchTests(aRange1, daFixedRange2);
    this->LaunchTests(aRange1, caRange2);
    this->LaunchTests(aRange1, cdaRange2);
    this->LaunchTests(aRange1, caFixedRange2);
    this->LaunchTests(aRange1, cdaFixedRange2);
    this->LaunchTests(daRange1, aRange2);
    this->LaunchTests(daRange1, daRange2);
    this->LaunchTests(daRange1, aFixedRange2);
    this->LaunchTests(daRange1, daFixedRange2);
    this->LaunchTests(daRange1, caRange2);
    this->LaunchTests(daRange1, cdaRange2);
    this->LaunchTests(daRange1, caFixedRange2);
    this->LaunchTests(daRange1, cdaFixedRange2);
    this->LaunchTests(aFixedRange1, aRange2);
    this->LaunchTests(aFixedRange1, daRange2);
    this->LaunchTests(aFixedRange1, aFixedRange2);
    this->LaunchTests(aFixedRange1, daFixedRange2);
    this->LaunchTests(aFixedRange1, caRange2);
    this->LaunchTests(aFixedRange1, cdaRange2);
    this->LaunchTests(aFixedRange1, caFixedRange2);
    this->LaunchTests(aFixedRange1, cdaFixedRange2);
    this->LaunchTests(daFixedRange1, aRange2);
    this->LaunchTests(daFixedRange1, daRange2);
    this->LaunchTests(daFixedRange1, aFixedRange2);
    this->LaunchTests(daFixedRange1, daFixedRange2);
    this->LaunchTests(daFixedRange1, caRange2);
    this->LaunchTests(daFixedRange1, cdaRange2);
    this->LaunchTests(daFixedRange1, caFixedRange2);
    this->LaunchTests(daFixedRange1, cdaFixedRange2);
    this->LaunchTests(caRange1, aRange2);
    this->LaunchTests(caRange1, daRange2);
    this->LaunchTests(caRange1, aFixedRange2);
    this->LaunchTests(caRange1, daFixedRange2);
    this->LaunchTests(caRange1, caRange2);
    this->LaunchTests(caRange1, cdaRange2);
    this->LaunchTests(caRange1, caFixedRange2);
    this->LaunchTests(caRange1, cdaFixedRange2);
    this->LaunchTests(cdaRange1, aRange2);
    this->LaunchTests(cdaRange1, daRange2);
    this->LaunchTests(cdaRange1, aFixedRange2);
    this->LaunchTests(cdaRange1, daFixedRange2);
    this->LaunchTests(cdaRange1, caRange2);
    this->LaunchTests(cdaRange1, cdaRange2);
    this->LaunchTests(cdaRange1, caFixedRange2);
    this->LaunchTests(cdaRange1, cdaFixedRange2);
    this->LaunchTests(caFixedRange1, aRange2);
    this->LaunchTests(caFixedRange1, daRange2);
    this->LaunchTests(caFixedRange1, aFixedRange2);
    this->LaunchTests(caFixedRange1, daFixedRange2);
    this->LaunchTests(caFixedRange1, caRange2);
    this->LaunchTests(caFixedRange1, cdaRange2);
    this->LaunchTests(caFixedRange1, caFixedRange2);
    this->LaunchTests(caFixedRange1, cdaFixedRange2);
    this->LaunchTests(cdaFixedRange1, aRange2);
    this->LaunchTests(cdaFixedRange1, daRange2);
    this->LaunchTests(cdaFixedRange1, aFixedRange2);
    this->LaunchTests(cdaFixedRange1, daFixedRange2);
    this->LaunchTests(cdaFixedRange1, caRange2);
    this->LaunchTests(cdaFixedRange1, cdaRange2);
    this->LaunchTests(cdaFixedRange1, caFixedRange2);
    this->LaunchTests(cdaFixedRange1, cdaFixedRange2);
  }

  template <typename Range>
  using IsConst = std::is_const<Range>;

  template <typename Range>
  using IsMutable = std::integral_constant<bool, !IsConst<Range>::value>;

  template <typename R1, typename R2>
  using SameComponentType = std::integral_constant<bool,
    std::is_same<typename R1::ComponentType, typename R2::ComponentType>::value>;

  template <typename Range1, typename Range2>
  using IsSwappable = std::integral_constant<bool,
    (SameComponentType<Range1, Range2>::value && IsMutable<Range1>::value &&
      IsMutable<Range2>::value)>;

  template <typename Range1, typename Range2>
  using IsNotSwappable = std::integral_constant<bool, !IsSwappable<Range1, Range2>::value>;

  template <typename Range, typename T = void>
  using EnableIfRangeIsConst = typename std::enable_if<IsConst<Range>::value, T>::type;

  template <typename Range, typename T = void>
  using EnableIfRangeIsMutable = typename std::enable_if<IsMutable<Range>::value, T>::type;

  template <typename Range1, typename Range2, typename T = void>
  using EnableIfSameComponentType =
    typename std::enable_if<SameComponentType<Range1, Range2>::value, T>::type;

  template <typename Range1, typename Range2, typename T = void>
  using EnableIfSwappable = typename std::enable_if<IsSwappable<Range1, Range2>::value, T>::type;

  template <typename Range1, typename Range2, typename T = void>
  using EnableIfNotSwappable =
    typename std::enable_if<IsNotSwappable<Range1, Range2>::value, T>::type;

  // range1 is const:
  template <typename Range1, typename Range2>
  void LaunchTests(Range1& r1, Range2& r2, EnableIfRangeIsConst<Range1, void*> = nullptr)
  {
    this->TestTupleEquality(r1, r2);
    this->TestTupleInequality(r1, r2);
    this->TestCompCompare(r1, r2);
  }

  // range1 is not const:
  template <typename Range1, typename Range2>
  void LaunchTests(Range1& r1, Range2& r2, EnableIfRangeIsMutable<Range1, void*> = nullptr)
  {
    this->TestTupleAssignment(r1, r2);
    this->TestTupleEquality(r1, r2);
    this->TestTupleInequality(r1, r2);
    this->TestTupleSwap(r1, r2);
    this->TestCompAssign(r1, r2);
    this->TestCompCompare(r1, r2);
    this->TestCompSwap(r1, r2);
  }

  template <typename Range1, typename Range2>
  void TestTupleAssignment(Range1& r1, Range2& r2)
  {
    static_assert(IsMutable<Range1>{}, "r1 must be mutable.");

    typename Range1::reference ref1 = r1[3];
    typename Range2::const_reference ref2 = r2[4];
    auto data1 = this->StoreTuple(ref1);
    auto data2 = this->StoreTuple(ref2);

    CHECK_TRUE(this->CompareTuple(ref1, data1));
    CHECK_TRUE(this->CompareTuple(ref2, data2));
    CHECK_FALSE(this->CompareTuple(ref1, data2));

    ref1 = ref2;

    CHECK_TRUE(this->CompareTuple(ref1, data2));
    CHECK_TRUE(this->CompareTuple(*(r1.begin() + 3), data2));
    CHECK_TRUE(this->CompareTuple(ref2, data2));

    this->RestoreTuple(ref1, data1);
  }

  template <typename Range1, typename Range2>
  void TestTupleEquality(Range1& r1, Range2& r2)
  {
    auto i1 = r1.begin();
    auto i2 = r2.begin();
    auto end1 = r1.end();
    auto end2 = r2.end();

    while (i1 < end1 && i2 < end2)
    {
      CHECK_TRUE(*i1++ == *i2++);
    }
  }

  template <typename Range1, typename Range2>
  void TestTupleInequality(Range1& r1, Range2& r2)
  {
    auto i1 = r1.begin();
    auto i2 = r2.begin();
    auto end1 = r1.end() - 1;
    auto end2 = r2.end();

    while (i1 < end1 && i2 < end2)
    {
      CHECK_TRUE(*++i1 != *i2++);
    }
  }

  template <typename Range1, typename Range2>
  EnableIfNotSwappable<Range1, Range2, void> TestTupleSwap(Range1&, Range2&)
  {
    // no-op, ranges aren't swappable.
  }

  template <typename Range1, typename Range2>
  EnableIfSwappable<Range1, Range2, void> TestTupleSwap(Range1& r1, Range2& r2)
  {
    static_assert(SameComponentType<Range1, Range2>::value, "Mismatched ComponentTypes.");
    static_assert(IsMutable<Range1>::value, "r1 must be mutable.");
    static_assert(IsMutable<Range2>::value, "r2 must be mutable.");

    typename Range1::iterator iter1 = r1.begin() + 3;
    typename Range2::iterator iter2 = r2.begin() + 4;

    typename Range1::reference ref1 = *iter1;
    typename Range2::reference ref2 = *iter2;

    auto data1 = this->StoreTuple(ref1);
    auto data2 = this->StoreTuple(ref2);

    CHECK_TRUE(this->CompareTuple(ref1, data1));
    CHECK_TRUE(this->CompareTuple(ref2, data2));
    CHECK_FALSE(this->CompareTuple(ref1, data2));
    CHECK_FALSE(this->CompareTuple(ref2, data1));

    {
      using std::swap;
      swap(ref1, ref2);
    }

    CHECK_TRUE(this->CompareTuple(ref1, data2));
    CHECK_TRUE(this->CompareTuple(*(r1.begin() + 3), data2));
    CHECK_TRUE(this->CompareTuple(ref2, data1));
    CHECK_TRUE(this->CompareTuple(*(r2.begin() + 4), data1));

    ref1.swap(ref2);

    CHECK_TRUE(this->CompareTuple(ref1, data1));
    CHECK_TRUE(this->CompareTuple(*(r1.begin() + 3), data1));
    CHECK_TRUE(this->CompareTuple(ref2, data2));
    CHECK_TRUE(this->CompareTuple(*(r2.begin() + 4), data2));

    {
      std::iter_swap(iter1, iter2);
    }

    CHECK_TRUE(this->CompareTuple(ref1, data2));
    CHECK_TRUE(this->CompareTuple(*(r1.begin() + 3), data2));
    CHECK_TRUE(this->CompareTuple(ref2, data1));
    CHECK_TRUE(this->CompareTuple(*(r2.begin() + 4), data1));

    this->RestoreTuple(ref1, data1);
    this->RestoreTuple(ref2, data2);
  }

  template <typename Range1, typename Range2>
  void TestCompAssign(Range1& r1, Range2& r2)
  {
    static_assert(IsMutable<Range1>{}, "r1 must be mutable.");

    typename Range1::reference tref1 = r1[3];
    typename Range2::const_reference tref2 = r2[4];

    auto data1 = this->StoreTuple(tref1);
    auto data2 = this->StoreTuple(tref2);

    CHECK_FALSE(this->CompareTuple(tref1, data2));

    // Use iterator_traits to find a proper reference type:
    using IterT = decltype(tref1.begin());
    using RefT = typename std::iterator_traits<IterT>::reference;

    auto iter2 = tref2.begin();
    for (RefT comp : tref1)
    {
      comp = *iter2++;
    }

    CHECK_TRUE(this->CompareTuple(tref1, data2));
    CHECK_TRUE(this->CompareTuple(tref2, data2));

    this->RestoreTuple(tref1, data1);
  }

  template <typename Range1, typename Range2>
  void TestCompCompare(Range1& r1, Range2& r2)
  {
    typename Range1::const_reference tref1 = r1[3];
    typename Range2::const_reference tref2 = r2[3];

    auto iter1 = tref1.begin();
    auto iter2 = tref2.begin();

    CHECK_TRUE(*iter1 == *iter2);
    CHECK_FALSE(*iter1 != *iter2);
    CHECK_FALSE(*iter1 < *iter2);
    CHECK_FALSE(*iter1 > *iter2);
    CHECK_TRUE(*iter1 <= *iter2);
    CHECK_TRUE(*iter1 >= *iter2);

    ++iter2;

    CHECK_FALSE(*iter1 == *iter2);
    CHECK_TRUE(*iter1 != *iter2);
    CHECK_TRUE(*iter1 < *iter2);
    CHECK_FALSE(*iter1 > *iter2);
    CHECK_TRUE(*iter1 <= *iter2);
    CHECK_FALSE(*iter1 >= *iter2);

    iter1 += 2;

    CHECK_FALSE(*iter1 == *iter2);
    CHECK_TRUE(*iter1 != *iter2);
    CHECK_FALSE(*iter1 < *iter2);
    CHECK_TRUE(*iter1 > *iter2);
    CHECK_FALSE(*iter1 <= *iter2);
    CHECK_TRUE(*iter1 >= *iter2);
  }

  template <typename Range1, typename Range2>
  EnableIfNotSwappable<Range1, Range2, void> TestCompSwap(Range1&, Range2&)
  {
    // no-op, ranges aren't swappable.
  }

  template <typename Range1, typename Range2>
  EnableIfSwappable<Range1, Range2, void> TestCompSwap(Range1& r1, Range2& r2)
  {
    static_assert(SameComponentType<Range1, Range2>::value, "Mismatched ComponentTypes.");
    static_assert(IsMutable<Range1>::value, "r1 must be mutable.");
    static_assert(IsMutable<Range2>::value, "r2 must be mutable.");

    typename Range1::reference tref1 = r1[3];
    typename Range2::reference tref2 = r2[4];

    auto data1 = this->StoreTuple(tref1);
    auto data2 = this->StoreTuple(tref2);

    CHECK_FALSE(this->CompareTuple(tref1, data2));

    {
      auto iter2 = tref2.begin();
      for (typename decltype(tref1)::reference comp : tref1)
      {
        using std::swap;
        swap(comp, *iter2++);
      }
      CHECK_TRUE(iter2 == tref2.end());
    }

    CHECK_TRUE(this->CompareTuple(tref1, data2));
    CHECK_TRUE(this->CompareTuple(*(r1.begin() + 3), data2));
    CHECK_TRUE(this->CompareTuple(tref2, data1));
    CHECK_TRUE(this->CompareTuple(*(r2.begin() + 4), data1));

    {
      auto iter1 = tref1.begin();
      auto iter2 = tref2.begin();
      while (iter1 < tref1.end())
      {
        std::iter_swap(iter1++, iter2++);
      }
      CHECK_TRUE(iter2 == tref2.end());
    }

    CHECK_TRUE(this->CompareTuple(tref1, data1));
    CHECK_TRUE(this->CompareTuple(*(r1.begin() + 3), data1));
    CHECK_TRUE(this->CompareTuple(tref2, data2));
    CHECK_TRUE(this->CompareTuple(*(r2.begin() + 4), data2));

    this->RestoreTuple(tref1, data1);
    this->RestoreTuple(tref2, data2);
  }

  template <typename RefType>
  static auto StoreTuple(const RefType& ref) -> std::vector<typename RefType::value_type>
  {
    return std::vector<typename RefType::value_type>{ ref.begin(), ref.end() };
  }

  template <typename RefType>
  static void RestoreTuple(RefType& ref, const std::vector<typename RefType::value_type>& data)
  {
    std::copy(data.begin(), data.end(), ref.begin());
  }

  template <typename RefType, typename DataType>
  static bool CompareTuple(const RefType& ref, const std::vector<DataType>& data)
  {
    return static_cast<std::size_t>(ref.size()) == data.size() &&
      std::equal(data.begin(), data.end(), ref.begin());
  }
};

template <typename ArrayType>
void RunTestsForArray()
{
  std::cerr << "TupleRangeAPI:\n";
  UnitTestTupleRangeAPI<ArrayType>{}();
  std::cerr << "TupleIteratorAPI:\n";
  UnitTestTupleIteratorAPI<ArrayType>{}();
  std::cerr << "TupleReferenceAPI:\n";
  UnitTestTupleReferenceAPI<ArrayType>{}();
  std::cerr << "ComponentIteratorAPI:\n";
  UnitTestComponentIteratorAPI<ArrayType>{}();
  std::cerr << "ComponentReferenceAPI:\n";
  UnitTestComponentReferenceAPI<ArrayType>{}();
}

} // end anon namespace

int TestDataArrayTupleRange(int, char*[])
{
  std::cerr << "AOS:\n";
  RunTestsForArray<vtkAOSDataArrayTemplate<float> >();
  std::cerr << "SOA:\n";
  RunTestsForArray<vtkSOADataArrayTemplate<float> >();
#ifdef VTK_USE_SCALED_SOA_ARRAYS
  std::cerr << "ScaleSOA:\n";
  RunTestsForArray<vtkScaledSOADataArrayTemplate<float> >();
#endif
  std::cerr << "vtkFloatArray:\n";
  RunTestsForArray<vtkFloatArray>();

  std::cerr << "\nEdgeCases:\n";
  UnitTestEdgeCases{}();

  return NumErrors != 0;
}
