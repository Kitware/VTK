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

#include <viskores/cont/ArrayCopyDevice.h>
#include <viskores/cont/ArrayHandleBasic.h>
#include <viskores/cont/ArrayHandleCartesianProduct.h>
#include <viskores/cont/ArrayHandleCast.h>
#include <viskores/cont/ArrayHandleCompositeVector.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/ArrayHandleExtractComponent.h>
#include <viskores/cont/ArrayHandleGroupVec.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ArrayHandleRandomUniformReal.h>
#include <viskores/cont/ArrayHandleSOA.h>
#include <viskores/cont/ArrayHandleStride.h>
#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>
#include <viskores/cont/ArrayHandleView.h>
#include <viskores/cont/ArrayHandleXGCCoordinates.h>
#include <viskores/cont/ArrayRangeCompute.h>

#include <viskores/Math.h>
#include <viskores/VecTraits.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

constexpr viskores::Id ARRAY_SIZE = 20;

template <typename T, typename S>
void VerifyRangeScalar(const viskores::cont::ArrayHandle<T, S>& array,
                       const viskores::cont::ArrayHandle<viskores::Range>& computedRangeArray,
                       const viskores::cont::ArrayHandle<viskores::UInt8>& maskArray,
                       bool finitesOnly)
{
  using Traits = viskores::VecTraits<T>;
  viskores::IdComponent numComponents = Traits::NUM_COMPONENTS;

  VISKORES_TEST_ASSERT(computedRangeArray.GetNumberOfValues() == numComponents);
  auto computedRangePortal = computedRangeArray.ReadPortal();

  auto portal = array.ReadPortal();
  auto maskPortal = maskArray.ReadPortal();
  for (viskores::IdComponent component = 0; component < numComponents; ++component)
  {
    viskores::Range computedRange = computedRangePortal.Get(component);
    viskores::Range expectedRange{};
    for (viskores::Id index = 0; index < portal.GetNumberOfValues(); ++index)
    {
      if (maskPortal.GetNumberOfValues() != 0 && (maskPortal.Get(index) == 0))
      {
        continue;
      }
      auto value =
        static_cast<viskores::Float64>(Traits::GetComponent(portal.Get(index), component));
      if (finitesOnly && !viskores::IsFinite(value))
      {
        continue;
      }
      expectedRange.Include(value);
    }
    try
    {
      VISKORES_TEST_ASSERT(!viskores::IsNan(computedRange.Min));
      VISKORES_TEST_ASSERT(!viskores::IsNan(computedRange.Max));
      VISKORES_TEST_ASSERT((!expectedRange.IsNonEmpty() && !computedRange.IsNonEmpty()) ||
                           (test_equal(expectedRange, computedRange)));
    }
    catch (const viskores::testing::Testing::TestFailure&)
    {
      std::cout << "Test array: \n";
      viskores::cont::printSummary_ArrayHandle(array, std::cout, true);
      std::cout << "Mask array: \n";
      viskores::cont::printSummary_ArrayHandle(maskArray, std::cout, true);
      std::cout << "Range type: " << (finitesOnly ? "Scalar, Finite" : "Scalar, NonFinite") << "\n";
      std::cout << "Computed range: \n";
      viskores::cont::printSummary_ArrayHandle(computedRangeArray, std::cout, true);
      std::cout << "Expected range: " << expectedRange << ", component: " << component << "\n";
      throw;
    }
  }
}

template <typename T, typename S>
void VerifyRangeVector(const viskores::cont::ArrayHandle<T, S>& array,
                       const viskores::Range& computedRange,
                       const viskores::cont::ArrayHandle<viskores::UInt8>& maskArray,
                       bool finitesOnly)
{
  auto portal = array.ReadPortal();
  auto maskPortal = maskArray.ReadPortal();
  viskores::Range expectedRange{};
  for (viskores::Id index = 0; index < portal.GetNumberOfValues(); ++index)
  {
    if (maskPortal.GetNumberOfValues() != 0 && (maskPortal.Get(index) == 0))
    {
      continue;
    }
    auto value = static_cast<viskores::Float64>(viskores::MagnitudeSquared(portal.Get(index)));
    if (finitesOnly && !viskores::IsFinite(value))
    {
      continue;
    }
    expectedRange.Include(value);
  }

  if (expectedRange.IsNonEmpty())
  {
    expectedRange.Min = viskores::Sqrt(expectedRange.Min);
    expectedRange.Max = viskores::Sqrt(expectedRange.Max);
  }

  try
  {
    VISKORES_TEST_ASSERT(!viskores::IsNan(computedRange.Min));
    VISKORES_TEST_ASSERT(!viskores::IsNan(computedRange.Max));
    VISKORES_TEST_ASSERT((!expectedRange.IsNonEmpty() && !computedRange.IsNonEmpty()) ||
                         (test_equal(expectedRange, computedRange)));
  }
  catch (const viskores::testing::Testing::TestFailure&)
  {
    std::cout << "Test array: \n";
    viskores::cont::printSummary_ArrayHandle(array, std::cout, true);
    std::cout << "Mask array: \n";
    viskores::cont::printSummary_ArrayHandle(maskArray, std::cout, true);
    std::cout << "Range type: " << (finitesOnly ? "Vector, Finite" : "Vector, NonFinite") << "\n";
    std::cout << "Computed range: " << computedRange << "\n";
    std::cout << "Expected range: " << expectedRange << "\n";
    throw;
  }
}

auto FillMaskArray(viskores::Id length)
{
  viskores::cont::ArrayHandle<viskores::UInt8> maskArray;
  maskArray.Allocate(length);

  viskores::cont::ArrayHandleRandomUniformBits randomBits(length + 1);
  auto randomPortal = randomBits.ReadPortal();
  switch (randomPortal.Get(length) % 3)
  {
    case 0: // all masked
      maskArray.Fill(0);
      break;
    case 1: // none masked
      maskArray.Fill(1);
      break;
    case 2: // random masked
    default:
    {
      auto maskPortal = maskArray.WritePortal();
      for (viskores::Id i = 0; i < length; ++i)
      {
        viskores::UInt8 maskVal = ((randomPortal.Get(i) % 8) == 0) ? 0 : 1;
        maskPortal.Set(i, maskVal);
      }
      break;
    }
  }

  return maskArray;
}

template <typename T, typename S>
void CheckRange(const viskores::cont::ArrayHandle<T, S>& array)
{
  auto length = array.GetNumberOfValues();
  viskores::cont::ArrayHandle<viskores::UInt8> emptyMaskArray;

  auto maskArray = FillMaskArray(length);

  viskores::cont::ArrayHandle<viskores::Range> scalarRange;
  std::cout << "\tchecking scalar range without mask\n";
  scalarRange = viskores::cont::ArrayRangeCompute(array);
  VerifyRangeScalar(array, scalarRange, emptyMaskArray, false);
  std::cout << "\tchecking scalar range with mask\n";
  scalarRange = viskores::cont::ArrayRangeCompute(array, maskArray);
  VerifyRangeScalar(array, scalarRange, maskArray, false);

  viskores::Range vectorRange;
  std::cout << "\tchecking vector range without mask\n";
  vectorRange = viskores::cont::ArrayRangeComputeMagnitude(array);
  VerifyRangeVector(array, vectorRange, emptyMaskArray, false);
  std::cout << "\tchecking vector range with mask\n";
  vectorRange = viskores::cont::ArrayRangeComputeMagnitude(array, maskArray);
  VerifyRangeVector(array, vectorRange, maskArray, false);
}

template <typename ArrayHandleType>
void CheckRangeFiniteImpl(const ArrayHandleType& array, std::true_type)
{
  auto length = array.GetNumberOfValues();
  viskores::cont::ArrayHandle<viskores::UInt8> emptyMaskArray;

  auto maskArray = FillMaskArray(length);

  viskores::cont::ArrayHandle<viskores::Range> scalarRange;
  std::cout << "\tchecking finite scalar range without mask\n";
  scalarRange = viskores::cont::ArrayRangeCompute(array, true);
  VerifyRangeScalar(array, scalarRange, emptyMaskArray, true);
  std::cout << "\tchecking finite scalar range with mask\n";
  scalarRange = viskores::cont::ArrayRangeCompute(array, maskArray, true);
  VerifyRangeScalar(array, scalarRange, maskArray, true);

  viskores::Range vectorRange;
  std::cout << "\tchecking finite vector range without mask\n";
  vectorRange = viskores::cont::ArrayRangeComputeMagnitude(array, true);
  VerifyRangeVector(array, vectorRange, emptyMaskArray, true);
  std::cout << "\tchecking finite vector range with mask\n";
  vectorRange = viskores::cont::ArrayRangeComputeMagnitude(array, maskArray, true);
  VerifyRangeVector(array, vectorRange, maskArray, true);
}

template <typename ArrayHandleType>
void CheckRangeFiniteImpl(const ArrayHandleType&, std::false_type)
{
}

template <typename T, typename S>
void CheckRangeFinite(const viskores::cont::ArrayHandle<T, S>& array)
{
  using ComponentType = typename viskores::VecTraits<T>::ComponentType;
  auto tag = std::integral_constant < bool,
       std::is_same<ComponentType, viskores::Float32>::value ||
    std::is_same<ComponentType, viskores::Float64>::value > {};
  CheckRangeFiniteImpl(array, tag);
}

// Transform random values in range [0, 1) to the range [From, To).
// If the `AddNonFinites` flag is set, some values are transformed to non-finite values.
struct TransformRange
{
  viskores::Float64 From, To;
  bool AddNonFinites = false;

  VISKORES_EXEC viskores::Float64 operator()(viskores::Float64 in) const
  {
    if (AddNonFinites)
    {
      if (in >= 0.3 && in <= 0.33)
      {
        return viskores::NegativeInfinity64();
      }
      if (in >= 0.9 && in <= 0.93)
      {
        return viskores::Infinity64();
      }
    }
    return (in * (this->To - this->From)) + this->From;
  }
};

template <typename T, typename S>
void FillArray(viskores::cont::ArrayHandle<T, S>& array, bool addNonFinites)
{
  using Traits = viskores::VecTraits<T>;
  viskores::IdComponent numComponents = Traits::NUM_COMPONENTS;

  // non-finites only applies to floating point types
  addNonFinites = addNonFinites &&
    (std::is_same<typename Traits::ComponentType, viskores::Float32>::value ||
     std::is_same<typename Traits::ComponentType, viskores::Float64>::value);

  array.AllocateAndFill(ARRAY_SIZE, viskores::TypeTraits<T>::ZeroInitialization());

  for (viskores::IdComponent component = 0; component < numComponents; ++component)
  {
    viskores::cont::ArrayHandleRandomUniformReal<viskores::Float64> randomArray(ARRAY_SIZE);
    auto dest = viskores::cont::make_ArrayHandleExtractComponent(array, component);
    auto transformFunctor = std::numeric_limits<typename Traits::BaseComponentType>::is_signed
      ? TransformRange{ -100.0, 100.0, addNonFinites }
      : TransformRange{ 0.0, 200.0, addNonFinites };
    viskores::cont::ArrayCopyDevice(
      viskores::cont::make_ArrayHandleTransform(randomArray, transformFunctor), dest);
  }
}

template <typename T>
void TestBasicArray()
{
  std::cout << "Checking basic array" << std::endl;
  viskores::cont::ArrayHandleBasic<T> array;
  FillArray(array, false);
  CheckRange(array);
  FillArray(array, true);
  CheckRangeFinite(array);
}

template <typename T>
void TestSOAArray(viskores::TypeTraitsVectorTag)
{
  std::cout << "Checking SOA array" << std::endl;
  viskores::cont::ArrayHandleSOA<T> array;
  FillArray(array, false);
  CheckRange(array);
  FillArray(array, true);
  CheckRangeFinite(array);
}

template <typename T>
void TestSOAArray(viskores::TypeTraitsScalarTag)
{
  // Skip test.
}

template <typename T>
void TestStrideArray()
{
  std::cout << "Checking stride array" << std::endl;
  viskores::cont::ArrayHandleBasic<T> array;
  FillArray(array, false);
  CheckRange(viskores::cont::ArrayHandleStride<T>(array, ARRAY_SIZE / 2, 2, 1));
  FillArray(array, true);
  CheckRangeFinite(viskores::cont::ArrayHandleStride<T>(array, ARRAY_SIZE / 2, 2, 1));
}

template <typename T>
void TestCastArray()
{
  std::cout << "Checking cast array" << std::endl;
  using CastType =
    typename viskores::VecTraits<T>::template ReplaceBaseComponentType<viskores::Float64>;
  viskores::cont::ArrayHandle<T> array;
  FillArray(array, false);
  CheckRange(viskores::cont::make_ArrayHandleCast<CastType>(array));
}

template <typename T>
auto FillCartesianProductArray(bool addNonFinites)
{
  viskores::cont::ArrayHandleBasic<T> array0;
  FillArray(array0, addNonFinites);
  viskores::cont::ArrayHandleBasic<T> array1;
  FillArray(array1, addNonFinites);
  viskores::cont::ArrayHandleBasic<T> array2;
  FillArray(array2, addNonFinites);
  return viskores::cont::make_ArrayHandleCartesianProduct(array0, array1, array2);
}

template <typename T>
void TestCartesianProduct(viskores::TypeTraitsScalarTag)
{
  std::cout << "Checking Cartesian product" << std::endl;
  auto array = FillCartesianProductArray<T>(false);
  CheckRange(array);
  array = FillCartesianProductArray<T>(true);
  CheckRangeFinite(array);
}

template <typename T>
void TestCartesianProduct(viskores::TypeTraitsVectorTag)
{
  // Skip test.
}

template <typename T>
auto FillCompositeVectorArray(bool addNonFinites)
{
  viskores::cont::ArrayHandleBasic<T> array0;
  FillArray(array0, addNonFinites);
  viskores::cont::ArrayHandleBasic<T> array1;
  FillArray(array1, addNonFinites);
  viskores::cont::ArrayHandleBasic<T> array2;
  FillArray(array2, addNonFinites);
  return viskores::cont::make_ArrayHandleCompositeVector(array0, array1, array2);
}

template <typename T>
void TestComposite(viskores::TypeTraitsScalarTag)
{
  std::cout << "Checking composite vector array" << std::endl;

  auto array = FillCompositeVectorArray<T>(false);
  CheckRange(array);
  array = FillCompositeVectorArray<T>(true);
  CheckRangeFinite(array);
}

template <typename T>
void TestComposite(viskores::TypeTraitsVectorTag)
{
  // Skip test.
}

template <typename T>
void TestGroup(viskores::TypeTraitsScalarTag)
{
  std::cout << "Checking group vec array" << std::endl;

  viskores::cont::ArrayHandleBasic<T> array;
  FillArray(array, false);
  CheckRange(viskores::cont::make_ArrayHandleGroupVec<2>(array));
  FillArray(array, true);
  CheckRangeFinite(viskores::cont::make_ArrayHandleGroupVec<2>(array));
}

template <typename T>
void TestGroup(viskores::TypeTraitsVectorTag)
{
  // Skip test.
}

template <typename T>
void TestView()
{
  std::cout << "Checking view array" << std::endl;

  viskores::cont::ArrayHandleBasic<T> array;
  FillArray(array, false);
  CheckRange(viskores::cont::make_ArrayHandleView(array, 2, ARRAY_SIZE - 5));
  FillArray(array, true);
  CheckRangeFinite(viskores::cont::make_ArrayHandleView(array, 2, ARRAY_SIZE - 5));
}

template <typename T>
void TestConstant()
{
  std::cout << "Checking constant array" << std::endl;
  CheckRange(viskores::cont::make_ArrayHandleConstant(TestValue(10, T{}), ARRAY_SIZE));
}

template <typename T>
void TestCounting(std::true_type viskoresNotUsed(is_signed))
{
  std::cout << "Checking counting array" << std::endl;
  CheckRange(viskores::cont::make_ArrayHandleCounting(TestValue(10, T{}), T{ 1 }, ARRAY_SIZE));

  std::cout << "Checking counting backward array" << std::endl;
  CheckRange(viskores::cont::make_ArrayHandleCounting(TestValue(10, T{}), T{ -1 }, ARRAY_SIZE));
}

template <typename T>
void TestCounting(std::false_type viskoresNotUsed(is_signed))
{
  // Skip test
}

void TestIndex()
{
  std::cout << "Checking index array" << std::endl;
  CheckRange(viskores::cont::make_ArrayHandleIndex(ARRAY_SIZE));
}

void TestUniformPointCoords()
{
  std::cout << "Checking uniform point coordinates" << std::endl;
  CheckRange(viskores::cont::ArrayHandleUniformPointCoordinates(
    viskores::Id3(ARRAY_SIZE, ARRAY_SIZE, ARRAY_SIZE)));
}

void TestXGCCoordinates()
{
  std::cout << "Checking XGC coordinates array" << std::endl;
  viskores::cont::ArrayHandle<viskores::FloatDefault> array;
  FillArray(array, false);
  CheckRange(viskores::cont::make_ArrayHandleXGCCoordinates(array, 4, true));
  FillArray(array, true);
  CheckRangeFinite(viskores::cont::make_ArrayHandleXGCCoordinates(array, 4, true));
}

struct DoTestFunctor
{
  template <typename T>
  void operator()(T) const
  {
    typename viskores::TypeTraits<T>::DimensionalityTag dimensionality{};

    TestBasicArray<T>();
    TestSOAArray<T>(dimensionality);
    TestStrideArray<T>();
    TestCastArray<T>();
    TestCartesianProduct<T>(dimensionality);
    TestComposite<T>(dimensionality);
    TestGroup<T>(dimensionality);
    TestView<T>();
    TestConstant<T>();
    TestCounting<T>(
      typename std::is_signed<typename viskores::VecTraits<T>::ComponentType>::type{});
  }
};

void DoTest()
{
  viskores::testing::Testing::TryTypes(DoTestFunctor{});

  std::cout << "*** Specific arrays *****************" << std::endl;
  TestIndex();
  TestUniformPointCoords();
  TestXGCCoordinates();
}

} // anonymous namespace

int UnitTestArrayRangeCompute(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(DoTest, argc, argv);
}
