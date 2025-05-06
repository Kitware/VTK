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

#include <viskores/cont/ArrayExtractComponent.h>

#include <viskores/cont/ArrayHandleCartesianProduct.h>
#include <viskores/cont/ArrayHandleCompositeVector.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleExtractComponent.h>
#include <viskores/cont/ArrayHandleGroupVec.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ArrayHandleMultiplexer.h>
#include <viskores/cont/ArrayHandleReverse.h>
#include <viskores/cont/ArrayHandleRuntimeVec.h>
#include <viskores/cont/ArrayHandleSOA.h>
#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>
#include <viskores/cont/ArrayHandleView.h>

#include <viskores/VecFlat.h>

#include <viskores/cont/testing/Testing.h>

#include <algorithm>
#include <random>
#include <vector>

namespace
{

constexpr viskores::Id ARRAY_SIZE = 10;

template <typename T>
viskores::IdComponent GetTotalNumComponents(const T& vec)
{
  using VTraits = viskores::VecTraits<T>;
  if (std::is_same<typename VTraits::ComponentType, typename VTraits::BaseComponentType>::value)
  {
    return VTraits::GetNumberOfComponents(vec);
  }
  else
  {
    return VTraits::GetNumberOfComponents(vec) *
      GetTotalNumComponents(VTraits::GetComponent(vec, 0));
  }
}

// VecFlat.h has something similar, but it only works with static Vec sizes. It might make sense
// to move this somewhere else later
template <typename BaseComponentType>
struct GetVecFlatIndexImpl
{
  template <typename VecType>
  VISKORES_CONT BaseComponentType operator()(const VecType& vec, viskores::IdComponent index) const
  {
    const viskores::IdComponent subSize = GetTotalNumComponents(vec[0]);
    return (*this)(vec[index / subSize], index % subSize);
  }

  VISKORES_CONT BaseComponentType operator()(const BaseComponentType& component,
                                             viskores::IdComponent index) const
  {
    VISKORES_ASSERT(index == 0);
    return component;
  }
};

template <typename T>
auto GetVecFlatIndex(const T& vec, viskores::IdComponent index)
{
  return GetVecFlatIndexImpl<typename viskores::VecTraits<T>::BaseComponentType>{}(vec, index);
}

template <typename T, typename S>
void CheckInputArray(const viskores::cont::ArrayHandle<T, S>& originalArray,
                     viskores::CopyFlag allowCopy = viskores::CopyFlag::Off)
{
  auto originalPortal = originalArray.ReadPortal();
  using ComponentType = typename viskores::VecTraits<T>::BaseComponentType;
  const viskores::IdComponent numComponents = GetTotalNumComponents(originalPortal.Get(0));
  for (viskores::IdComponent componentId = 0; componentId < numComponents; ++componentId)
  {
    viskores::cont::ArrayHandleStride<ComponentType> componentArray =
      viskores::cont::ArrayExtractComponent(originalArray, componentId, allowCopy);

    auto componentPortal = componentArray.ReadPortal();
    VISKORES_TEST_ASSERT(originalPortal.GetNumberOfValues() == componentPortal.GetNumberOfValues());
    for (viskores::Id arrayIndex = 0; arrayIndex < originalArray.GetNumberOfValues(); ++arrayIndex)
    {
      auto originalValue = GetVecFlatIndex(originalPortal.Get(arrayIndex), componentId);
      ComponentType componentValue = componentPortal.Get(arrayIndex);
      VISKORES_TEST_ASSERT(test_equal(originalValue, componentValue));
    }
  }
}

template <typename T, typename S>
void CheckOutputArray(
  const viskores::cont::ArrayHandle<T, S>& originalArray,
  const viskores::cont::ArrayHandle<T, S>& outputArray = viskores::cont::ArrayHandle<T, S>{})
{
  CheckInputArray(originalArray);

  using ComponentType = typename viskores::VecTraits<T>::BaseComponentType;
  const viskores::IdComponent numComponents =
    GetTotalNumComponents(originalArray.ReadPortal().Get(0));

  // Extract all the stride arrays first, and then allocate them later. This tests to
  // to make sure that the independent allocation of all the extracted arrays are consistent
  // and correct.
  std::vector<std::pair<viskores::cont::ArrayHandleStride<ComponentType>,
                        viskores::cont::ArrayHandleStride<ComponentType>>>
    componentArrays;
  componentArrays.reserve(static_cast<std::size_t>(numComponents));
  for (viskores::IdComponent componentId = 0; componentId < numComponents; ++componentId)
  {
    componentArrays.emplace_back(
      viskores::cont::ArrayExtractComponent(originalArray, numComponents - componentId - 1),
      viskores::cont::ArrayExtractComponent(outputArray, componentId, viskores::CopyFlag::Off));
  }

  // Shuffle the component arrays to ensure the allocation/copy can occur in any order.
  std::random_device rd;
  std::default_random_engine rng(rd());
  std::shuffle(componentArrays.begin(), componentArrays.end(), rng);

  for (auto& inOutArrays : componentArrays)
  {
    inOutArrays.second.Allocate(originalArray.GetNumberOfValues());

    auto inPortal = inOutArrays.first.ReadPortal();
    auto outPortal = inOutArrays.second.WritePortal();
    VISKORES_TEST_ASSERT(inPortal.GetNumberOfValues() == originalArray.GetNumberOfValues());
    VISKORES_TEST_ASSERT(outPortal.GetNumberOfValues() == originalArray.GetNumberOfValues());
    for (viskores::Id arrayIndex = 0; arrayIndex < originalArray.GetNumberOfValues(); ++arrayIndex)
    {
      outPortal.Set(arrayIndex, inPortal.Get(arrayIndex));
    }
  }

  auto inPortal = originalArray.ReadPortal();
  auto outPortal = outputArray.ReadPortal();
  for (viskores::Id arrayIndex = 0; arrayIndex < originalArray.GetNumberOfValues(); ++arrayIndex)
  {
    auto inValue = inPortal.Get(arrayIndex);
    auto outValue = outPortal.Get(arrayIndex);
    for (viskores::IdComponent componentId = 0; componentId < numComponents; ++componentId)
    {
      VISKORES_TEST_ASSERT(test_equal(GetVecFlatIndex(inValue, componentId),
                                      GetVecFlatIndex(outValue, numComponents - componentId - 1)));
    }
  }

  // Check to make sure you can fill extracted components with a constant value.
  for (viskores::IdComponent componentId = 0; componentId < numComponents; ++componentId)
  {
    auto componentArray =
      viskores::cont::ArrayExtractComponent(outputArray, componentId, viskores::CopyFlag::Off);
    componentArray.Fill(TestValue(componentId, ComponentType{}));
  }
  for (viskores::IdComponent componentId = 0; componentId < numComponents; ++componentId)
  {
    auto componentArray =
      viskores::cont::ArrayExtractComponent(outputArray, componentId, viskores::CopyFlag::Off);
    auto constantArray = viskores::cont::make_ArrayHandleConstant(
      TestValue(componentId, ComponentType{}), originalArray.GetNumberOfValues());
    VISKORES_TEST_ASSERT(test_equal_ArrayHandles(componentArray, constantArray));
  }
}

void DoTest()
{
  using ArrayMultiplexerType =
    viskores::cont::ArrayHandleMultiplexer<viskores::cont::ArrayHandleBasic<viskores::Vec3f>,
                                           viskores::cont::ArrayHandleSOA<viskores::Vec3f>>;

  {
    std::cout << "Basic array" << std::endl;
    viskores::cont::ArrayHandle<viskores::Vec3f> array;
    array.Allocate(ARRAY_SIZE);
    SetPortal(array.WritePortal());
    CheckOutputArray(array);

    std::cout << "ArrayHandleExtractComponent" << std::endl;
    CheckOutputArray(viskores::cont::make_ArrayHandleExtractComponent(array, 1));

    std::cout << "ArrayHandleMultiplexer" << std::endl;
    CheckInputArray(ArrayMultiplexerType(array));
  }

  {
    std::cout << "SOA array" << std::endl;
    viskores::cont::ArrayHandleSOA<viskores::Vec3f> array;
    array.Allocate(ARRAY_SIZE);
    SetPortal(array.WritePortal());
    CheckOutputArray(array);

    CheckInputArray(ArrayMultiplexerType(array));
  }

  {
    std::cout << "Stride array" << std::endl;
    constexpr viskores::Id STRIDE = 7;
    viskores::cont::ArrayHandleBasic<viskores::Vec3f> originalArray;
    originalArray.Allocate(ARRAY_SIZE * STRIDE);
    SetPortal(originalArray.WritePortal());
    for (viskores::Id offset = 0; offset < STRIDE; ++offset)
    {
      viskores::cont::ArrayHandleStride<viskores::Vec3f> strideArray(
        originalArray, ARRAY_SIZE, STRIDE, offset);
      CheckInputArray(strideArray);
    }
  }

  {
    std::cout << "ArrayHandleGroupVec" << std::endl;
    viskores::cont::ArrayHandle<viskores::Vec3f> array;
    array.Allocate(ARRAY_SIZE * 4);
    SetPortal(array.WritePortal());
    CheckOutputArray(viskores::cont::make_ArrayHandleGroupVec<2>(array));
    CheckOutputArray(viskores::cont::make_ArrayHandleGroupVec<4>(array));
  }

  {
    std::cout << "ArrayHandleCompositeVector" << std::endl;
    viskores::cont::ArrayHandle<viskores::Vec3f> array0;
    viskores::cont::ArrayHandle<viskores::Vec3f> array1;
    array0.Allocate(ARRAY_SIZE);
    array1.Allocate(ARRAY_SIZE);
    SetPortal(array0.WritePortal());
    SetPortal(array1.WritePortal());
    auto compositeArray = viskores::cont::make_ArrayHandleCompositeVector(array0, array1);
    CheckOutputArray(compositeArray);

    // Note that when the extracted component array gets allocated, it only allocates the
    // array it was given. This is a weird case when using `ArrayHandleExtractComponent`
    // on something that has multiple arrays as input. It works fine if all components get
    // extracted and updated, but can cause issues if only one is resized. In this case
    // just test the input.
    CheckInputArray(viskores::cont::make_ArrayHandleExtractComponent(compositeArray, 1));
  }

  {
    std::cout << "ArrayHandleRuntimeVec" << std::endl;
    viskores::cont::ArrayHandle<viskores::FloatDefault> array;
    array.Allocate(ARRAY_SIZE * 4);
    SetPortal(array.WritePortal());
    CheckOutputArray(viskores::cont::make_ArrayHandleRuntimeVec(2, array),
                     viskores::cont::ArrayHandleRuntimeVec<viskores::FloatDefault>(2));
    CheckOutputArray(viskores::cont::make_ArrayHandleRuntimeVec(4, array),
                     viskores::cont::ArrayHandleRuntimeVec<viskores::FloatDefault>(4));
  }

  {
    std::cout << "ArrayHandleCartesianProduct" << std::endl;
    viskores::cont::ArrayHandle<viskores::Float64> array0;
    viskores::cont::ArrayHandle<viskores::Float64> array1;
    viskores::cont::ArrayHandle<viskores::Float64> array2;
    array0.Allocate(ARRAY_SIZE);
    array1.Allocate(ARRAY_SIZE / 2);
    array2.Allocate(ARRAY_SIZE + 2);
    SetPortal(array0.WritePortal());
    SetPortal(array1.WritePortal());
    SetPortal(array2.WritePortal());
    CheckInputArray(viskores::cont::make_ArrayHandleCartesianProduct(array0, array1, array2));
  }

  {
    std::cout << "ArrayHandleUniformPointCoordinates" << std::endl;
    viskores::cont::ArrayHandleUniformPointCoordinates array(
      viskores::Id3{ ARRAY_SIZE, ARRAY_SIZE + 2, ARRAY_SIZE / 2 });
    CheckInputArray(array, viskores::CopyFlag::On);
  }

  {
    std::cout << "ArrayHandleReverse" << std::endl;
    viskores::cont::ArrayHandle<viskores::Vec3f> array;
    array.Allocate(ARRAY_SIZE);
    SetPortal(array.WritePortal());
    CheckOutputArray(viskores::cont::make_ArrayHandleReverse(array));
  }

  {
    std::cout << "ArrayHandleView" << std::endl;
    viskores::cont::ArrayHandle<viskores::Vec3f> array;
    array.Allocate(ARRAY_SIZE);
    SetPortal(array.WritePortal());
    CheckInputArray(
      viskores::cont::make_ArrayHandleView(array, (ARRAY_SIZE / 3), (ARRAY_SIZE / 3) + 1));
  }

  {
    std::cout << "ArrayHandleIndex (expect warning)" << std::endl;
    viskores::cont::ArrayHandleIndex array(ARRAY_SIZE);
    CheckInputArray(array, viskores::CopyFlag::On);
  }

  {
    std::cout << "ArrayHandleConstant" << std::endl;
    viskores::cont::ArrayHandleConstant<viskores::Vec3f> array(TestValue(0, viskores::Vec3f{}),
                                                               ARRAY_SIZE);
    CheckInputArray(array, viskores::CopyFlag::On);
  }

  {
    std::cout << "Weird combination." << std::endl;

    viskores::cont::ArrayHandle<viskores::Vec<viskores::Vec4f, 2>> base0;
    base0.Allocate(ARRAY_SIZE);
    SetPortal(base0.WritePortal());

    viskores::cont::ArrayHandleSOA<viskores::Vec4f> base1_sub;
    base1_sub.Allocate(ARRAY_SIZE);
    SetPortal(base1_sub.WritePortal());
    auto base1 = viskores::cont::make_ArrayHandleGroupVec<2>(base1_sub);

    viskores::cont::ArrayHandle<viskores::Vec<viskores::Vec4f, 2>> base2_sub;
    base2_sub.Allocate(ARRAY_SIZE + 10);
    SetPortal(base2_sub.WritePortal());
    auto base2 = viskores::cont::make_ArrayHandleView(base2_sub, 2, ARRAY_SIZE + 4);

    auto array = viskores::cont::make_ArrayHandleCartesianProduct(base0, base1, base2);
    CheckInputArray(array);
  }
}

} // anonymous namespace

int UnitTestArrayExtractComponent(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(DoTest, argc, argv);
}
