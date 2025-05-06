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
#include <viskores/TypeTraits.h>
#include <viskores/cont/ArrayHandle.h>

#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/ArrayHandleExtractComponent.h>
#include <viskores/cont/testing/Testing.h>

#include <algorithm>
#include <vector>

namespace
{

template <class IteratorType, typename T>
void CheckValues(IteratorType begin, IteratorType end, T offset)
{

  viskores::Id index = 0;
  for (IteratorType iter = begin; iter != end; iter++)
  {
    T expectedValue = TestValue(index, T()) + offset;
    if (!test_equal(*iter, expectedValue))
    {
      std::stringstream message;
      message << "Got unexpected value in array." << std::endl
              << "Expected: " << expectedValue << ", Found: " << *iter << std::endl;
      VISKORES_TEST_FAIL(message.str().c_str());
    }

    index++;
  }
}

template <typename T>
void CheckArray(const viskores::cont::ArrayHandle<T>& handle, T offset = T(0))
{
  CheckPortal(handle.ReadPortal(), offset);
}

// Use to get an arbitrarily different valuetype than T:
template <typename T>
struct OtherType
{
  using Type = viskores::Int32;
};
template <>
struct OtherType<viskores::Int32>
{
  using Type = viskores::UInt8;
};

struct PassThrough : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = _2(_1);

  template <typename T>
  VISKORES_EXEC T operator()(const T& value) const
  {
    return value;
  }
};

struct AssignTestValue : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn indices, FieldOut values);

  template <typename T>
  VISKORES_EXEC void operator()(viskores::Id index, T& valueOut) const
  {
    valueOut = TestValue(index, T());
  }
};

struct InplaceAdd1 : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldInOut);

  template <typename T>
  VISKORES_EXEC void operator()(T& value) const
  {
    value = static_cast<T>(value + T(1));
  }
};

constexpr viskores::Id ARRAY_SIZE = 100;

struct VerifyEmptyArrays
{
  template <typename T>
  VISKORES_CONT void operator()(T) const
  {
    std::cout << "Try operations on empty arrays." << std::endl;
    // After each operation, reinitialize array in case something gets
    // allocated.
    viskores::cont::ArrayHandle<T> arrayHandle = viskores::cont::ArrayHandle<T>();
    VISKORES_TEST_ASSERT(arrayHandle.GetNumberOfValues() == 0,
                         "Uninitialized array does not report zero values.");
    arrayHandle = viskores::cont::ArrayHandle<T>();
    VISKORES_TEST_ASSERT(arrayHandle.ReadPortal().GetNumberOfValues() == 0,
                         "Uninitialized array does not give portal with zero values.");
    viskores::cont::Token token;
    arrayHandle = viskores::cont::ArrayHandle<T>();
    arrayHandle.Allocate(0, viskores::CopyFlag::On);
    arrayHandle = viskores::cont::ArrayHandle<T>();
    arrayHandle.ReleaseResourcesExecution();
    arrayHandle = viskores::cont::ArrayHandle<T>();
    arrayHandle.ReleaseResources();
    arrayHandle = viskores::cont::make_ArrayHandleMove(std::vector<T>());
    arrayHandle.PrepareForInput(viskores::cont::DeviceAdapterTagSerial{}, token);
    arrayHandle = viskores::cont::ArrayHandle<T>();
    arrayHandle.PrepareForInPlace(viskores::cont::DeviceAdapterTagSerial{}, token);
    arrayHandle = viskores::cont::ArrayHandle<T>();
    arrayHandle.PrepareForOutput(ARRAY_SIZE, viskores::cont::DeviceAdapterTagSerial{}, token);
  }
};

struct VerifyUserOwnedMemory
{
  template <typename T>
  VISKORES_CONT void operator()(T) const
  {
    viskores::cont::Invoker invoke;

    std::cout << "Creating array with user-allocated memory." << std::endl;
    std::vector<T> buffer(ARRAY_SIZE);
    for (viskores::Id index = 0; index < ARRAY_SIZE; index++)
    {
      buffer[static_cast<std::size_t>(index)] = TestValue(index, T());
    }

    viskores::cont::ArrayHandle<T> arrayHandle =
      viskores::cont::make_ArrayHandle(buffer, viskores::CopyFlag::Off);

    VISKORES_TEST_ASSERT(arrayHandle.GetNumberOfValues() == ARRAY_SIZE,
                         "ArrayHandle has wrong number of entries.");

    std::cout << "Check array with user provided memory." << std::endl;
    CheckArray(arrayHandle);

    std::cout << "Check out execution array behavior." << std::endl;
    { //as input
      viskores::cont::ArrayHandle<T> result;
      invoke(PassThrough{}, arrayHandle, result);
      CheckArray(result);
    }

    std::cout << "Check out inplace." << std::endl;
    { //as inplace
      invoke(InplaceAdd1{}, arrayHandle);
      CheckArray(arrayHandle, T(1));
    }

    //clear out user array for next test
    std::fill(buffer.begin(), buffer.end(), static_cast<T>(-1));

    std::cout << "Check out output." << std::endl;
    { //as output with same length as user provided. This should work
      //as no new memory needs to be allocated
      invoke(AssignTestValue{}, viskores::cont::ArrayHandleIndex(ARRAY_SIZE), arrayHandle);

      //sync data, which should fill up the user buffer
      arrayHandle.SyncControlArray();

      //check that we got the proper values in the user array
      CheckValues(buffer.begin(), buffer.end(), T{ 0 });
    }

    std::cout << "Check invalid reallocation." << std::endl;
    { //as output with a length larger than the memory provided by the user
      //this should fail
      bool gotException = false;
      try
      {
        //you should not be able to allocate a size larger than the
        //user provided and get the results
        viskores::cont::Token token;
        arrayHandle.PrepareForOutput(
          ARRAY_SIZE * 2, viskores::cont::DeviceAdapterTagSerial{}, token);
        token.DetachFromAll();
        arrayHandle.WritePortal();
      }
      catch (viskores::cont::Error&)
      {
        gotException = true;
      }
      VISKORES_TEST_ASSERT(gotException,
                           "PrepareForOutput should fail when asked to "
                           "re-allocate user provided memory.");
    }
  }
};

struct VerifyUserTransferredMemory
{
  template <typename T>
  VISKORES_CONT void operator()(T) const
  {
    viskores::cont::Invoker invoke;

    T* buffer = new T[ARRAY_SIZE];
    for (viskores::Id index = 0; index < ARRAY_SIZE; index++)
    {
      buffer[static_cast<std::size_t>(index)] = TestValue(index, T());
    }

    auto user_free_function = [](void* ptr) { delete[] static_cast<T*>(ptr); };
    viskores::cont::ArrayHandleBasic<T> arrayHandle(buffer, ARRAY_SIZE, user_free_function);

    VISKORES_TEST_ASSERT(arrayHandle.GetNumberOfValues() == ARRAY_SIZE,
                         "ArrayHandle has wrong number of entries.");
    VISKORES_TEST_ASSERT(arrayHandle.GetNumberOfComponentsFlat() ==
                         viskores::VecFlat<T>::NUM_COMPONENTS);

    std::cout << "Check array with user transferred memory." << std::endl;
    CheckArray(arrayHandle);

    std::cout << "Check out execution array behavior." << std::endl;
    { //as input
      viskores::cont::ArrayHandle<T> result;
      invoke(PassThrough{}, arrayHandle, result);
      CheckArray(result);
    }

    std::cout << "Check out inplace." << std::endl;
    { //as inplace
      invoke(InplaceAdd1{}, arrayHandle);
      CheckArray(arrayHandle, T(1));
    }

    std::cout << "Check out output." << std::endl;
    { //as output with same length as user provided. This should work
      //as no new memory needs to be allocated
      invoke(AssignTestValue{}, viskores::cont::ArrayHandleIndex(ARRAY_SIZE), arrayHandle);

      //sync data, which should fill up the user buffer
      arrayHandle.SyncControlArray();

      //check that we got the proper values in the user array
      CheckValues(buffer, buffer + ARRAY_SIZE, T{ 0 });
    }

    { //as output with a length larger than the memory provided by the user
      //this should fail
      bool gotException = false;
      try
      {
        //you should not be able to allocate a size larger than the
        //user provided and get the results
        viskores::cont::Token token;
        arrayHandle.PrepareForOutput(
          ARRAY_SIZE * 2, viskores::cont::DeviceAdapterTagSerial{}, token);
        token.DetachFromAll();
        arrayHandle.WritePortal();
      }
      catch (viskores::cont::Error&)
      {
        gotException = true;
      }
      VISKORES_TEST_ASSERT(gotException,
                           "PrepareForOutput should fail when asked to "
                           "re-allocate user provided memory.");
    }
  }
};

struct VerifyVectorMovedMemory
{
  template <typename T>
  VISKORES_CONT void operator()(T) const
  {
    viskores::cont::Invoker invoke;

    std::cout << "Creating moved std::vector memory." << std::endl;
    std::vector<T> buffer(ARRAY_SIZE);
    for (viskores::Id index = 0; index < ARRAY_SIZE; index++)
    {
      buffer[static_cast<std::size_t>(index)] = TestValue(index, T());
    }

    viskores::cont::ArrayHandle<T> arrayHandle =
      viskores::cont::make_ArrayHandleMove(std::move(buffer));
    // buffer is now invalid

    VISKORES_TEST_ASSERT(arrayHandle.GetNumberOfValues() == ARRAY_SIZE,
                         "ArrayHandle has wrong number of entries.");

    std::cout << "Check array with moved std::vector memory." << std::endl;
    CheckArray(arrayHandle);

    std::cout << "Check out execution array behavior." << std::endl;
    { //as input
      viskores::cont::ArrayHandle<T> result;
      invoke(PassThrough{}, arrayHandle, result);
      CheckArray(result);
    }

    std::cout << "Check out inplace." << std::endl;
    { //as inplace
      invoke(InplaceAdd1{}, arrayHandle);
      CheckArray(arrayHandle, T(1));
    }

    std::cout << "Check out output." << std::endl;
    { //as output with same length as user provided. This should work
      //as no new memory needs to be allocated
      invoke(AssignTestValue{}, viskores::cont::ArrayHandleIndex(ARRAY_SIZE), arrayHandle);

      //check that we got the proper values in the user array
      CheckArray(arrayHandle);
    }

    { //as a vector moved to the ArrayHandle, reallocation should be possible
      invoke(AssignTestValue{}, viskores::cont::ArrayHandleIndex(ARRAY_SIZE * 2), arrayHandle);
      VISKORES_TEST_ASSERT(arrayHandle.GetNumberOfValues() == ARRAY_SIZE * 2);

      //check that we got the proper values in the user array
      CheckArray(arrayHandle);
    }
  }
};

struct VerifyInitializerList
{
  template <typename T>
  VISKORES_CONT void operator()(T) const
  {
    viskores::cont::Invoker invoke;

    std::cout << "Creating array with initializer list memory." << std::endl;
    viskores::cont::ArrayHandle<T> arrayHandle =
      viskores::cont::make_ArrayHandle({ TestValue(0, T()), TestValue(1, T()), TestValue(2, T()) });

    VISKORES_TEST_ASSERT(arrayHandle.GetNumberOfValues() == 3,
                         "ArrayHandle has wrong number of entries.");

    std::cout << "Check array with initializer list memory." << std::endl;
    CheckArray(arrayHandle);

    std::cout << "Check out execution array behavior." << std::endl;
    { //as input
      viskores::cont::ArrayHandle<T> result;
      invoke(PassThrough{}, arrayHandle, result);
      CheckArray(result);
    }

    std::cout << "Check out inplace." << std::endl;
    { //as inplace
      invoke(InplaceAdd1{}, arrayHandle);
      CheckArray(arrayHandle, T(1));
    }

    std::cout << "Check out output." << std::endl;
    { //as output with same length as user provided. This should work
      //as no new memory needs to be allocated
      invoke(AssignTestValue{}, viskores::cont::ArrayHandleIndex(3), arrayHandle);

      //check that we got the proper values in the user array
      CheckArray(arrayHandle);
    }

    { //as a vector moved to the ArrayHandle, reallocation should be possible
      invoke(AssignTestValue{}, viskores::cont::ArrayHandleIndex(ARRAY_SIZE * 2), arrayHandle);
      VISKORES_TEST_ASSERT(arrayHandle.GetNumberOfValues() == ARRAY_SIZE * 2);

      //check that we got the proper values in the user array
      CheckArray(arrayHandle);
    }
  }
};

struct VerifyVISKORESAllocatedHandle
{
  template <typename T>
  VISKORES_CONT void operator()(T) const
  {
    viskores::cont::Invoker invoke;

    viskores::cont::ArrayHandle<T> arrayHandle;

    VISKORES_TEST_ASSERT(arrayHandle.GetNumberOfValues() == 0,
                         "ArrayHandle has wrong number of entries.");
    invoke(AssignTestValue{}, viskores::cont::ArrayHandleIndex(ARRAY_SIZE * 2), arrayHandle);

    VISKORES_TEST_ASSERT(arrayHandle.GetNumberOfValues() == ARRAY_SIZE * 2,
                         "Array not allocated correctly.");
    CheckArray(arrayHandle);

    std::cout << "Try shrinking the array." << std::endl;
    arrayHandle.Allocate(ARRAY_SIZE, viskores::CopyFlag::On);
    VISKORES_TEST_ASSERT(arrayHandle.GetNumberOfValues() == ARRAY_SIZE,
                         "Array size did not shrink correctly.");
    CheckArray(arrayHandle);

    std::cout << "Try reallocating array." << std::endl;
    arrayHandle.Allocate(ARRAY_SIZE * 2);
    VISKORES_TEST_ASSERT(arrayHandle.GetNumberOfValues() == ARRAY_SIZE * 2,
                         "Array size did not allocate correctly.");
    // No point in checking values. This method can invalidate them.

    std::cout << "Try in place operation." << std::endl;
    // Reset array data.
    invoke(AssignTestValue{}, viskores::cont::ArrayHandleIndex(ARRAY_SIZE * 2), arrayHandle);

    invoke(InplaceAdd1{}, arrayHandle);
    CheckArray(arrayHandle, T(1));

    VISKORES_TEST_ASSERT(arrayHandle == arrayHandle, "Array handle does not equal itself.");
    VISKORES_TEST_ASSERT(arrayHandle != viskores::cont::ArrayHandle<T>(),
                         "Array handle equals different array.");
  }
};

struct VerifyVISKORESTransferredOwnership
{
  template <typename T>
  VISKORES_CONT void operator()(T) const
  {
    viskores::cont::Invoker invoke;

    viskores::cont::internal::TransferredBuffer transferredMemory;

    //Steal memory from a handle that has multiple copies to verify all
    //copies are updated correctly
    {
      viskores::cont::ArrayHandle<T> arrayHandle;
      auto copyOfHandle = arrayHandle;

      VISKORES_TEST_ASSERT(arrayHandle.GetNumberOfValues() == 0,
                           "ArrayHandle has wrong number of entries.");
      invoke(AssignTestValue{}, viskores::cont::ArrayHandleIndex(ARRAY_SIZE * 2), arrayHandle);

      transferredMemory = copyOfHandle.GetBuffers()[0].TakeHostBufferOwnership();

      VISKORES_TEST_ASSERT(copyOfHandle.GetNumberOfValues() == ARRAY_SIZE * 2,
                           "Array not allocated correctly.");
      CheckArray(arrayHandle);

      std::cout << "Try in place operation." << std::endl;
      invoke(InplaceAdd1{}, arrayHandle);
      CheckArray(arrayHandle, T(1));
    }

    transferredMemory.Delete(transferredMemory.Container);
  }
};

struct VerifyEqualityOperators
{
  template <typename T>
  VISKORES_CONT void operator()(T) const
  {
    std::cout << "Verify that shallow copied array handles compare equal:\n";
    {
      viskores::cont::ArrayHandle<T> a1;
      viskores::cont::ArrayHandle<T> a2 = a1; // shallow copy
      viskores::cont::ArrayHandle<T> a3;
      VISKORES_TEST_ASSERT(a1 == a2, "Shallow copied array not equal.");
      VISKORES_TEST_ASSERT(!(a1 != a2), "Shallow copied array not equal.");
      VISKORES_TEST_ASSERT(a1 != a3, "Distinct arrays compared equal.");
      VISKORES_TEST_ASSERT(!(a1 == a3), "Distinct arrays compared equal.");

      // Operations on a1 shouldn't affect equality
      a1.Allocate(200);
      VISKORES_TEST_ASSERT(a1 == a2, "Shallow copied array not equal.");
      VISKORES_TEST_ASSERT(!(a1 != a2), "Shallow copied array not equal.");

      a1.ReadPortal();
      VISKORES_TEST_ASSERT(a1 == a2, "Shallow copied array not equal.");
      VISKORES_TEST_ASSERT(!(a1 != a2), "Shallow copied array not equal.");

      viskores::cont::Token token;
      a1.PrepareForInPlace(viskores::cont::DeviceAdapterTagSerial{}, token);
      VISKORES_TEST_ASSERT(a1 == a2, "Shallow copied array not equal.");
      VISKORES_TEST_ASSERT(!(a1 != a2), "Shallow copied array not equal.");
    }

    std::cout << "Verify that handles with different storage types are not equal.\n";
    {
      viskores::cont::ArrayHandle<T, viskores::cont::StorageTagBasic> a1;
      viskores::cont::ArrayHandle<viskores::Vec<T, 3>, viskores::cont::StorageTagBasic> tmp;
      auto a2 = viskores::cont::make_ArrayHandleExtractComponent(tmp, 1);

      VISKORES_TEST_ASSERT(a1 != a2, "Arrays with different storage type compared equal.");
      VISKORES_TEST_ASSERT(!(a1 == a2), "Arrays with different storage type compared equal.");
    }

    std::cout << "Verify that handles with different value types are not equal.\n";
    {
      viskores::cont::ArrayHandle<T, viskores::cont::StorageTagBasic> a1;
      viskores::cont::ArrayHandle<typename OtherType<T>::Type, viskores::cont::StorageTagBasic> a2;

      VISKORES_TEST_ASSERT(a1 != a2, "Arrays with different value type compared equal.");
      VISKORES_TEST_ASSERT(!(a1 == a2), "Arrays with different value type compared equal.");
    }

    std::cout << "Verify that handles with different storage and value types are not equal.\n";
    {
      viskores::cont::ArrayHandle<T, viskores::cont::StorageTagBasic> a1;
      viskores::cont::ArrayHandle<viskores::Vec<typename OtherType<T>::Type, 3>,
                                  viskores::cont::StorageTagBasic>
        tmp;
      auto a2 = viskores::cont::make_ArrayHandleExtractComponent(tmp, 1);

      VISKORES_TEST_ASSERT(a1 != a2,
                           "Arrays with different storage and value type compared equal.");
      VISKORES_TEST_ASSERT(!(a1 == a2),
                           "Arrays with different storage and value type compared equal.");
    }
  }
};

struct VerifyFill
{
  template <typename T>
  VISKORES_CONT void operator()(T) const
  {
    std::cout << "Initialize values of array." << std::endl;
    const T testValue1 = TestValue(13, T{});
    viskores::cont::ArrayHandle<T> array;
    array.AllocateAndFill(ARRAY_SIZE, testValue1);
    {
      auto portal = array.ReadPortal();
      for (viskores::Id index = 0; index < ARRAY_SIZE; ++index)
      {
        VISKORES_TEST_ASSERT(portal.Get(index) == testValue1);
      }
    }

    std::cout << "Grow array with new values." << std::endl;
    const T testValue2 = TestValue(42, T{});
    array.AllocateAndFill(ARRAY_SIZE * 2, testValue2, viskores::CopyFlag::On);
    {
      auto portal = array.ReadPortal();
      for (viskores::Id index = 0; index < ARRAY_SIZE; ++index)
      {
        VISKORES_TEST_ASSERT(portal.Get(index) == testValue1);
      }
      for (viskores::Id index = ARRAY_SIZE; index < ARRAY_SIZE * 2; ++index)
      {
        VISKORES_TEST_ASSERT(portal.Get(index) == testValue2);
      }
    }
  }
};

VISKORES_CONT void Run()
{
  viskores::testing::Testing::TryTypes(VerifyEmptyArrays{});
  viskores::testing::Testing::TryTypes(VerifyUserOwnedMemory{});
  viskores::testing::Testing::TryTypes(VerifyUserTransferredMemory{});
  viskores::testing::Testing::TryTypes(VerifyVectorMovedMemory{});
  viskores::testing::Testing::TryTypes(VerifyInitializerList{});
  viskores::testing::Testing::TryTypes(VerifyVISKORESAllocatedHandle{});
  viskores::testing::Testing::TryTypes(VerifyVISKORESTransferredOwnership{});
  viskores::testing::Testing::TryTypes(VerifyEqualityOperators{});
  viskores::testing::Testing::TryTypes(VerifyFill{});
}

} // anonymous namespace

int UnitTestArrayHandle(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Run, argc, argv);
}
