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

#include <algorithm>
#include <numeric>
#include <utility>
#include <type_traits>

namespace
{

std::size_t NumErrors = 0;

#define TO_STRING(x) TO_STRING2(x)
#define TO_STRING2(x) #x
#define LOCATION() "line " TO_STRING(__LINE__) ""

#define CHECK_TYPEDEF(t1, t2) \
  static_assert(std::is_same<typename std::decay<t1>::type, \
                             typename std::decay<t2>::type>{}, \
                "Type mismatch: '" #t1 "' not same as '" #t2 "' in " \
                LOCATION())

#define LOG_ERROR(message) \
  ++NumErrors; \
  std::cerr << NumErrors << ": " << message << "\n"

#define CHECK_TRUE(expr) \
  do { if (!(expr)) \
  { \
    LOG_ERROR("Expression not true: '" #expr << "' at " LOCATION()); \
  } } while (false)

#define CHECK_FALSE(expr) \
  do { if ((expr)) \
  { \
    LOG_ERROR("Expression expected to be false but is true: '" #expr \
              << "' at " LOCATION()); \
  } } while (false)

#define CHECK_EQUAL(v1, v2) \
  do { if (!(v1 == v2)) \
  { \
    LOG_ERROR("Expressions not equal: '" #v1 "' (" << v1 << ") and '" #v2 "' ("\
              << v2 << ") in " LOCATION()); \
  } } while (false)

#define CHECK_NOT_EQUAL(v1, v2) \
  do { if (!(v1 != v2)) \
  { \
    LOG_ERROR("Expressions not equal: '" #v1 "' (" << v1 << ") and '" #v2 "' ("\
              << v2 << ") in " LOCATION()); \
  } } while (false)

#define CHECK_EQUAL_NODUMP(v1, v2) \
  do { if (!(v1 == v2)) \
  { \
    LOG_ERROR("Expressions not equal: '" #v1 "' and '" #v2 "' in " LOCATION());\
  } } while (false)

#define CHECK_NOT_EQUAL_NODUMP(v1, v2) \
  do { if (!(v1 != v2)) \
  { \
    LOG_ERROR("Expressions should be unequal but aren't: '" #v1 "' and '" \
              #v2 "' in " LOCATION());\
  } } while (false)

//==============================================================================
//==============================================================================
// Helpers:
//==============================================================================
//==============================================================================
template <typename Range>
void FillValueRangeIota(Range range)
{
  using ValueType = typename Range::ValueType;

  ValueType value{1};
  std::iota(range.begin(), range.end(), value);
}

template <typename Range>
void TestIota(Range range)
{
  auto startValue = range.GetBeginValueId() + 1;
  auto endValue = range.GetEndValueId() + 1;

  auto value = startValue;

  for (auto& comp : range)
  {
    CHECK_EQUAL(value, comp);
    ++value;
  }

  CHECK_EQUAL(value, endValue);
}

//==============================================================================
//==============================================================================
// TupleRange:
//==============================================================================
//==============================================================================
template <typename ArrayType>
struct UnitTestValueRangeAPI
{
  static constexpr vtk::ComponentIdType NumComps = 3;
  static constexpr vtk::TupleIdType NumTuples = 12;
  static constexpr vtk::ValueIdType NumValues = NumTuples * NumComps;

  void operator()()
  {
    vtkNew<ArrayType> array;
    auto da = static_cast<vtkDataArray*>(array);
    array->SetNumberOfComponents(NumComps);

    this->TestEmptyRange(vtk::DataArrayValueRange(array));
    this->TestEmptyRange(vtk::DataArrayValueRange(da));
    this->TestEmptyRange(vtk::DataArrayValueRange<NumComps>(array));
    this->TestEmptyRange(vtk::DataArrayValueRange<NumComps>(da));

    array->SetNumberOfTuples(this->NumTuples);

    this->TestEmptyRange(vtk::DataArrayValueRange(array, 4, 4));
    this->TestEmptyRange(vtk::DataArrayValueRange(da, 4, 4));
    this->TestEmptyRange(vtk::DataArrayValueRange<NumComps>(array, 4, 4));
    this->TestEmptyRange(vtk::DataArrayValueRange<NumComps>(da, 4, 4));

    FillValueRangeIota(vtk::DataArrayValueRange<NumComps>(array));

    auto pStart = static_cast<vtk::ValueIdType>(NumTuples / 4 * NumComps + 1);
    auto pEnd = static_cast<vtk::ValueIdType>(3 * NumTuples / 4 * NumComps + 2);

    { // Full, dynamic-size, real typed range
      auto range = vtk::DataArrayValueRange(array);
      DispatchRangeTests<ArrayType, vtk::detail::DynamicTupleSize>
          (range, array, 0, NumValues);
    }
    { // Full, dynamic-size, generic-typed range
      auto range = vtk::DataArrayValueRange(da);
      DispatchRangeTests<vtkDataArray, vtk::detail::DynamicTupleSize>
          (range, array, 0, NumValues);
    }
    { // Full, fixed-size, real typed range
      auto range = vtk::DataArrayValueRange<NumComps>(array);
      DispatchRangeTests<ArrayType, NumComps>(range, array, 0, NumValues);
    }
    { // Full, fixed-size, generic-typed range
      auto range = vtk::DataArrayValueRange<NumComps>(da);
      DispatchRangeTests<vtkDataArray, NumComps>(range, array, 0, NumValues);
    }
    { // Partial, dynamic-size, real typed range
      auto range = vtk::DataArrayValueRange(array, pStart, pEnd);
      DispatchRangeTests<ArrayType, vtk::detail::DynamicTupleSize>
          (range, array, pStart, pEnd);
    }
    { // Partial, dynamic-size, generic-typed range
      auto range = vtk::DataArrayValueRange(da, pStart, pEnd);
      DispatchRangeTests<vtkDataArray, vtk::detail::DynamicTupleSize>
          (range, array, pStart, pEnd);
    }
    { // Partial, fixed-size, real typed range
      auto range = vtk::DataArrayValueRange<NumComps>(array, pStart, pEnd);
      DispatchRangeTests<ArrayType, NumComps>(range, array, pStart, pEnd);
    }
    { // Partial, fixed-size, generic-typed range
      auto range = vtk::DataArrayValueRange<NumComps>(da, pStart, pEnd);
      DispatchRangeTests<vtkDataArray, NumComps>(range, array, pStart, pEnd);
    }
  }

  template <typename Range>
  void TestEmptyRange(Range range)
  {
    for (auto& value : range)
    {
      (void)value;
      CHECK_TRUE(false && "This should not execute.");
    }
  }

  template <typename RangeArrayType,
            vtk::ComponentIdType RangeTupleSize,
            typename Range>
  void DispatchRangeTests(Range range,
                          RangeArrayType *array,
                          vtk::ValueIdType start,
                          vtk::ValueIdType end)
  {
    {
      TestRange<RangeArrayType, RangeTupleSize>(range, array, start, end);
    }

    {
      const Range& crange = range;
      TestRange<RangeArrayType, RangeTupleSize>(crange, array, start, end);
    }
  }

  template <typename RangeArrayType,
            vtk::ComponentIdType RangeTupleSize,
            typename Range>
  void TestRange(Range& range,
                 RangeArrayType *array,
                 vtk::ValueIdType start,
                 vtk::ValueIdType end)
  {
    TestTypes<RangeArrayType, RangeTupleSize>(range);

    CHECK_EQUAL(range.GetArray(), array);
    CHECK_EQUAL(range.GetTupleSize(), NumComps);
    CHECK_EQUAL(range.GetBeginValueId(), start);
    CHECK_EQUAL(range.GetEndValueId(), end);
    CHECK_EQUAL(range.size(), end - start);
    CHECK_EQUAL(range.end() - range.begin(), range.size());
    CHECK_EQUAL(range.cend() - range.cbegin(), range.size());

    TestIota(range);
  }

  template <typename RangeArrayType,
            vtk::ComponentIdType RangeTupleSize,
            typename Range>
  void TestTypes(Range &range)
  {
    using ConstRange = typename std::add_const<Range>::type;
    using MutableRange = typename std::remove_const<Range>::type;
    (void)range; // MSVC thinks this is unused when it appears in decltype.

    CHECK_TYPEDEF(typename Range::ArrayType, RangeArrayType);
    CHECK_TYPEDEF(typename Range::ValueType,
                  vtk::GetAPIType<RangeArrayType>);
    CHECK_TYPEDEF(typename Range::ComponentType,
                  vtk::GetAPIType<RangeArrayType>);
    CHECK_TYPEDEF(typename Range::size_type, vtk::ValueIdType);
    CHECK_TYPEDEF(typename Range::size_type, decltype(range.size()));
    CHECK_TYPEDEF(typename Range::iterator,
                  decltype(std::declval<MutableRange>().begin()));
    CHECK_TYPEDEF(typename Range::iterator,
                  decltype(std::declval<MutableRange>().end()));
    CHECK_TYPEDEF(typename Range::const_iterator,
                  decltype(std::declval<ConstRange>().begin()));
    CHECK_TYPEDEF(typename Range::const_iterator,
                  decltype(std::declval<ConstRange>().end()));
    CHECK_TYPEDEF(typename Range::const_iterator,
                  decltype(range.cbegin()));
    CHECK_TYPEDEF(typename Range::const_iterator,
                  decltype(range.cend()));
    CHECK_TYPEDEF(typename Range::ArrayType, decltype(*range.GetArray()));
    CHECK_TYPEDEF(vtk::ValueIdType, decltype(range.GetBeginValueId()));
    CHECK_TYPEDEF(vtk::ValueIdType, decltype(range.GetEndValueId()));

    static_assert(Range::TupleSizeTag == RangeTupleSize,
                  "Range::TupleSizeTag incorrect.");
  }
};

template <typename ArrayType>
struct UnitTestValueIteratorAPI
{
  static constexpr vtk::ComponentIdType NumComps = 3;
  static constexpr vtk::TupleIdType NumTuples = 12;
  static constexpr vtk::ValueIdType NumValues = NumComps * NumTuples;

  void operator()()
  {
    vtkNew<ArrayType> array;
    array->SetNumberOfComponents(NumComps);
    array->SetNumberOfTuples(NumTuples);
    FillValueRangeIota(vtk::DataArrayValueRange<NumComps>(array));

    auto da = static_cast<vtkDataArray*>(array);

    { // Full, dynamic-size, real typed range
      auto range = vtk::DataArrayValueRange(array);
      DispatchRangeTests(range);
    }
    { // Full, dynamic-size, generic-typed range
      auto range = vtk::DataArrayValueRange(da);
      DispatchRangeTests(range);
    }
    { // Full, fixed-size, real typed range
      auto range = vtk::DataArrayValueRange<NumComps>(array);
      DispatchRangeTests(range);
    }
    { // Full, fixed-size, generic-typed range
      auto range = vtk::DataArrayValueRange<NumComps>(da);
      DispatchRangeTests(range);
    }
  }

  template <typename Range>
  void DispatchRangeTests(Range range)
  {
    {
      TestValueIterator(range);
      TestConstValueIterator(range);
    }

    {
      const Range& crange = range;
      TestConstValueIterator(crange);
    }
  }

  template <typename Range>
  void TestValueIterator(Range& range)
  {
    TestDeref(range);
    TestIndexing(range);
    TestIterSwap(range);
  }

  template <typename Range>
  void TestConstValueIterator(Range& range)
  {
    TestCopy(range);
    TestAssign(range);
    TestTraversal(range);
    TestDerefConst(range);
    TestComparison(range);
    TestIndexingConst(range);
    TestSwap(range);
  }

  template <typename Range>
  void TestCopy(Range& range)
  {
    auto iter = this->GetTestingIter(range);

    using IterType = decltype(iter);
    IterType iter2{iter};

    CHECK_EQUAL_NODUMP(iter, iter2);
  }

  template <typename Range>
  void TestAssign(Range& range)
  {
    auto iter = this->GetTestingIter(range);

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
    using ValueType = typename Range::ValueType;

    auto start = this->GetTestingIter(range);
    auto end = start + 4;

    ValueType value = *start;
    for (auto it = start; it < end; ++it)
    {
      CHECK_EQUAL(value++, *it);
    }
  }

  template <typename Range>
  void TestDeref(Range& range)
  {
    using ValueType = typename Range::ValueType;

    auto start = this->GetTestingIter(range);
    auto end = start + 4;

    ValueType initialValue = *start;

    for (auto it = start; it < end; ++it)
    {
      *it = 10;
    }

    for (auto it = start; it < end; ++it)
    {
      CHECK_EQUAL(*it, 10);
    }

    // Assigning to auto by value is currently disabled. See note on deleted
    // ComponentReference copy constructor.
#if 0
    // auto should deduce to a raw value type, this assignment should have
    // no effect:
    for (auto it = start; it < end; ++it)
    {
      comp = 16;
    }

    for (auto it = start; it < end; ++it)
    {
      CHECK_EQUAL(*it, 10); // Still 10
    }
#endif

    // auto should deduce to a reference type, this assignment should be saved:
    for (auto it = start; it < end; ++it)
    {
      *it = 16;
    }

    for (auto it = start; it < end; ++it)
    {
      CHECK_EQUAL(*it, 16);
    }

    // Restore:
    for (auto it = start; it < end; ++it)
    {
      *it = initialValue++;
    }
  }

  template <typename Range>
  void TestIndexingConst(Range& range)
  {
    (void)range;
    // operator[] disabled. See vtk::DataArrayTupleRange documentation.
#if 0
    using ValueType = typename Range::ValueType;

    auto tuple = this->GetTestingIterRange(range);

    using IndexType = typename decltype(*tuple)::size_type;

    auto iter = tuple->begin();
    ValueType value = *iter;
    for (IndexType i = 0; i < tuple->size(); ++i)
    {
      CHECK_EQUAL(value++, iter[i]);
    }
#endif
  }

  template <typename Range>
  void TestIndexing(Range& range)
  {
    (void)range;
#if 0
    using ValueType = typename Range::ValueType;

    auto tuple = this->GetTestingIterRange(range);

    using IndexType = typename decltype(*tuple)::size_type;
    ValueType initialValue = *tuple->begin();

    auto iter = tuple->begin();
    for (IndexType i = 0; i < tuple->size(); ++i)
    {
      iter[i] = 19;
    }

    for (auto& comp : *tuple)
    {
      CHECK_EQUAL(comp, 19);
    }

    // Restore:
    for (IndexType i = 0; i < tuple->size(); ++i)
    {
      iter[i] = initialValue++;
    }
#endif
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
    using ValueType = typename Range::ValueType;

    auto iter = this->GetTestingIter(range);
    auto iter1 = iter;
    auto iter2 = iter1 + 1;

    ValueType val1 = *iter1;
    ValueType val2 = *iter2;

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


  // Returns an iterator. tupleOffset allows iterators from different tuples to
  // be obtained. The returned iterator +/- 4 are guaranteed valid.
  template <typename Range>
  static auto GetTestingIter(Range& range)
  -> decltype(range.begin())
  {
    return range.begin() + NumValues / 2;
  }
};

struct UnitTestEdgeCases
{
  static constexpr vtk::ComponentIdType NumComps = 3;
  static constexpr vtk::TupleIdType NumTuples = 12;

  void operator()()
  {
    std::cerr << "SOA<float> <--> AOS<float>\n";
    DispatchValueCompat<vtkSOADataArrayTemplate<float>,
                        vtkAOSDataArrayTemplate<float>>();

    std::cerr << "AOS<float> <--> SOA<float>\n";
    DispatchValueCompat<vtkAOSDataArrayTemplate<float>,
                        vtkSOADataArrayTemplate<float>>();

    std::cerr << "SOA<double> <--> AOS<float>\n";
    DispatchValueCompat<vtkSOADataArrayTemplate<double>,
                        vtkAOSDataArrayTemplate<float>>();

    std::cerr << "AOS<float> <--> SOA<double>\n";
    DispatchValueCompat<vtkAOSDataArrayTemplate<float>,
                        vtkSOADataArrayTemplate<double>>();

    std::cerr << "SOA<int> <--> AOS<float>\n";
    DispatchValueCompat<vtkSOADataArrayTemplate<int>,
                        vtkAOSDataArrayTemplate<float>>();

    std::cerr << "AOS<float> <--> SOA<int>\n";
    DispatchValueCompat<vtkAOSDataArrayTemplate<float>,
                        vtkSOADataArrayTemplate<int>>();

#ifdef VTK_USE_SCALED_SOA_ARRAYS
    std::cerr << "ScaleSOA<float> <--> AOS<float>\n";
    DispatchValueCompat<vtkScaledSOADataArrayTemplate<float>,
                        vtkAOSDataArrayTemplate<float>>();

    std::cerr << "AOS<float> <--> ScaleSOA<float>\n";
    DispatchValueCompat<vtkAOSDataArrayTemplate<float>,
                        vtkScaledSOADataArrayTemplate<float>>();

    std::cerr << "ScaleSOA<double> <--> AOS<float>\n";
    DispatchValueCompat<vtkScaledSOADataArrayTemplate<double>,
                        vtkAOSDataArrayTemplate<float>>();

    std::cerr << "AOS<float> <--> ScaleSOA<double>\n";
    DispatchValueCompat<vtkAOSDataArrayTemplate<float>,
                        vtkScaledSOADataArrayTemplate<double>>();

    std::cerr << "ScaleSOA<int> <--> AOS<float>\n";
    DispatchValueCompat<vtkScaledSOADataArrayTemplate<int>,
                        vtkAOSDataArrayTemplate<float>>();

    std::cerr << "AOS<float> <--> ScaleSOA<int>\n";
    DispatchValueCompat<vtkAOSDataArrayTemplate<float>,
                        vtkScaledSOADataArrayTemplate<int>>();
#endif
  }

  template <typename ArrayType>
  static void PrepArray(ArrayType *array)
  {
    array->SetNumberOfComponents(NumComps);
    array->SetNumberOfTuples(NumTuples);
    FillValueRangeIota(vtk::DataArrayValueRange<NumComps>(array));
  }

  template <typename ArrayT1, typename ArrayT2>
  void DispatchValueCompat()
  {
    vtkNew<ArrayT1> storage1;
    vtkNew<ArrayT2> storage2;
    this->PrepArray(static_cast<ArrayT1*>(storage1));
    this->PrepArray(static_cast<ArrayT2*>(storage2));

    ArrayT1 *a1 = storage1;
    ArrayT2 *a2 = storage2;
    vtkDataArray *da1 = a1;
    vtkDataArray *da2 = a2;

    // Generate ranges:
    // - derived and vtkDataArray pointers
    // - dynamic and fixed tuple sizes
    // - mutable and const tuple sizes

    auto aRange1 = vtk::DataArrayValueRange(a1);
    auto aRange2 = vtk::DataArrayValueRange(a2);
    auto daRange1 = vtk::DataArrayValueRange(da1);
    auto daRange2 = vtk::DataArrayValueRange(da2);

    auto aFixedRange1 = vtk::DataArrayValueRange<NumComps>(a1);
    auto aFixedRange2 = vtk::DataArrayValueRange<NumComps>(a2);
    auto daFixedRange1 = vtk::DataArrayValueRange<NumComps>(da1);
    auto daFixedRange2 = vtk::DataArrayValueRange<NumComps>(da2);

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
  using SameValueType =
  std::integral_constant<bool, std::is_same<typename R1::value_type,
                                            typename R2::value_type>::value>;

  template <typename Range1, typename Range2>
  using IsSwappable =
  std::integral_constant<bool, (SameValueType<Range1, Range2>::value &&
                                IsMutable<Range1>::value &&
                                IsMutable<Range2>::value)>;

  template <typename Range1, typename Range2>
  using IsNotSwappable =
  std::integral_constant<bool, !IsSwappable<Range1, Range2>::value>;

  template <typename Range, typename T = void>
  using EnableIfRangeIsConst =
  typename std::enable_if<IsConst<Range>::value, T>::type;

  template <typename Range, typename T = void>
  using EnableIfRangeIsMutable =
  typename std::enable_if<IsMutable<Range>::value, T>::type;

  template <typename Range1, typename Range2, typename T = void>
  using EnableIfSameValueType =
  typename std::enable_if<SameValueType<Range1, Range2>::value, T>::type;

  template <typename Range1, typename Range2, typename T = void>
  using EnableIfSwappable =
  typename std::enable_if<IsSwappable<Range1, Range2>::value, T>::type;

  template <typename Range1, typename Range2, typename T = void>
  using EnableIfNotSwappable =
  typename std::enable_if<IsNotSwappable<Range1, Range2>::value, T>::type;

  // range1 is const:
  template <typename Range1,
            typename Range2>
  void LaunchTests(Range1& r1, Range2& r2,
                   EnableIfRangeIsConst<Range1, void*> = nullptr)
  {
    this->TestValueCompare(r1, r2);
  }

  // range1 is not const:
  template <typename Range1,
            typename Range2>
  void LaunchTests(Range1& r1, Range2& r2,
                   EnableIfRangeIsMutable<Range1, void*> = nullptr)
  {
    this->TestValueAssign(r1, r2);
    this->TestValueCompare(r1, r2);
    this->TestValueSwap(r1, r2);
  }

  template <typename Range1, typename Range2>
  void TestValueAssign(Range1 &r1, Range2 &r2)
  {
    static_assert(IsMutable<Range1>{}, "r1 must be mutable.");

    auto start1 = r1.begin() + 2;
    auto end1 = start1 + 4;
    auto start2 = r2.begin() + 6;
    auto end2 = start2 + 4;

    auto data1 = this->StoreRange(start1, end1);
    auto data2 = this->StoreRange(start2, end2);

    CHECK_FALSE(this->CompareRange(start1, end1, data2));

    auto iter2 = start2;
    for (auto it = start1; it < end1; ++it)
    {
      *it = *iter2++;
    }

    CHECK_TRUE(this->CompareRange(start1, end1, data2));
    CHECK_TRUE(this->CompareRange(start2, end2, data2));

    this->RestoreRange(start1, end1, data1);
  }

  template <typename Range1, typename Range2>
  void TestValueCompare(Range1 &r1, Range2 &r2)
  {

    auto iter1 = r1.begin() + 7;
    auto iter2 = r2.begin() + 7;

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

  template <typename Range1,
            typename Range2>
  EnableIfNotSwappable<Range1, Range2, void>
  TestValueSwap(Range1 &, Range2 &)
  {
    // no-op, ranges aren't swappable.
  }

  template <typename Range1,
            typename Range2>
  EnableIfSwappable<Range1, Range2, void>
  TestValueSwap(Range1 &r1, Range2 &r2)
  {
    static_assert(SameValueType<Range1, Range2>::value,
                  "Mismatched value_types.");
    static_assert(IsMutable<Range1>::value, "r1 must be mutable.");
    static_assert(IsMutable<Range2>::value, "r2 must be mutable.");

    auto start1 = r1.begin() + 2;
    auto end1 = start1 + 4;
    auto start2 = r2.begin() + 6;
    auto end2 = start2 + 4;

    auto data1 = this->StoreRange(start1, end1);
    auto data2 = this->StoreRange(start2, end2);

    CHECK_TRUE(this->CompareRange(start1, end1, data1));
    CHECK_TRUE(this->CompareRange(start2, end2, data2));
    CHECK_TRUE(this->CompareRange(r1.begin() + 2, r1.begin() + 6, data1));
    CHECK_TRUE(this->CompareRange(r2.begin() + 6, r2.begin() + 10, data2));

    CHECK_FALSE(this->CompareRange(start1, end1, data2));
    CHECK_FALSE(this->CompareRange(start2, end2, data1));
    CHECK_FALSE(this->CompareRange(r1.begin() + 2, r1.begin() + 6, data2));
    CHECK_FALSE(this->CompareRange(r2.begin() + 6, r2.begin() + 10, data1));

    {
      auto it2 = start2;
      for (auto it1 = start1; it1 < end1; ++it1)
      {
        using std::swap;
        swap(*it1, *it2++);
      }
      CHECK_TRUE(it2 == end2);
    }

    CHECK_TRUE(this->CompareRange(start1, end1, data2));
    CHECK_TRUE(this->CompareRange(r1.begin() + 2, r1.begin() + 6, data2));
    CHECK_TRUE(this->CompareRange(start2, end2, data1));
    CHECK_TRUE(this->CompareRange(r2.begin() + 6, r2.begin() + 10, data1));

    {
      auto it2 = start2;
      for (auto it1 = start1; it1 < end1; ++it1)
      {
        std::iter_swap(it1, it2++);
      }
      CHECK_TRUE(it2 == end2);
    }

    CHECK_TRUE(this->CompareRange(start1, end1, data1));
    CHECK_TRUE(this->CompareRange(r1.begin() + 2, r1.begin() + 6, data1));
    CHECK_TRUE(this->CompareRange(start2, end2, data2));
    CHECK_TRUE(this->CompareRange(r2.begin() + 6, r2.begin() + 10, data2));

    this->RestoreRange(start1, end1, data1);
    this->RestoreRange(start2, end2, data2);
  }

  template <typename IterType>
  static auto StoreRange(IterType start, IterType end)
  -> std::vector<typename std::iterator_traits<IterType>::value_type>
  {
    using T = typename std::iterator_traits<IterType>::value_type;
    return std::vector<T>{start, end};
  }

  template <typename IterType, typename VectorType>
  static void RestoreRange(IterType start, IterType end,
                           const VectorType &data)
  {
    static_assert(std::is_same<
                  typename std::iterator_traits<IterType>::value_type,
                  typename VectorType::value_type>::value,
                  "Mismatched value types.");
    CHECK_EQUAL(data.size(), static_cast<size_t>(end - start));
    std::copy(data.begin(), data.end(), start);
  }

  template <typename IterType, typename VectorType>
  static bool CompareRange(IterType start, IterType end,
                           const VectorType &data)
  {
    static_assert(std::is_convertible<
                  typename std::iterator_traits<IterType>::value_type,
                  typename VectorType::value_type>::value,
                  "Mismatched value types.");

    return static_cast<std::size_t>(end - start) == data.size() &&
        std::equal(data.begin(), data.end(), start);
  }
};

template <typename ArrayType>
void RunTestsForArray()
{
  std::cerr << "ValueRangeAPI:\n";
  UnitTestValueRangeAPI<ArrayType>{}();
  std::cerr << "ValueIteratorAPI:\n";
  UnitTestValueIteratorAPI<ArrayType>{}();
}

} // end anon namespace

int TestDataArrayValueRange(int, char*[])
{
  std::cerr << "AOS:\n";
  RunTestsForArray<vtkAOSDataArrayTemplate<float>>();
  std::cerr << "SOA:\n";
  RunTestsForArray<vtkSOADataArrayTemplate<float>>();
#ifdef VTK_USE_SCALED_SOA_ARRAYS
  std::cerr << "ScaleSOA:\n";
  RunTestsForArray<vtkScaledSOADataArrayTemplate<float>>();
#endif
  std::cerr << "vtkFloatArray:\n";
  RunTestsForArray<vtkFloatArray>();

  std::cerr << "\nEdgeCases:\n";
  UnitTestEdgeCases{}();

  return NumErrors != 0;
}
