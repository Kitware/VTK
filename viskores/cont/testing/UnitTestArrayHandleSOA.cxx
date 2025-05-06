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

#include <viskores/cont/ArrayHandleSOA.h>

#include <viskores/cont/ArrayCopyDevice.h>
#include <viskores/cont/Invoker.h>

#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

constexpr viskores::Id ARRAY_SIZE = 10;

using ScalarTypesToTest = viskores::List<viskores::UInt8, viskores::FloatDefault>;
using VectorTypesToTest = viskores::List<viskores::Vec2i_8, viskores::Vec3f_32>;

struct PassThrough : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename InValue, typename OutValue>
  VISKORES_EXEC void operator()(const InValue& inValue, OutValue& outValue) const
  {
    outValue = inValue;
  }
};

struct TestArrayPortalSOA
{
  template <typename ComponentType>
  VISKORES_CONT void operator()(ComponentType) const
  {
    constexpr viskores::IdComponent NUM_COMPONENTS = 4;
    using ValueType = viskores::Vec<ComponentType, NUM_COMPONENTS>;
    using ComponentArrayType = viskores::cont::ArrayHandle<ComponentType>;
    using SOAPortalType =
      viskores::internal::ArrayPortalSOA<ValueType, typename ComponentArrayType::WritePortalType>;

    std::cout << "Test SOA portal reflects data in component portals." << std::endl;
    SOAPortalType soaPortalIn(ARRAY_SIZE);

    std::array<viskores::cont::ArrayHandle<ComponentType>, NUM_COMPONENTS> implArrays;
    for (viskores::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS;
         ++componentIndex)
    {
      viskores::cont::ArrayHandle<ComponentType> array;
      array.Allocate(ARRAY_SIZE);
      auto portal = array.WritePortal();
      for (viskores::IdComponent valueIndex = 0; valueIndex < ARRAY_SIZE; ++valueIndex)
      {
        portal.Set(valueIndex, TestValue(valueIndex, ValueType{})[componentIndex]);
      }

      soaPortalIn.SetPortal(componentIndex, portal);

      implArrays[static_cast<std::size_t>(componentIndex)] = array;
    }

    VISKORES_TEST_ASSERT(soaPortalIn.GetNumberOfValues() == ARRAY_SIZE);
    CheckPortal(soaPortalIn);

    std::cout << "Test data set in SOA portal gets set in component portals." << std::endl;
    {
      SOAPortalType soaPortalOut(ARRAY_SIZE);
      for (viskores::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS;
           ++componentIndex)
      {
        viskores::cont::ArrayHandle<ComponentType> array;
        array.Allocate(ARRAY_SIZE);
        auto portal = array.WritePortal();
        soaPortalOut.SetPortal(componentIndex, portal);

        implArrays[static_cast<std::size_t>(componentIndex)] = array;
      }

      SetPortal(soaPortalOut);
    }

    for (viskores::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS;
         ++componentIndex)
    {
      auto portal = implArrays[static_cast<size_t>(componentIndex)].ReadPortal();
      for (viskores::Id valueIndex = 0; valueIndex < ARRAY_SIZE; ++valueIndex)
      {
        ComponentType x = TestValue(valueIndex, ValueType{})[componentIndex];
        VISKORES_TEST_ASSERT(test_equal(x, portal.Get(valueIndex)));
      }
    }
  }
};

struct TestSOAAsInput
{
  template <typename ValueType>
  VISKORES_CONT void operator()(const ValueType viskoresNotUsed(v)) const
  {
    using VTraits = viskores::VecTraits<ValueType>;
    using ComponentType = typename VTraits::ComponentType;
    constexpr viskores::IdComponent NUM_COMPONENTS = VTraits::NUM_COMPONENTS;

    {
      viskores::cont::ArrayHandleSOA<ValueType> soaArray;
      for (viskores::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS;
           ++componentIndex)
      {
        viskores::cont::ArrayHandle<ComponentType> componentArray;
        componentArray.Allocate(ARRAY_SIZE);
        auto componentPortal = componentArray.WritePortal();
        for (viskores::Id valueIndex = 0; valueIndex < ARRAY_SIZE; ++valueIndex)
        {
          componentPortal.Set(
            valueIndex, VTraits::GetComponent(TestValue(valueIndex, ValueType{}), componentIndex));
        }
        soaArray.SetArray(componentIndex, componentArray);
      }

      VISKORES_TEST_ASSERT(soaArray.GetNumberOfComponentsFlat() ==
                           viskores::VecFlat<ValueType>::NUM_COMPONENTS);
      VISKORES_TEST_ASSERT(soaArray.GetNumberOfValues() == ARRAY_SIZE);
      VISKORES_TEST_ASSERT(soaArray.ReadPortal().GetNumberOfValues() == ARRAY_SIZE);
      CheckPortal(soaArray.ReadPortal());

      viskores::cont::ArrayHandle<ValueType> basicArray;
      viskores::cont::ArrayCopyDevice(soaArray, basicArray);
      VISKORES_TEST_ASSERT(basicArray.GetNumberOfValues() == ARRAY_SIZE);
      CheckPortal(basicArray.ReadPortal());
    }

    {
      // Check constructors
      using Vec3 = viskores::Vec<ComponentType, 3>;
      std::vector<ComponentType> vector0;
      std::vector<ComponentType> vector1;
      std::vector<ComponentType> vector2;
      for (viskores::Id valueIndex = 0; valueIndex < ARRAY_SIZE; ++valueIndex)
      {
        Vec3 value = TestValue(valueIndex, Vec3{});
        vector0.push_back(value[0]);
        vector1.push_back(value[1]);
        vector2.push_back(value[2]);
      }

      {
        viskores::cont::ArrayHandleSOA<Vec3> soaArray =
          viskores::cont::make_ArrayHandleSOA<Vec3>({ vector0, vector1, vector2 });
        VISKORES_TEST_ASSERT(soaArray.GetNumberOfValues() == ARRAY_SIZE);
        CheckPortal(soaArray.ReadPortal());
      }

      {
        viskores::cont::ArrayHandleSOA<Vec3> soaArray =
          viskores::cont::make_ArrayHandleSOA(viskores::CopyFlag::Off, vector0, vector1, vector2);
        VISKORES_TEST_ASSERT(soaArray.GetNumberOfValues() == ARRAY_SIZE);
        CheckPortal(soaArray.ReadPortal());

        // Make sure calling ReleaseResources does not result in error.
        soaArray.ReleaseResources();
      }

      {
        viskores::cont::ArrayHandleSOA<Vec3> soaArray = viskores::cont::make_ArrayHandleSOA<Vec3>(
          { vector0.data(), vector1.data(), vector2.data() }, ARRAY_SIZE, viskores::CopyFlag::Off);
        VISKORES_TEST_ASSERT(soaArray.GetNumberOfValues() == ARRAY_SIZE);
        CheckPortal(soaArray.ReadPortal());
      }

      {
        viskores::cont::ArrayHandleSOA<Vec3> soaArray = viskores::cont::make_ArrayHandleSOA(
          ARRAY_SIZE, viskores::CopyFlag::Off, vector0.data(), vector1.data(), vector2.data());
        VISKORES_TEST_ASSERT(soaArray.GetNumberOfValues() == ARRAY_SIZE);
        CheckPortal(soaArray.ReadPortal());
      }
    }
  }
};

struct TestSOAAsOutput
{
  template <typename ValueType>
  VISKORES_CONT void operator()(const ValueType viskoresNotUsed(v)) const
  {
    using VTraits = viskores::VecTraits<ValueType>;
    using ComponentType = typename VTraits::ComponentType;
    constexpr viskores::IdComponent NUM_COMPONENTS = VTraits::NUM_COMPONENTS;

    viskores::cont::ArrayHandle<ValueType> basicArray;
    basicArray.Allocate(ARRAY_SIZE);
    SetPortal(basicArray.WritePortal());

    viskores::cont::ArrayHandleSOA<ValueType> soaArray;
    viskores::cont::Invoker{}(PassThrough{}, basicArray, soaArray);

    VISKORES_TEST_ASSERT(soaArray.GetNumberOfValues() == ARRAY_SIZE);
    for (viskores::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS;
         ++componentIndex)
    {
      viskores::cont::ArrayHandle<ComponentType> componentArray = soaArray.GetArray(componentIndex);
      auto componentPortal = componentArray.ReadPortal();
      for (viskores::Id valueIndex = 0; valueIndex < ARRAY_SIZE; ++valueIndex)
      {
        ComponentType expected =
          VTraits::GetComponent(TestValue(valueIndex, ValueType{}), componentIndex);
        ComponentType got = componentPortal.Get(valueIndex);
        VISKORES_TEST_ASSERT(test_equal(expected, got));
      }
    }
  }
};

static void Run()
{
  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayPortalSOA" << std::endl;
  viskores::testing::Testing::TryTypes(TestArrayPortalSOA(), ScalarTypesToTest());

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleSOA as Input" << std::endl;
  viskores::testing::Testing::TryTypes(TestSOAAsInput(), VectorTypesToTest());

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleSOA as Output" << std::endl;
  viskores::testing::Testing::TryTypes(TestSOAAsOutput(), VectorTypesToTest());
}

} // anonymous namespace

int UnitTestArrayHandleSOA(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Run, argc, argv);
}
