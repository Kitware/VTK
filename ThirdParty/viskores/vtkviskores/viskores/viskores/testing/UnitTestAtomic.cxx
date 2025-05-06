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

#include <viskores/Atomic.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleBasic.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/DeviceAdapterTag.h>
#include <viskores/cont/ExecutionObjectBase.h>
#include <viskores/cont/Invoker.h>

#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

constexpr viskores::Id ARRAY_SIZE = 100;

template <typename T>
struct AtomicTests
{
  viskores::cont::Invoker Invoke;

  static constexpr viskores::Id OVERLAP = sizeof(T) * CHAR_BIT;
  static constexpr viskores::Id EXTENDED_SIZE = ARRAY_SIZE * OVERLAP;

  VISKORES_EXEC_CONT static T TestValue(viskores::Id index) { return ::TestValue(index, T{}); }

  struct ArrayToRawPointer : viskores::cont::ExecutionObjectBase
  {
    viskores::cont::ArrayHandleBasic<T> Array;
    VISKORES_CONT ArrayToRawPointer(const viskores::cont::ArrayHandleBasic<T>& array)
      : Array(array)
    {
    }

    VISKORES_CONT T* PrepareForExecution(viskores::cont::DeviceAdapterId device,
                                         viskores::cont::Token& token) const
    {
      return reinterpret_cast<T*>(this->Array.GetBuffers()[0].WritePointerDevice(device, token));
    }
  };

  struct LoadFunctor : viskores::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn ignored, ExecObject);
    using ExecutionSignature = void(WorkIndex, _2);

    VISKORES_EXEC void operator()(viskores::Id index, T* data) const
    {
      if (!test_equal(viskores::AtomicLoad(data + index), TestValue(index)))
      {
        this->RaiseError("Bad AtomicLoad");
      }
    }
  };

  VISKORES_CONT void TestLoad()
  {
    std::cout << "AtomicLoad" << std::endl;
    viskores::cont::ArrayHandleBasic<T> array;
    array.Allocate(ARRAY_SIZE);
    SetPortal(array.WritePortal());

    this->Invoke(LoadFunctor{}, array, ArrayToRawPointer(array));
  }

  struct StoreFunctor : viskores::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn ignored, ExecObject);
    using ExecutionSignature = void(WorkIndex, _2);

    VISKORES_EXEC void operator()(viskores::Id index, T* data) const
    {
      viskores::AtomicStore(data + (index % ARRAY_SIZE), TestValue(index));
    }
  };

  VISKORES_CONT void TestStore()
  {
    std::cout << "AtomicStore" << std::endl;
    viskores::cont::ArrayHandleBasic<T> array;
    array.Allocate(ARRAY_SIZE);

    this->Invoke(
      StoreFunctor{}, viskores::cont::ArrayHandleIndex(EXTENDED_SIZE), ArrayToRawPointer(array));

    auto portal = array.ReadPortal();
    for (viskores::Id arrayIndex = 0; arrayIndex < ARRAY_SIZE; ++arrayIndex)
    {
      bool foundExpected = false;
      T foundValue = portal.Get(arrayIndex);
      for (viskores::Id overlapIndex = 0; overlapIndex < OVERLAP; ++overlapIndex)
      {
        if (test_equal(foundValue, TestValue(arrayIndex + (overlapIndex * ARRAY_SIZE))))
        {
          foundExpected = true;
          break;
        }
      }
      VISKORES_TEST_ASSERT(
        foundExpected, "Wrong value (", foundValue, ") stored in index ", arrayIndex);
    }
  }

  struct AddFunctor : viskores::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn ignored, ExecObject);
    using ExecutionSignature = void(WorkIndex, _2);

    VISKORES_EXEC void operator()(viskores::Id index, T* data) const
    {
      viskores::AtomicAdd(data + (index % ARRAY_SIZE), 2);
      viskores::AtomicAdd(data + (index % ARRAY_SIZE), -1);
    }
  };

  VISKORES_CONT void TestAdd()
  {
    std::cout << "AtomicAdd" << std::endl;
    viskores::cont::ArrayHandleBasic<T> array;
    viskores::cont::ArrayCopy(viskores::cont::make_ArrayHandleConstant<T>(0, ARRAY_SIZE), array);
    array.Allocate(ARRAY_SIZE);

    this->Invoke(
      AddFunctor{}, viskores::cont::ArrayHandleIndex(EXTENDED_SIZE), ArrayToRawPointer(array));

    auto portal = array.ReadPortal();
    T expectedValue = T(OVERLAP);
    for (viskores::Id arrayIndex = 0; arrayIndex < ARRAY_SIZE; ++arrayIndex)
    {
      T foundValue = portal.Get(arrayIndex);
      VISKORES_TEST_ASSERT(
        test_equal(foundValue, expectedValue), foundValue, " != ", expectedValue);
    }
  }

  struct AndFunctor : viskores::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn ignored, ExecObject);
    using ExecutionSignature = void(WorkIndex, _2);

    VISKORES_EXEC void operator()(viskores::Id index, T* data) const
    {
      viskores::Id arrayIndex = index % ARRAY_SIZE;
      viskores::Id offsetIndex = index / ARRAY_SIZE;
      viskores::AtomicAnd(data + arrayIndex, ~(T(0x1u) << offsetIndex));
    }
  };

  VISKORES_CONT void TestAnd()
  {
    std::cout << "AtomicAnd" << std::endl;
    viskores::cont::ArrayHandleBasic<T> array;
    viskores::cont::ArrayCopy(viskores::cont::make_ArrayHandleConstant<T>(T(-1), ARRAY_SIZE),
                              array);
    array.Allocate(ARRAY_SIZE);

    this->Invoke(
      AndFunctor{}, viskores::cont::ArrayHandleIndex(EXTENDED_SIZE), ArrayToRawPointer(array));

    auto portal = array.ReadPortal();
    for (viskores::Id arrayIndex = 0; arrayIndex < ARRAY_SIZE; ++arrayIndex)
    {
      T foundValue = portal.Get(arrayIndex);
      VISKORES_TEST_ASSERT(test_equal(foundValue, 0), foundValue, " != 0");
    }
  }

  struct OrFunctor : viskores::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn ignored, ExecObject);
    using ExecutionSignature = void(WorkIndex, _2);

    VISKORES_EXEC void operator()(viskores::Id index, T* data) const
    {
      viskores::Id arrayIndex = index % ARRAY_SIZE;
      viskores::Id offsetIndex = index / ARRAY_SIZE;
      viskores::AtomicOr(data + arrayIndex, 0x1u << offsetIndex);
    }
  };

  VISKORES_CONT void TestOr()
  {
    std::cout << "AtomicOr" << std::endl;
    viskores::cont::ArrayHandleBasic<T> array;
    viskores::cont::ArrayCopy(viskores::cont::make_ArrayHandleConstant<T>(0, ARRAY_SIZE), array);
    array.Allocate(ARRAY_SIZE);

    this->Invoke(
      AndFunctor{}, viskores::cont::ArrayHandleIndex(EXTENDED_SIZE), ArrayToRawPointer(array));

    auto portal = array.ReadPortal();
    T expectedValue = T(-1);
    for (viskores::Id arrayIndex = 0; arrayIndex < ARRAY_SIZE; ++arrayIndex)
    {
      T foundValue = portal.Get(arrayIndex);
      VISKORES_TEST_ASSERT(test_equal(foundValue, 0), foundValue, " != ", expectedValue);
    }
  }

  struct XorFunctor : viskores::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn ignored, ExecObject);
    using ExecutionSignature = void(WorkIndex, _2);

    VISKORES_EXEC void operator()(viskores::Id index, T* data) const
    {
      viskores::Id arrayIndex = index % ARRAY_SIZE;
      viskores::Id offsetIndex = index / ARRAY_SIZE;
      viskores::AtomicXor(data + arrayIndex, 0x3u << offsetIndex);
    }
  };

  VISKORES_CONT void TestXor()
  {
    std::cout << "AtomicXor" << std::endl;
    viskores::cont::ArrayHandleBasic<T> array;
    viskores::cont::ArrayCopy(viskores::cont::make_ArrayHandleConstant<T>(0, ARRAY_SIZE), array);
    array.Allocate(ARRAY_SIZE);

    this->Invoke(
      AndFunctor{}, viskores::cont::ArrayHandleIndex(EXTENDED_SIZE), ArrayToRawPointer(array));

    auto portal = array.ReadPortal();
    T expectedValue = T(1);
    for (viskores::Id arrayIndex = 0; arrayIndex < ARRAY_SIZE; ++arrayIndex)
    {
      T foundValue = portal.Get(arrayIndex);
      VISKORES_TEST_ASSERT(test_equal(foundValue, 0), foundValue, " != ", expectedValue);
    }
  }

  struct NotFunctor : viskores::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn ignored, ExecObject);
    using ExecutionSignature = void(WorkIndex, _2);

    VISKORES_EXEC void operator()(viskores::Id index, T* data) const
    {
      viskores::Id arrayIndex = index % ARRAY_SIZE;
      viskores::Id offsetIndex = index / ARRAY_SIZE;
      if (offsetIndex < arrayIndex)
      {
        viskores::AtomicNot(data + arrayIndex);
      }
    }
  };

  VISKORES_CONT void TestNot()
  {
    std::cout << "AtomicNot" << std::endl;
    viskores::cont::ArrayHandleBasic<T> array;
    viskores::cont::ArrayCopy(viskores::cont::make_ArrayHandleConstant<T>(0xA, ARRAY_SIZE), array);
    array.Allocate(ARRAY_SIZE);

    this->Invoke(
      AndFunctor{}, viskores::cont::ArrayHandleIndex(EXTENDED_SIZE), ArrayToRawPointer(array));

    auto portal = array.ReadPortal();
    T expectedValue = T(0xA);
    for (viskores::Id arrayIndex = 0; arrayIndex < ARRAY_SIZE; ++arrayIndex)
    {
      T foundValue = portal.Get(arrayIndex);
      VISKORES_TEST_ASSERT(test_equal(foundValue, 0), foundValue, " != ", expectedValue);
      expectedValue = static_cast<T>(~expectedValue);
    }
  }

  struct CompareExchangeFunctor : viskores::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn ignored, ExecObject);
    using ExecutionSignature = void(WorkIndex, _2);

    VISKORES_EXEC void operator()(viskores::Id index, T* data) const
    {
      viskores::Id arrayIndex = index % ARRAY_SIZE;
      bool success = false;
      for (T overlapIndex = 0; overlapIndex < static_cast<T>(OVERLAP); ++overlapIndex)
      {
        T expectedValue = overlapIndex;
        if (viskores::AtomicCompareExchange(data + arrayIndex, &expectedValue, overlapIndex + 1))
        {
          success = true;
          break;
        }
      }

      if (!success)
      {
        this->RaiseError("No compare succeeded");
      }
    }
  };

  VISKORES_CONT void TestCompareExchange()
  {
    std::cout << "AtomicCompareExchange" << std::endl;
    viskores::cont::ArrayHandleBasic<T> array;
    viskores::cont::ArrayCopy(viskores::cont::make_ArrayHandleConstant<T>(0, ARRAY_SIZE), array);
    array.Allocate(ARRAY_SIZE);

    this->Invoke(
      AddFunctor{}, viskores::cont::ArrayHandleIndex(EXTENDED_SIZE), ArrayToRawPointer(array));

    auto portal = array.ReadPortal();
    T expectedValue = T(OVERLAP);
    for (viskores::Id arrayIndex = 0; arrayIndex < ARRAY_SIZE; ++arrayIndex)
    {
      T foundValue = portal.Get(arrayIndex);
      VISKORES_TEST_ASSERT(
        test_equal(foundValue, expectedValue), foundValue, " != ", expectedValue);
    }
  }

  VISKORES_CONT void TestAll()
  {
    TestLoad();
    TestStore();
    TestAdd();
    TestAnd();
    TestOr();
    TestXor();
    TestNot();
    TestCompareExchange();
  }
};

struct TestFunctor
{
  template <typename T>
  VISKORES_CONT void operator()(T) const
  {
    AtomicTests<T>().TestAll();
  }
};

void Run()
{
  VISKORES_TEST_ASSERT(
    viskores::ListHas<viskores::AtomicTypesSupported, viskores::AtomicTypePreferred>::value);

  viskores::testing::Testing::TryTypes(TestFunctor{}, viskores::AtomicTypesSupported{});
}

} // anonymous namespace

int UnitTestAtomic(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Run, argc, argv);
}
