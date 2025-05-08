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
#include <viskores/cont/ArrayHandleCompositeVector.h>

#include <viskores/VecTraits.h>

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleExtractComponent.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/serial/DeviceAdapterSerial.h>

#include <viskores/cont/testing/Testing.h>

#include <vector>

namespace
{

const viskores::Id ARRAY_SIZE = 10;

using StorageTag = viskores::cont::StorageTagBasic;

viskores::FloatDefault TestValue3Ids(viskores::Id index,
                                     viskores::IdComponent inComponentIndex,
                                     int inArrayId)
{
  return (viskores::FloatDefault(index) + 0.1f * viskores::FloatDefault(inComponentIndex) +
          0.01f * viskores::FloatDefault(inArrayId));
}

template <typename ValueType>
viskores::cont::ArrayHandle<ValueType, StorageTag> MakeInputArray(int arrayId)
{
  using VTraits = viskores::VecTraits<ValueType>;

  // Create a buffer with valid test values.
  ValueType buffer[ARRAY_SIZE];
  for (viskores::Id index = 0; index < ARRAY_SIZE; index++)
  {
    for (viskores::IdComponent componentIndex = 0; componentIndex < VTraits::NUM_COMPONENTS;
         componentIndex++)
    {
      VTraits::SetComponent(
        buffer[index], componentIndex, TestValue3Ids(index, componentIndex, arrayId));
    }
  }

  // Make an array handle that points to this buffer.
  return viskores::cont::make_ArrayHandle(buffer, ARRAY_SIZE, viskores::CopyFlag::On);
}

template <typename ValueType, typename C>
void CheckArray(const viskores::cont::ArrayHandle<ValueType, C>& outArray,
                const viskores::IdComponent* inComponents,
                const int* inArrayIds)
{
  // ArrayHandleCompositeVector currently does not implement the ability to
  // get to values on the control side, so copy to an array that is accessible.
  using ArrayHandleType = viskores::cont::ArrayHandle<ValueType, StorageTag>;
  ArrayHandleType arrayCopy;
  viskores::cont::ArrayCopy(outArray, arrayCopy);

  typename ArrayHandleType::ReadPortalType portal = arrayCopy.ReadPortal();
  using VTraits = viskores::VecTraits<ValueType>;
  for (viskores::Id index = 0; index < ARRAY_SIZE; index++)
  {
    ValueType retreivedValue = portal.Get(index);
    for (viskores::IdComponent componentIndex = 0; componentIndex < VTraits::NUM_COMPONENTS;
         componentIndex++)
    {
      viskores::FloatDefault retrievedComponent =
        VTraits::GetComponent(retreivedValue, componentIndex);
      viskores::FloatDefault expectedComponent =
        TestValue3Ids(index, inComponents[componentIndex], inArrayIds[componentIndex]);
      VISKORES_TEST_ASSERT(retrievedComponent == expectedComponent, "Got bad value.");
    }
  }
}

template <viskores::IdComponent inComponents>
void TryScalarArray()
{
  std::cout << "Creating a scalar array from one of " << inComponents << " components."
            << std::endl;

  using InValueType = viskores::Vec<viskores::FloatDefault, inComponents>;
  using InArrayType = viskores::cont::ArrayHandle<InValueType, StorageTag>;
  int inArrayId = 0;
  InArrayType inArray = MakeInputArray<InValueType>(inArrayId);

  for (viskores::IdComponent inComponentIndex = 0; inComponentIndex < inComponents;
       inComponentIndex++)
  {
    auto c1 = viskores::cont::make_ArrayHandleExtractComponent(inArray, inComponentIndex);
    auto composite = viskores::cont::make_ArrayHandleCompositeVector(c1);
    CheckArray(composite, &inComponentIndex, &inArrayId);
  }
}

template <typename T1, typename T2, typename T3, typename T4>
void TryVector4(viskores::cont::ArrayHandle<T1, StorageTag> array1,
                viskores::cont::ArrayHandle<T2, StorageTag> array2,
                viskores::cont::ArrayHandle<T3, StorageTag> array3,
                viskores::cont::ArrayHandle<T4, StorageTag> array4)
{
  int arrayIds[4] = { 0, 1, 2, 3 };
  viskores::IdComponent inComponents[4];

  for (inComponents[0] = 0; inComponents[0] < viskores::VecTraits<T1>::NUM_COMPONENTS;
       inComponents[0]++)
  {
    auto c1 = viskores::cont::make_ArrayHandleExtractComponent(array1, inComponents[0]);
    for (inComponents[1] = 0; inComponents[1] < viskores::VecTraits<T2>::NUM_COMPONENTS;
         inComponents[1]++)
    {
      auto c2 = viskores::cont::make_ArrayHandleExtractComponent(array2, inComponents[1]);
      for (inComponents[2] = 0; inComponents[2] < viskores::VecTraits<T3>::NUM_COMPONENTS;
           inComponents[2]++)
      {
        auto c3 = viskores::cont::make_ArrayHandleExtractComponent(array3, inComponents[2]);
        for (inComponents[3] = 0; inComponents[3] < viskores::VecTraits<T4>::NUM_COMPONENTS;
             inComponents[3]++)
        {
          auto c4 = viskores::cont::make_ArrayHandleExtractComponent(array4, inComponents[3]);
          CheckArray(viskores::cont::make_ArrayHandleCompositeVector(c1, c2, c3, c4),
                     inComponents,
                     arrayIds);
        }
      }
    }
  }
}

template <typename T1, typename T2, typename T3>
void TryVector3(viskores::cont::ArrayHandle<T1, StorageTag> array1,
                viskores::cont::ArrayHandle<T2, StorageTag> array2,
                viskores::cont::ArrayHandle<T3, StorageTag> array3)
{
  int arrayIds[3] = { 0, 1, 2 };
  viskores::IdComponent inComponents[3];

  for (inComponents[0] = 0; inComponents[0] < viskores::VecTraits<T1>::NUM_COMPONENTS;
       inComponents[0]++)
  {
    auto c1 = viskores::cont::make_ArrayHandleExtractComponent(array1, inComponents[0]);
    for (inComponents[1] = 0; inComponents[1] < viskores::VecTraits<T2>::NUM_COMPONENTS;
         inComponents[1]++)
    {
      auto c2 = viskores::cont::make_ArrayHandleExtractComponent(array2, inComponents[1]);
      for (inComponents[2] = 0; inComponents[2] < viskores::VecTraits<T3>::NUM_COMPONENTS;
           inComponents[2]++)
      {
        auto c3 = viskores::cont::make_ArrayHandleExtractComponent(array3, inComponents[2]);
        CheckArray(
          viskores::cont::make_ArrayHandleCompositeVector(c1, c2, c3), inComponents, arrayIds);
      }
    }
  }

  std::cout << "        Fourth component from Scalar." << std::endl;
  TryVector4(array1, array2, array3, MakeInputArray<viskores::FloatDefault>(3));
  std::cout << "        Fourth component from Vector4." << std::endl;
  TryVector4(array1, array2, array3, MakeInputArray<viskores::Vec4f>(3));
}

template <typename T1, typename T2>
void TryVector2(viskores::cont::ArrayHandle<T1, StorageTag> array1,
                viskores::cont::ArrayHandle<T2, StorageTag> array2)
{
  int arrayIds[2] = { 0, 1 };
  viskores::IdComponent inComponents[2];

  for (inComponents[0] = 0; inComponents[0] < viskores::VecTraits<T1>::NUM_COMPONENTS;
       inComponents[0]++)
  {
    auto c1 = viskores::cont::make_ArrayHandleExtractComponent(array1, inComponents[0]);
    for (inComponents[1] = 0; inComponents[1] < viskores::VecTraits<T2>::NUM_COMPONENTS;
         inComponents[1]++)
    {
      auto c2 = viskores::cont::make_ArrayHandleExtractComponent(array2, inComponents[1]);
      CheckArray(viskores::cont::make_ArrayHandleCompositeVector(c1, c2), inComponents, arrayIds);
    }
  }

  std::cout << "      Third component from Scalar." << std::endl;
  TryVector3(array1, array2, MakeInputArray<viskores::FloatDefault>(2));
  std::cout << "      Third component from Vector2." << std::endl;
  TryVector3(array1, array2, MakeInputArray<viskores::Vec2f>(2));
}

template <typename T1>
void TryVector1(viskores::cont::ArrayHandle<T1, StorageTag> array1)
{
  int arrayIds[1] = { 0 };
  viskores::IdComponent inComponents[1];

  for (inComponents[0] = 0; inComponents[0] < viskores::VecTraits<T1>::NUM_COMPONENTS;
       inComponents[0]++)
  {
    auto testArray = viskores::cont::make_ArrayHandleExtractComponent(array1, inComponents[0]);
    CheckArray(viskores::cont::make_ArrayHandleCompositeVector(testArray), inComponents, arrayIds);
  }

  std::cout << "    Second component from Scalar." << std::endl;
  TryVector2(array1, MakeInputArray<viskores::FloatDefault>(1));
  std::cout << "    Second component from Vector4." << std::endl;
  TryVector2(array1, MakeInputArray<viskores::Vec4f>(1));
}

void TryVector()
{
  std::cout << "Trying many permutations of composite vectors." << std::endl;

  std::cout << "  First component from Scalar." << std::endl;
  TryVector1(MakeInputArray<viskores::FloatDefault>(0));
  std::cout << "  First component from Vector3." << std::endl;
  TryVector1(MakeInputArray<viskores::Vec3f>(0));
}

void TryFill()
{
  std::cout << "Trying fill." << std::endl;

  viskores::cont::ArrayHandle<viskores::FloatDefault> array0;
  viskores::cont::ArrayHandle<viskores::FloatDefault> array1;
  viskores::cont::ArrayHandle<viskores::FloatDefault> array2;

  auto composite = viskores::cont::make_ArrayHandleCompositeVector(array0, array1, array2);

  const viskores::Vec3f testValue = TestValue(0, viskores::Vec3f{});

  composite.AllocateAndFill(ARRAY_SIZE, testValue);

  auto portal0 = array0.ReadPortal();
  auto portal1 = array1.ReadPortal();
  auto portal2 = array2.ReadPortal();

  VISKORES_TEST_ASSERT(portal0.GetNumberOfValues() == ARRAY_SIZE);
  VISKORES_TEST_ASSERT(portal1.GetNumberOfValues() == ARRAY_SIZE);
  VISKORES_TEST_ASSERT(portal2.GetNumberOfValues() == ARRAY_SIZE);

  for (viskores::Id index = 0; index < ARRAY_SIZE; ++index)
  {
    VISKORES_TEST_ASSERT(portal0.Get(index) == testValue[0]);
    VISKORES_TEST_ASSERT(portal1.Get(index) == testValue[1]);
    VISKORES_TEST_ASSERT(portal2.Get(index) == testValue[2]);
  }
}

void TrySpecialArrays()
{
  std::cout << "Trying special arrays." << std::endl;

  using ArrayType1 = viskores::cont::ArrayHandleIndex;
  ArrayType1 array1(ARRAY_SIZE);

  using ArrayType2 = viskores::cont::ArrayHandleConstant<viskores::Id>;
  ArrayType2 array2(295, ARRAY_SIZE);

  auto compositeArray = viskores::cont::make_ArrayHandleCompositeVector(array1, array2);

  viskores::cont::printSummary_ArrayHandle(compositeArray, std::cout);
  std::cout << std::endl;

  VISKORES_TEST_ASSERT(compositeArray.GetNumberOfValues() == ARRAY_SIZE, "Wrong array size.");

  auto compositePortal = compositeArray.ReadPortal();
  for (viskores::Id index = 0; index < ARRAY_SIZE; index++)
  {
    VISKORES_TEST_ASSERT(test_equal(compositePortal.Get(index), viskores::Id2(index, 295)),
                         "Bad value.");
  }
}

void TestCompositeVector()
{
  TryScalarArray<2>();
  TryScalarArray<3>();
  TryScalarArray<4>();

  TryVector();

  TryFill();

  TrySpecialArrays();
}

} // anonymous namespace

int UnitTestArrayHandleCompositeVector(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestCompositeVector, argc, argv);
}
