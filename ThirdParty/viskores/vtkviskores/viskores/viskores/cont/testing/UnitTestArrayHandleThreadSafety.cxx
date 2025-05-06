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

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleExtractComponent.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ArrayHandlePermutation.h>
#include <viskores/cont/Token.h>

#include <viskores/cont/serial/DeviceAdapterSerial.h>

#include <viskores/cont/testing/Testing.h>

#include <array>
#include <chrono>
#include <future>
#include <thread>

namespace
{

constexpr viskores::Id ARRAY_SIZE = 10;
constexpr std::size_t NUM_THREADS = 20;

using ValueType = viskores::FloatDefault;

template <typename Storage>
bool IncrementArray(viskores::cont::ArrayHandle<ValueType, Storage> array)
{
  viskores::cont::Token token;
  auto portal = array.PrepareForInPlace(viskores::cont::DeviceAdapterTagSerial{}, token);
  if (portal.GetNumberOfValues() != ARRAY_SIZE)
  {
    std::cout << "!!!!! Wrong array size: " << portal.GetNumberOfValues() << std::endl;
    return false;
  }

  for (viskores::Id index = 0; index < ARRAY_SIZE; ++index)
  {
    ValueType value = portal.Get(index);
    ValueType base = TestValue(index, ValueType{});
    if ((value < base) || (value >= base + static_cast<ValueType>(NUM_THREADS)))
    {
      std::cout << "!!!!! Unexpected value in array: " << value << std::endl;
      return false;
    }
    portal.Set(index, value + 1);
  }

  return true;
}

template <typename Storage>
bool IncrementArrayOrdered(viskores::cont::ArrayHandle<ValueType, Storage> array,
                           viskores::cont::Token&& token_,
                           std::size_t threadNum)
{
  // Make sure the Token is moved to the proper scope.
  viskores::cont::Token token = std::move(token_);

  // Sleep for a bit to make sure that threads at the end wait for threads before that
  // are sleeping.
  std::this_thread::sleep_for(
    std::chrono::milliseconds(10 * static_cast<long long>(NUM_THREADS - threadNum)));

  auto portal = array.PrepareForInPlace(viskores::cont::DeviceAdapterTagSerial{}, token);
  if (portal.GetNumberOfValues() != ARRAY_SIZE)
  {
    std::cout << "!!!!! Wrong array size: " << portal.GetNumberOfValues() << std::endl;
    return false;
  }

  for (viskores::Id index = 0; index < ARRAY_SIZE; ++index)
  {
    ValueType value = portal.Get(index);
    ValueType base = TestValue(index, ValueType{});
    if (!test_equal(value, base + static_cast<ValueType>(threadNum)))
    {
      std::cout << "!!!!! Unexpected value in array: " << value << std::endl;
      std::cout << "!!!!! ArrayHandle access likely out of order." << std::endl;
      return false;
    }
    portal.Set(index, value + 1);
  }

  return true;
}

template <typename Storage>
bool CheckArray(viskores::cont::ArrayHandle<ValueType, Storage> array)
{
  viskores::cont::Token token;
  auto portal = array.PrepareForInput(viskores::cont::DeviceAdapterTagSerial{}, token);
  if (portal.GetNumberOfValues() != ARRAY_SIZE)
  {
    std::cout << "!!!!! Wrong array size: " << portal.GetNumberOfValues() << std::endl;
    return false;
  }

  for (viskores::Id index = 0; index < ARRAY_SIZE; ++index)
  {
    ValueType value = portal.Get(index);
    ValueType expectedValue = TestValue(index, value) + static_cast<ValueType>(NUM_THREADS);
    if (!test_equal(value, expectedValue))
    {
      std::cout << "!!!!! Unexpected value in array: " << value << std::endl;
      return false;
    }
  }

  return true;
}

template <typename Storage>
bool DecrementArray(viskores::cont::ArrayHandle<ValueType, Storage> array)
{
  viskores::cont::Token token;
  auto portal = array.PrepareForInPlace(viskores::cont::DeviceAdapterTagSerial{}, token);
  if (portal.GetNumberOfValues() != ARRAY_SIZE)
  {
    std::cout << "!!!!! Wrong array size: " << portal.GetNumberOfValues() << std::endl;
    return false;
  }

  for (viskores::Id index = 0; index < ARRAY_SIZE; ++index)
  {
    ValueType value = portal.Get(index);
    ValueType base = TestValue(index, value);
    if ((value <= base) || (value >= base + static_cast<ValueType>(NUM_THREADS) + 1))
    {
      std::cout << "!!!!! Unexpected value in array: " << value << std::endl;
      return false;
    }
    portal.Set(index, value - 1);
  }

  return true;
}

template <typename Storage>
void ThreadsIncrementToArray(viskores::cont::ArrayHandle<ValueType, Storage> array)
{
  viskores::cont::Token token;
  auto portal = array.PrepareForOutput(ARRAY_SIZE, viskores::cont::DeviceAdapterTagSerial{}, token);

  std::cout << "  Starting write threads" << std::endl;
  std::array<decltype(std::async(std::launch::async, IncrementArray<Storage>, array)), NUM_THREADS>
    futures;
  for (std::size_t index = 0; index < NUM_THREADS; ++index)
  {
    futures[index] = std::async(std::launch::async, IncrementArray<Storage>, array);
  }

  std::cout << "  Filling array" << std::endl;
  for (viskores::Id index = 0; index < ARRAY_SIZE; ++index)
  {
    portal.Set(index, TestValue(index, ValueType{}));
  }

  std::cout << "  Releasing portal" << std::endl;
  token.DetachFromAll();

  std::cout << "  Wait for threads to complete" << std::endl;
  for (std::size_t index = 0; index < NUM_THREADS; ++index)
  {
    bool futureResult = futures[index].get();
    VISKORES_TEST_ASSERT(futureResult, "Failure in IncrementArray");
  }
}

template <typename Storage>
void ThreadsCheckArray(viskores::cont::ArrayHandle<ValueType, Storage> array)
{
  std::cout << "  Check array in control environment" << std::endl;
  auto portal = array.ReadPortal();
  VISKORES_TEST_ASSERT(portal.GetNumberOfValues() == ARRAY_SIZE);

  std::cout << "  Starting threads to check" << std::endl;
  std::array<decltype(std::async(std::launch::async, CheckArray<Storage>, array)), NUM_THREADS>
    futures;
  for (std::size_t index = 0; index < NUM_THREADS; ++index)
  {
    futures[index] = std::async(std::launch::async, CheckArray<Storage>, array);
  }

  std::cout << "  Wait for threads to complete" << std::endl;
  for (std::size_t index = 0; index < NUM_THREADS; ++index)
  {
    bool futureResult = futures[index].get();
    VISKORES_TEST_ASSERT(futureResult, "Failure in CheckArray");
  }
}

template <typename Storage>
void ThreadsDecrementArray(viskores::cont::ArrayHandle<ValueType, Storage> array)
{
  std::cout << "  Starting threads to decrement" << std::endl;
  std::array<decltype(std::async(std::launch::async, DecrementArray<Storage>, array)), NUM_THREADS>
    futures;
  for (std::size_t index = 0; index < NUM_THREADS; ++index)
  {
    futures[index] = std::async(std::launch::async, DecrementArray<Storage>, array);
  }

  std::cout << "  Wait for threads to complete" << std::endl;
  for (std::size_t index = 0; index < NUM_THREADS; ++index)
  {
    bool futureResult = futures[index].get();
    VISKORES_TEST_ASSERT(futureResult, "Failure in DecrementArray");
  }

  CheckPortal(array.ReadPortal());
}

template <typename Storage>
void ThreadsIncrementToArrayOrdered(viskores::cont::ArrayHandle<ValueType, Storage> array)
{
  VISKORES_TEST_ASSERT(array.GetNumberOfValues() == ARRAY_SIZE);
  SetPortal(array.WritePortal());

  std::cout << "  Starting ordered write threads" << std::endl;
  std::array<decltype(std::async(std::launch::async, IncrementArray<Storage>, array)), NUM_THREADS>
    futures;
  for (std::size_t index = 0; index < NUM_THREADS; ++index)
  {
    viskores::cont::Token token;
    array.Enqueue(token);
    futures[index] = std::async(
      std::launch::async, IncrementArrayOrdered<Storage>, array, std::move(token), index);
  }

  std::cout << "  Wait for threads to complete" << std::endl;
  for (std::size_t index = 0; index < NUM_THREADS; ++index)
  {
    bool futureResult = futures[index].get();
    VISKORES_TEST_ASSERT(futureResult, "Failure in IncrementArray");
  }
}

template <typename Storage>
void AllocateQueuedArray(viskores::cont::ArrayHandle<ValueType, Storage>& array)
{
  std::cout << "  Check allocating queued array." << std::endl;

  // We have had instances where a PrepareForOutput that resized the array would lock
  // up even when the given Token had the lock because the allocation used a different
  // token. This regression tests makes sure we don't reintroduce that.

  viskores::cont::Token token;

  array.Enqueue(token);

  // If we deadlock in this call, then there is probably an issue with the allocation
  // not waiting for write access correctly.
  auto writePortal =
    array.PrepareForOutput(ARRAY_SIZE * 2, viskores::cont::DeviceAdapterTagSerial{}, token);
  VISKORES_TEST_ASSERT(writePortal.GetNumberOfValues() == ARRAY_SIZE * 2);
  SetPortal(writePortal);

  token.DetachFromAll();
  CheckPortal(array.ReadPortal());
}

template <typename S1, typename S2>
void AllocateQueuedArray(
  viskores::cont::ArrayHandle<ValueType, viskores::cont::StorageTagPermutation<S1, S2>>&)
{
  // Permutation array cannot be resized.
  std::cout << "  Check allocating queued array... skipping" << std::endl;
}

template <typename Storage>
void DoThreadSafetyTest(viskores::cont::ArrayHandle<ValueType, Storage> array)
{
  ThreadsIncrementToArray(array);
  ThreadsCheckArray(array);
  ThreadsDecrementArray(array);
  ThreadsIncrementToArrayOrdered(array);
  AllocateQueuedArray(array);
}

void DoTest()
{
  std::cout << "Basic array handle." << std::endl;
  viskores::cont::ArrayHandle<ValueType> basicArray;
  DoThreadSafetyTest(basicArray);

  std::cout << "Fancy array handle 1." << std::endl;
  viskores::cont::ArrayHandle<ValueType> valueArray;
  valueArray.Allocate(ARRAY_SIZE);
  auto fancyArray1 = viskores::cont::make_ArrayHandlePermutation(
    viskores::cont::ArrayHandleIndex(ARRAY_SIZE), valueArray);
  DoThreadSafetyTest(fancyArray1);

  std::cout << "Fancy array handle 2." << std::endl;
  viskores::cont::ArrayHandle<viskores::Vec<ValueType, 3>> vecArray;
  vecArray.Allocate(ARRAY_SIZE);
  auto fancyArray2 = viskores::cont::make_ArrayHandleExtractComponent(vecArray, 0);
  DoThreadSafetyTest(fancyArray2);
}

} // anonymous namespace

int UnitTestArrayHandleThreadSafety(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(DoTest, argc, argv);
}
