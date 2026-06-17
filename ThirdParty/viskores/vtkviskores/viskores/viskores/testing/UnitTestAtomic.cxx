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

#include <vector>

namespace
{

constexpr viskores::Id ARRAY_SIZE = 100;

template <typename T>
struct AtomicTests
{
  viskores::cont::Invoker Invoke;

  static constexpr viskores::Id OVERLAP = sizeof(T) * CHAR_BIT;
  static constexpr viskores::Id EXTENDED_SIZE = ARRAY_SIZE * OVERLAP;
  static constexpr viskores::Id RETURN_VALUE_CONTENTION_SIZE = ARRAY_SIZE;
  static_assert((RETURN_VALUE_CONTENTION_SIZE % 2) == 0,
                "The AtomicXor return-value contention test expects an even number of work items.");

  VISKORES_EXEC_CONT static T TestValue(viskores::Id index) { return ::TestValue(index, T{}); }

  VISKORES_CONT static viskores::IdComponent CountSetBits(T value)
  {
    viskores::IdComponent count = 0;
    for (viskores::IdComponent bitIndex = 0; bitIndex < static_cast<viskores::IdComponent>(OVERLAP);
         ++bitIndex)
    {
      if ((value & (T(1) << bitIndex)) != T(0))
      {
        ++count;
      }
    }
    return count;
  }

  VISKORES_CONT void CheckSortedSequentialReturnValues(
    viskores::cont::ArrayHandleBasic<T>& returnValues,
    viskores::Id numberOfValues) const
  {
    viskores::cont::Algorithm::Sort(returnValues);
    auto returnPortal = returnValues.ReadPortal();
    for (viskores::Id index = 0; index < numberOfValues; ++index)
    {
      T foundValue = returnPortal.Get(index);
      VISKORES_TEST_ASSERT(test_equal(foundValue, T(index)), foundValue, " != ", index);
    }
  }

  VISKORES_CONT void CheckPopCountPermutation(
    const viskores::cont::ArrayHandleBasic<T>& returnValues,
    viskores::IdComponent firstExpectedPopCount,
    viskores::IdComponent lastExpectedPopCount) const
  {
    std::vector<bool> foundPopCount(static_cast<std::size_t>(OVERLAP + 1), false);
    auto returnPortal = returnValues.ReadPortal();
    for (viskores::Id index = 0; index < OVERLAP; ++index)
    {
      const viskores::IdComponent popCount = CountSetBits(returnPortal.Get(index));
      VISKORES_TEST_ASSERT(popCount >= firstExpectedPopCount && popCount <= lastExpectedPopCount,
                           popCount,
                           " is outside the expected popcount range.");
      VISKORES_TEST_ASSERT(!foundPopCount[static_cast<std::size_t>(popCount)],
                           "Repeated popcount ",
                           popCount,
                           " in contended atomic return values.");
      foundPopCount[static_cast<std::size_t>(popCount)] = true;
    }
  }

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

  struct ReturnValueFunctor : viskores::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn ignored, ExecObject);
    using ExecutionSignature = void(WorkIndex, _2);

    VISKORES_EXEC void CheckEqual(T foundValue, T expectedValue, const char* message) const
    {
      if (!test_equal(foundValue, expectedValue))
      {
        this->RaiseError(message);
      }
    }

    VISKORES_EXEC void operator()(viskores::Id index, T* data) const
    {
      T* value = data + index;

      // Each element is visited once, so each atomic operation has a deterministic old value.
      // Starting from 0x6, this sequence leaves the stored value at ~0x6.
      this->CheckEqual(viskores::AtomicAdd(value, T(3)), T(0x6), "Bad AtomicAdd return value");
      this->CheckEqual(viskores::AtomicAnd(value, T(0x7)), T(0x9), "Bad AtomicAnd return value");
      this->CheckEqual(viskores::AtomicOr(value, T(0x4)), T(0x1), "Bad AtomicOr return value");
      this->CheckEqual(viskores::AtomicXor(value, T(0x3)), T(0x5), "Bad AtomicXor return value");
      this->CheckEqual(viskores::AtomicNot(value), T(0x6), "Bad AtomicNot return value");
    }
  };

  VISKORES_CONT void TestReturnValues()
  {
    std::cout << "AtomicReturnValues" << std::endl;
    viskores::cont::ArrayHandleBasic<T> array;
    array.AllocateAndFill(ARRAY_SIZE, T(0x6));

    this->Invoke(
      ReturnValueFunctor{}, viskores::cont::ArrayHandleIndex(ARRAY_SIZE), ArrayToRawPointer(array));

    auto portal = array.ReadPortal();
    T expectedValue = static_cast<T>(~T(0x6));
    // Also check the final value so a backend cannot return the right old values while storing
    // the wrong result.
    for (viskores::Id arrayIndex = 0; arrayIndex < ARRAY_SIZE; ++arrayIndex)
    {
      T foundValue = portal.Get(arrayIndex);
      VISKORES_TEST_ASSERT(
        test_equal(foundValue, expectedValue), foundValue, " != ", expectedValue);
    }
  }

  // These worklets intentionally direct every invocation at the same raw pointer. The output
  // field captures the value returned by each atomic operation so the control-side checks can
  // validate the complete set of old values after the nondeterministic execution order settles.
  struct AddReturnValueContentionFunctor : viskores::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn ignored, ExecObject, FieldOut returnValues);
    using ExecutionSignature = void(_2, _3);

    VISKORES_EXEC void operator()(T* data, T& returnValue) const
    {
      returnValue = viskores::AtomicAdd(data, T(1));
    }
  };

  struct AndReturnValueContentionFunctor : viskores::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn ignored, ExecObject, FieldOut returnValues);
    using ExecutionSignature = void(WorkIndex, _2, _3);

    VISKORES_EXEC void operator()(viskores::Id index, T* data, T& returnValue) const
    {
      returnValue = viskores::AtomicAnd(data, static_cast<T>(~(T(1) << index)));
    }
  };

  struct OrReturnValueContentionFunctor : viskores::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn ignored, ExecObject, FieldOut returnValues);
    using ExecutionSignature = void(WorkIndex, _2, _3);

    VISKORES_EXEC void operator()(viskores::Id index, T* data, T& returnValue) const
    {
      returnValue = viskores::AtomicOr(data, T(1) << index);
    }
  };

  struct XorReturnValueContentionFunctor : viskores::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn ignored, ExecObject, FieldOut returnValues);
    using ExecutionSignature = void(_2, _3);

    VISKORES_EXEC void operator()(T* data, T& returnValue) const
    {
      returnValue = viskores::AtomicXor(data, T(1));
    }
  };

  struct NotReturnValueContentionFunctor : viskores::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn ignored, ExecObject, FieldOut returnValues);
    using ExecutionSignature = void(_2, _3);

    VISKORES_EXEC void operator()(T* data, T& returnValue) const
    {
      returnValue = viskores::AtomicNot(data);
    }
  };

  struct CompareExchangeReturnValueContentionFunctor : viskores::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn ignored, ExecObject, FieldOut returnValues);
    using ExecutionSignature = void(_2, _3);

    VISKORES_EXEC void operator()(T* data, T& returnValue) const
    {
      bool success = false;
      while (!success)
      {
        T expectedValue = viskores::AtomicLoad(data);
        success = viskores::AtomicCompareExchange(data, &expectedValue, expectedValue + T(1));
        if (success)
        {
          returnValue = expectedValue;
        }
      }
    }
  };

  VISKORES_CONT void TestContendedReturnValues()
  {
    std::cout << "AtomicReturnValuesUnderContention" << std::endl;

    // AtomicAdd is the strongest return-value contention check. With N work items all adding
    // one to the same location, the final stored value must be N. More importantly, the returned
    // old values must form a permutation of 0..N-1; duplicate or missing values would mean two
    // work items observed the same state or an update was lost.
    viskores::cont::ArrayHandleBasic<T> addValue;
    addValue.AllocateAndFill(1, T(0));
    viskores::cont::ArrayHandleBasic<T> addReturnValues;

    this->Invoke(AddReturnValueContentionFunctor{},
                 viskores::cont::ArrayHandleIndex(RETURN_VALUE_CONTENTION_SIZE),
                 ArrayToRawPointer(addValue),
                 addReturnValues);

    auto addValuePortal = addValue.ReadPortal();
    VISKORES_TEST_ASSERT(test_equal(addValuePortal.Get(0), T(RETURN_VALUE_CONTENTION_SIZE)),
                         addValuePortal.Get(0),
                         " != ",
                         RETURN_VALUE_CONTENTION_SIZE);

    // Device scheduling makes the order of those return values unspecified, so sort before
    // comparing against the deterministic sequence.
    this->CheckSortedSequentialReturnValues(addReturnValues, RETURN_VALUE_CONTENTION_SIZE);

    // AtomicAnd and AtomicOr use one work item per bit. The final value checks that every update
    // happened. The returned old values are order-dependent, so the test checks two invariants:
    // each work item sees the expected state of the bit it owns, and the popcounts form the same
    // serial sequence any correct ordering would produce.
    viskores::cont::ArrayHandleBasic<T> andValue;
    andValue.AllocateAndFill(1, T(-1));
    viskores::cont::ArrayHandleBasic<T> andReturnValues;

    this->Invoke(AndReturnValueContentionFunctor{},
                 viskores::cont::ArrayHandleIndex(OVERLAP),
                 ArrayToRawPointer(andValue),
                 andReturnValues);

    auto andValuePortal = andValue.ReadPortal();
    VISKORES_TEST_ASSERT(test_equal(andValuePortal.Get(0), T(0)), andValuePortal.Get(0), " != 0");

    auto andReturnPortal = andReturnValues.ReadPortal();
    for (viskores::Id index = 0; index < OVERLAP; ++index)
    {
      const T bit = T(1) << index;
      T foundValue = andReturnPortal.Get(index);
      VISKORES_TEST_ASSERT(
        (foundValue & bit) == bit,
        "AtomicAnd return value did not include the bit cleared by this work item.");
    }
    this->CheckPopCountPermutation(andReturnValues, 1, static_cast<viskores::IdComponent>(OVERLAP));

    viskores::cont::ArrayHandleBasic<T> orValue;
    orValue.AllocateAndFill(1, T(0));
    viskores::cont::ArrayHandleBasic<T> orReturnValues;

    this->Invoke(OrReturnValueContentionFunctor{},
                 viskores::cont::ArrayHandleIndex(OVERLAP),
                 ArrayToRawPointer(orValue),
                 orReturnValues);

    auto orValuePortal = orValue.ReadPortal();
    VISKORES_TEST_ASSERT(
      test_equal(orValuePortal.Get(0), T(-1)), orValuePortal.Get(0), " != ", T(-1));

    auto orReturnPortal = orReturnValues.ReadPortal();
    for (viskores::Id index = 0; index < OVERLAP; ++index)
    {
      const T bit = T(1) << index;
      T foundValue = orReturnPortal.Get(index);
      VISKORES_TEST_ASSERT((foundValue & bit) == T(0),
                           "AtomicOr return value already included the bit set by this work item.");
    }
    this->CheckPopCountPermutation(
      orReturnValues, 0, static_cast<viskores::IdComponent>(OVERLAP - 1));

    // AtomicXor gives a complementary bitwise check. Starting from 0, every operation toggles
    // bit 0. With an even number of contending work items, the stored result must end at 0. The
    // old values should alternate logically between 0 and 1, but the physical order is again
    // unspecified.
    viskores::cont::ArrayHandleBasic<T> xorValue;
    xorValue.AllocateAndFill(1, T(0));
    viskores::cont::ArrayHandleBasic<T> xorReturnValues;

    this->Invoke(XorReturnValueContentionFunctor{},
                 viskores::cont::ArrayHandleIndex(RETURN_VALUE_CONTENTION_SIZE),
                 ArrayToRawPointer(xorValue),
                 xorReturnValues);

    auto xorValuePortal = xorValue.ReadPortal();
    VISKORES_TEST_ASSERT(test_equal(xorValuePortal.Get(0), T(0)), xorValuePortal.Get(0), " != 0");

    // Sorting turns the unordered return values into a simple count check: exactly half should
    // be old 0 values and exactly half should be old 1 values.
    viskores::cont::Algorithm::Sort(xorReturnValues);
    auto xorReturnPortal = xorReturnValues.ReadPortal();
    for (viskores::Id index = 0; index < RETURN_VALUE_CONTENTION_SIZE; ++index)
    {
      T expectedValue = (index < (RETURN_VALUE_CONTENTION_SIZE / 2)) ? T(0) : T(1);
      T foundValue = xorReturnPortal.Get(index);
      VISKORES_TEST_ASSERT(
        test_equal(foundValue, expectedValue), foundValue, " != ", expectedValue);
    }

    // AtomicNot is another toggle test, but it flips every bit rather than one bit. With an even
    // number of work items the final value returns to 0, and the return values should contain an
    // equal split of old 0 values and old all-ones values.
    viskores::cont::ArrayHandleBasic<T> notValue;
    notValue.AllocateAndFill(1, T(0));
    viskores::cont::ArrayHandleBasic<T> notReturnValues;

    this->Invoke(NotReturnValueContentionFunctor{},
                 viskores::cont::ArrayHandleIndex(RETURN_VALUE_CONTENTION_SIZE),
                 ArrayToRawPointer(notValue),
                 notReturnValues);

    auto notValuePortal = notValue.ReadPortal();
    VISKORES_TEST_ASSERT(test_equal(notValuePortal.Get(0), T(0)), notValuePortal.Get(0), " != 0");

    viskores::cont::Algorithm::Sort(notReturnValues);
    auto notReturnPortal = notReturnValues.ReadPortal();
    for (viskores::Id index = 0; index < RETURN_VALUE_CONTENTION_SIZE; ++index)
    {
      T expectedValue = (index < (RETURN_VALUE_CONTENTION_SIZE / 2)) ? T(0) : T(-1);
      T foundValue = notReturnPortal.Get(index);
      VISKORES_TEST_ASSERT(
        test_equal(foundValue, expectedValue), foundValue, " != ", expectedValue);
    }

    // AtomicCompareExchange does not return the old value directly, but it writes the old value
    // into the thread-local expected parameter. Each work item repeatedly tries to increment the
    // same shared value and records the successful old value. A correct contended execution must
    // end at N and produce a permutation of 0..N-1.
    viskores::cont::ArrayHandleBasic<T> compareExchangeValue;
    compareExchangeValue.AllocateAndFill(1, T(0));
    viskores::cont::ArrayHandleBasic<T> compareExchangeReturnValues;

    this->Invoke(CompareExchangeReturnValueContentionFunctor{},
                 viskores::cont::ArrayHandleIndex(RETURN_VALUE_CONTENTION_SIZE),
                 ArrayToRawPointer(compareExchangeValue),
                 compareExchangeReturnValues);

    auto compareExchangeValuePortal = compareExchangeValue.ReadPortal();
    VISKORES_TEST_ASSERT(
      test_equal(compareExchangeValuePortal.Get(0), T(RETURN_VALUE_CONTENTION_SIZE)),
      compareExchangeValuePortal.Get(0),
      " != ",
      RETURN_VALUE_CONTENTION_SIZE);

    this->CheckSortedSequentialReturnValues(compareExchangeReturnValues,
                                            RETURN_VALUE_CONTENTION_SIZE);
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
      viskores::AtomicOr(data + arrayIndex, T(0x1u) << offsetIndex);
    }
  };

  VISKORES_CONT void TestOr()
  {
    std::cout << "AtomicOr" << std::endl;
    viskores::cont::ArrayHandleBasic<T> array;
    viskores::cont::ArrayCopy(viskores::cont::make_ArrayHandleConstant<T>(0, ARRAY_SIZE), array);
    array.Allocate(ARRAY_SIZE);

    this->Invoke(
      OrFunctor{}, viskores::cont::ArrayHandleIndex(EXTENDED_SIZE), ArrayToRawPointer(array));

    auto portal = array.ReadPortal();
    T expectedValue = T(-1);
    for (viskores::Id arrayIndex = 0; arrayIndex < ARRAY_SIZE; ++arrayIndex)
    {
      T foundValue = portal.Get(arrayIndex);
      VISKORES_TEST_ASSERT(
        test_equal(foundValue, expectedValue), foundValue, " != ", expectedValue);
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
      viskores::AtomicXor(data + arrayIndex, T(0x3u) << offsetIndex);
    }
  };

  VISKORES_CONT void TestXor()
  {
    std::cout << "AtomicXor" << std::endl;
    viskores::cont::ArrayHandleBasic<T> array;
    viskores::cont::ArrayCopy(viskores::cont::make_ArrayHandleConstant<T>(0, ARRAY_SIZE), array);
    array.Allocate(ARRAY_SIZE);

    this->Invoke(
      XorFunctor{}, viskores::cont::ArrayHandleIndex(EXTENDED_SIZE), ArrayToRawPointer(array));

    auto portal = array.ReadPortal();
    T expectedValue = T(1);
    for (viskores::Id arrayIndex = 0; arrayIndex < ARRAY_SIZE; ++arrayIndex)
    {
      T foundValue = portal.Get(arrayIndex);
      VISKORES_TEST_ASSERT(
        test_equal(foundValue, expectedValue), foundValue, " != ", expectedValue);
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
      NotFunctor{}, viskores::cont::ArrayHandleIndex(EXTENDED_SIZE), ArrayToRawPointer(array));

    auto portal = array.ReadPortal();
    for (viskores::Id arrayIndex = 0; arrayIndex < ARRAY_SIZE; ++arrayIndex)
    {
      viskores::Id numFlips = (arrayIndex < OVERLAP) ? arrayIndex : OVERLAP;
      T expectedValue = ((numFlips % 2) == 0) ? T(0xA) : static_cast<T>(~T(0xA));
      T foundValue = portal.Get(arrayIndex);
      VISKORES_TEST_ASSERT(
        test_equal(foundValue, expectedValue), foundValue, " != ", expectedValue);
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

    this->Invoke(CompareExchangeFunctor{},
                 viskores::cont::ArrayHandleIndex(EXTENDED_SIZE),
                 ArrayToRawPointer(array));

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
    TestReturnValues();
    TestContendedReturnValues();
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
