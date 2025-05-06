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

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCompositeVector.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/ArrayHandleSwizzle.h>

#include <viskores/cont/testing/Testing.h>

#include <type_traits>

namespace
{

template <typename ValueType>
struct SwizzleTests
{
  static constexpr viskores::IdComponent InSize = 4;
  using SwizzleInputArrayType = viskores::cont::ArrayHandle<viskores::Vec<ValueType, InSize>>;

  template <viskores::IdComponent OutSize>
  using SwizzleArrayType = viskores::cont::ArrayHandleSwizzle<SwizzleInputArrayType, OutSize>;

  using ReferenceComponentArrayType = viskores::cont::ArrayHandleCounting<ValueType>;
  using ReferenceArrayType =
    viskores::cont::ArrayHandleCompositeVector<ReferenceComponentArrayType,
                                               ReferenceComponentArrayType,
                                               ReferenceComponentArrayType,
                                               ReferenceComponentArrayType>;

  template <viskores::IdComponent Size>
  using MapType = viskores::Vec<viskores::IdComponent, Size>;

  using Algo = viskores::cont::Algorithm;

  // This is used to build a ArrayHandleSwizzle's internal array.
  ReferenceArrayType RefArray;

  void ConstructReferenceArray()
  {
    // Build the Ref array
    const viskores::Id numValues = 32;
    ReferenceComponentArrayType c1 =
      viskores::cont::make_ArrayHandleCounting<ValueType>(3, 2, numValues);
    ReferenceComponentArrayType c2 =
      viskores::cont::make_ArrayHandleCounting<ValueType>(2, 3, numValues);
    ReferenceComponentArrayType c3 =
      viskores::cont::make_ArrayHandleCounting<ValueType>(4, 4, numValues);
    ReferenceComponentArrayType c4 =
      viskores::cont::make_ArrayHandleCounting<ValueType>(1, 3, numValues);

    this->RefArray = viskores::cont::make_ArrayHandleCompositeVector(c1, c2, c3, c4);
  }

  SwizzleInputArrayType BuildSwizzleInputArray() const
  {
    SwizzleInputArrayType result;
    Algo::Copy(this->RefArray, result);
    return result;
  }

  template <viskores::IdComponent OutSize>
  void SanityCheck(const MapType<OutSize>& map) const
  {
    SwizzleInputArrayType input = this->BuildSwizzleInputArray();
    auto swizzle = viskores::cont::make_ArrayHandleSwizzle(input, map);

    VISKORES_TEST_ASSERT(input.GetNumberOfValues() == swizzle.GetNumberOfValues(),
                         "Number of values in copied Swizzle array does not match input.");
  }

  template <viskores::IdComponent OutSize>
  void ReadTest(const MapType<OutSize>& map) const
  {
    // Test that the expected values are read from an Swizzle array.
    SwizzleInputArrayType input = this->BuildSwizzleInputArray();
    auto swizzle = viskores::cont::make_ArrayHandleSwizzle(input, map);

    // Test reading the data back in the control env:
    this->ValidateReadTest(swizzle, map);

    // Copy the extracted array in the execution environment to test reading:
    viskores::cont::ArrayHandle<viskores::Vec<ValueType, OutSize>> execCopy;
    Algo::Copy(swizzle, execCopy);
    this->ValidateReadTest(execCopy, map);
  }

  template <typename ArrayHandleType, viskores::IdComponent OutSize>
  void ValidateReadTest(ArrayHandleType testArray, const MapType<OutSize>& map) const
  {
    using ReferenceVectorType = typename ReferenceArrayType::ValueType;
    using SwizzleVectorType = viskores::Vec<ValueType, OutSize>;

    VISKORES_TEST_ASSERT(map.GetNumberOfComponents() ==
                           viskores::VecTraits<SwizzleVectorType>::NUM_COMPONENTS,
                         "Unexpected runtime component map size.");
    VISKORES_TEST_ASSERT(testArray.GetNumberOfValues() == this->RefArray.GetNumberOfValues(),
                         "Number of values incorrect in Read test.");

    auto refPortal = this->RefArray.ReadPortal();
    auto testPortal = testArray.ReadPortal();

    SwizzleVectorType refVecSwizzle(viskores::TypeTraits<SwizzleVectorType>::ZeroInitialization());
    for (viskores::Id i = 0; i < testArray.GetNumberOfValues(); ++i)
    {
      ReferenceVectorType refVec = refPortal.Get(i);

      // Manually swizzle the reference vector using the runtime map information:
      for (viskores::IdComponent j = 0; j < map.GetNumberOfComponents(); ++j)
      {
        refVecSwizzle[j] = refVec[map[j]];
      }

      VISKORES_TEST_ASSERT(test_equal(refVecSwizzle, testPortal.Get(i), 0.),
                           "Invalid value encountered in Read test.");
    }
  }

  // Doubles everything in the input portal.
  template <typename PortalType>
  struct WriteTestFunctor : viskores::exec::FunctorBase
  {
    PortalType Portal;

    VISKORES_CONT
    WriteTestFunctor(const PortalType& portal)
      : Portal(portal)
    {
    }

    VISKORES_EXEC_CONT
    void operator()(viskores::Id index) const
    {
      this->Portal.Set(index, this->Portal.Get(index) * 2.);
    }
  };

  struct WriteExec
  {
    template <typename DeviceTag, typename SwizzleHandleType>
    bool operator()(DeviceTag, SwizzleHandleType& swizzle) const
    {
      viskores::cont::Token token;
      using Portal = typename SwizzleHandleType::WritePortalType;
      WriteTestFunctor<Portal> functor(swizzle.PrepareForInPlace(DeviceTag(), token));
      Algo::Schedule(functor, swizzle.GetNumberOfValues());
      return true;
    }
  };


  template <viskores::IdComponent OutSize>
  void WriteTest(const MapType<OutSize>& map, std::true_type) const
  {
    // Control test:
    {
      SwizzleInputArrayType input = this->BuildSwizzleInputArray();
      auto swizzle = viskores::cont::make_ArrayHandleSwizzle(input, map);

      {
        WriteTestFunctor<typename SwizzleArrayType<OutSize>::WritePortalType> functor(
          swizzle.WritePortal());

        for (viskores::Id i = 0; i < swizzle.GetNumberOfValues(); ++i)
        {
          functor(i);
        }
      }

      this->ValidateWriteTestArray(input, map);
    }

    // Exec test:
    {
      SwizzleInputArrayType input = this->BuildSwizzleInputArray();
      auto swizzle = viskores::cont::make_ArrayHandleSwizzle(input, map);

      viskores::cont::TryExecute(WriteExec{}, swizzle);
      this->ValidateWriteTestArray(input, map);
    }
  }

  template <viskores::IdComponent OutSize>
  void WriteTest(const MapType<OutSize>&, std::false_type) const
  {
    // Array is not writable
  }

  template <viskores::IdComponent OutSize>
  void WriteTest(const MapType<OutSize>& map) const
  {
    this->WriteTest(map, std::integral_constant<bool, OutSize == InSize>{});
  }

  // Check that the swizzled components are twice the reference value.
  template <viskores::IdComponent OutSize>
  void ValidateWriteTestArray(SwizzleInputArrayType testArray, const MapType<OutSize>& map) const
  {
    auto refPortal = this->RefArray.ReadPortal();
    auto portal = testArray.ReadPortal();

    VISKORES_TEST_ASSERT(portal.GetNumberOfValues() == refPortal.GetNumberOfValues(),
                         "Number of values in write test output do not match input.");

    for (viskores::Id i = 0; i < portal.GetNumberOfValues(); ++i)
    {
      auto value = portal.Get(i);
      auto refValue = refPortal.Get(i);

      // Double all of the components that appear in the map to replicate the
      // test result:
      for (viskores::IdComponent j = 0; j < map.GetNumberOfComponents(); ++j)
      {
        refValue[map[j]] *= 2;
      }

      VISKORES_TEST_ASSERT(test_equal(refValue, value, 0.), "Value mismatch in Write test.");
    }
  }

  template <viskores::IdComponent OutSize>
  void TestSwizzle(const MapType<OutSize>& map) const
  {
    this->SanityCheck(map);
    this->ReadTest(map);

    this->WriteTest(map);
  }

  void operator()()
  {
    this->ConstructReferenceArray();

    this->TestSwizzle(viskores::make_Vec(0, 1));
    this->TestSwizzle(viskores::make_Vec(0, 2));
    this->TestSwizzle(viskores::make_Vec(0, 3));
    this->TestSwizzle(viskores::make_Vec(1, 0));
    this->TestSwizzle(viskores::make_Vec(1, 2));
    this->TestSwizzle(viskores::make_Vec(1, 3));
    this->TestSwizzle(viskores::make_Vec(2, 0));
    this->TestSwizzle(viskores::make_Vec(2, 1));
    this->TestSwizzle(viskores::make_Vec(2, 3));
    this->TestSwizzle(viskores::make_Vec(3, 0));
    this->TestSwizzle(viskores::make_Vec(3, 1));
    this->TestSwizzle(viskores::make_Vec(3, 2));
    this->TestSwizzle(viskores::make_Vec(0, 1, 2));
    this->TestSwizzle(viskores::make_Vec(0, 1, 3));
    this->TestSwizzle(viskores::make_Vec(0, 2, 1));
    this->TestSwizzle(viskores::make_Vec(0, 2, 3));
    this->TestSwizzle(viskores::make_Vec(0, 3, 1));
    this->TestSwizzle(viskores::make_Vec(0, 3, 2));
    this->TestSwizzle(viskores::make_Vec(1, 0, 2));
    this->TestSwizzle(viskores::make_Vec(1, 0, 3));
    this->TestSwizzle(viskores::make_Vec(1, 2, 0));
    this->TestSwizzle(viskores::make_Vec(1, 2, 3));
    this->TestSwizzle(viskores::make_Vec(1, 3, 0));
    this->TestSwizzle(viskores::make_Vec(1, 3, 2));
    this->TestSwizzle(viskores::make_Vec(2, 0, 1));
    this->TestSwizzle(viskores::make_Vec(2, 0, 3));
    this->TestSwizzle(viskores::make_Vec(2, 1, 0));
    this->TestSwizzle(viskores::make_Vec(2, 1, 3));
    this->TestSwizzle(viskores::make_Vec(2, 3, 0));
    this->TestSwizzle(viskores::make_Vec(2, 3, 1));
    this->TestSwizzle(viskores::make_Vec(3, 0, 1));
    this->TestSwizzle(viskores::make_Vec(3, 0, 2));
    this->TestSwizzle(viskores::make_Vec(3, 1, 0));
    this->TestSwizzle(viskores::make_Vec(3, 1, 2));
    this->TestSwizzle(viskores::make_Vec(3, 2, 0));
    this->TestSwizzle(viskores::make_Vec(3, 2, 1));
    this->TestSwizzle(viskores::make_Vec(0, 1, 2, 3));
    this->TestSwizzle(viskores::make_Vec(0, 1, 3, 2));
    this->TestSwizzle(viskores::make_Vec(0, 2, 1, 3));
    this->TestSwizzle(viskores::make_Vec(0, 2, 3, 1));
    this->TestSwizzle(viskores::make_Vec(0, 3, 1, 2));
    this->TestSwizzle(viskores::make_Vec(0, 3, 2, 1));
    this->TestSwizzle(viskores::make_Vec(1, 0, 2, 3));
    this->TestSwizzle(viskores::make_Vec(1, 0, 3, 2));
    this->TestSwizzle(viskores::make_Vec(1, 2, 0, 3));
    this->TestSwizzle(viskores::make_Vec(1, 2, 3, 0));
    this->TestSwizzle(viskores::make_Vec(1, 3, 0, 2));
    this->TestSwizzle(viskores::make_Vec(1, 3, 2, 0));
    this->TestSwizzle(viskores::make_Vec(2, 0, 1, 3));
    this->TestSwizzle(viskores::make_Vec(2, 0, 3, 1));
    this->TestSwizzle(viskores::make_Vec(2, 1, 0, 3));
    this->TestSwizzle(viskores::make_Vec(2, 1, 3, 0));
    this->TestSwizzle(viskores::make_Vec(2, 3, 0, 1));
    this->TestSwizzle(viskores::make_Vec(2, 3, 1, 0));
    this->TestSwizzle(viskores::make_Vec(3, 0, 1, 2));
    this->TestSwizzle(viskores::make_Vec(3, 0, 2, 1));
    this->TestSwizzle(viskores::make_Vec(3, 1, 0, 2));
    this->TestSwizzle(viskores::make_Vec(3, 1, 2, 0));
    this->TestSwizzle(viskores::make_Vec(3, 2, 0, 1));
    this->TestSwizzle(viskores::make_Vec(3, 2, 1, 0));
  }
};

struct ArgToTemplateType
{
  template <typename ValueType>
  void operator()(ValueType) const
  {
    SwizzleTests<ValueType>()();
  }
};

void TestArrayHandleSwizzle()
{
  using TestTypes =
    viskores::List<viskores::Int32, viskores::Int64, viskores::Float32, viskores::Float64>;
  viskores::testing::Testing::TryTypes(ArgToTemplateType(), TestTypes());
}

} // end anon namespace

int UnitTestArrayHandleSwizzle(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestArrayHandleSwizzle, argc, argv);
}
