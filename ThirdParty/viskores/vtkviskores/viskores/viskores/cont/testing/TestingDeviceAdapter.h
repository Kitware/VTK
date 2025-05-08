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
#ifndef viskores_cont_testing_TestingDeviceAdapter_h
#define viskores_cont_testing_TestingDeviceAdapter_h

#include <viskores/BinaryOperators.h>
#include <viskores/BinaryPredicates.h>
#include <viskores/TypeTraits.h>

#include <viskores/cont/ArrayGetValues.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCast.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ArrayHandlePermutation.h>
#include <viskores/cont/ArrayHandleView.h>
#include <viskores/cont/ArrayHandleZip.h>
#include <viskores/cont/ArrayPortalToIterators.h>
#include <viskores/cont/DeviceAdapterAlgorithm.h>
#include <viskores/cont/ErrorBadAllocation.h>
#include <viskores/cont/ErrorExecution.h>
#include <viskores/cont/RuntimeDeviceInformation.h>
#include <viskores/cont/Timer.h>

#include <viskores/cont/internal/ArrayPortalFromIterators.h>

#include <viskores/cont/testing/Testing.h>

#include <viskores/cont/AtomicArray.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <random>
#include <thread>
#include <utility>
#include <vector>

#include <viskores/internal/Windows.h>

namespace viskores
{
namespace cont
{
namespace testing
{

#define ERROR_MESSAGE "Got an error."
#define ARRAY_SIZE 100
#define OFFSET 10
#define DIM_SIZE 8

/// This class has a single static member, Run, that tests the templated
/// DeviceAdapter for conformance.
///
template <class DeviceAdapterTag>
struct TestingDeviceAdapter
{
private:
  using StorageTag = viskores::cont::StorageTagBasic;

  using IdArrayHandle = viskores::cont::ArrayHandle<viskores::Id, StorageTag>;
  using IdComponentArrayHandle = viskores::cont::ArrayHandle<viskores::IdComponent, StorageTag>;
  using ScalarArrayHandle = viskores::cont::ArrayHandle<viskores::FloatDefault, StorageTag>;
  using FloatCastHandle = viskores::cont::ArrayHandleCast<viskores::FloatDefault, IdArrayHandle>;

  using IdPortalType = typename IdArrayHandle::WritePortalType;
  using IdPortalConstType = typename IdArrayHandle::ReadPortalType;

  using Algorithm = viskores::cont::DeviceAdapterAlgorithm<DeviceAdapterTag>;

public:
  // Cuda kernels have to be public (in Cuda 4.0).


  template <typename PortalType>
  struct GenericClearArrayKernel
  {
    using ValueType = typename PortalType::ValueType;

    VISKORES_CONT
    GenericClearArrayKernel(const PortalType& array,
                            const ValueType& fillValue = static_cast<ValueType>(OFFSET))
      : Array(array)
      , Dims()
      , FillValue(fillValue)
    {
    }

    VISKORES_CONT
    GenericClearArrayKernel(const PortalType& array,
                            const viskores::Id3& dims,
                            const ValueType& fillValue = static_cast<ValueType>(OFFSET))
      : Array(array)
      , Dims(dims)
      , FillValue(fillValue)
    {
    }

    VISKORES_EXEC void operator()(viskores::Id index) const
    {
      this->Array.Set(index, this->FillValue);
    }

    VISKORES_EXEC void operator()(viskores::Id3 index) const
    {
      //convert from id3 to id
      viskores::Id flatIndex = index[0] + this->Dims[0] * (index[1] + this->Dims[1] * index[2]);
      this->operator()(flatIndex);
    }

    VISKORES_CONT void SetErrorMessageBuffer(const viskores::exec::internal::ErrorMessageBuffer&) {}

    PortalType Array;
    viskores::Id3 Dims;
    ValueType FillValue;
  };

  using ClearArrayKernel = GenericClearArrayKernel<IdPortalType>;

  template <typename PortalType>
  struct AddArrayKernel
  {
    VISKORES_CONT
    AddArrayKernel(const PortalType& array)
      : Array(array)
      , Dims()
    {
    }

    VISKORES_CONT
    AddArrayKernel(const PortalType& array, const viskores::Id3& dims)
      : Array(array)
      , Dims(dims)
    {
    }

    VISKORES_EXEC void operator()(viskores::Id index) const
    {
      this->Array.Set(index, this->Array.Get(index) + index);
    }

    VISKORES_EXEC void operator()(viskores::Id3 index) const
    {
      //convert from id3 to id
      viskores::Id flatIndex = index[0] + this->Dims[0] * (index[1] + this->Dims[1] * index[2]);
      this->operator()(flatIndex);
    }

    VISKORES_CONT void SetErrorMessageBuffer(const viskores::exec::internal::ErrorMessageBuffer&) {}

    PortalType Array;
    viskores::Id3 Dims;
  };

  template <typename PortalType>
  static VISKORES_CONT AddArrayKernel<PortalType> MakeAddArrayKernel(
    PortalType portal,
    const viskores::Id3& dims = viskores::Id3{})
  {
    return AddArrayKernel<PortalType>(portal, dims);
  }

  // Checks that each instance is only visited once:
  struct OverlapKernel
  {
    using ArrayType = ArrayHandle<bool>;
    using PortalType = typename ArrayType::WritePortalType;

    PortalType TrackerPortal;
    PortalType ValidPortal;
    viskores::Id3 Dims;

    VISKORES_CONT
    OverlapKernel(const PortalType& trackerPortal,
                  const PortalType& validPortal,
                  const viskores::Id3& dims)
      : TrackerPortal(trackerPortal)
      , ValidPortal(validPortal)
      , Dims(dims)
    {
    }

    VISKORES_CONT
    OverlapKernel(const PortalType& trackerPortal, const PortalType& validPortal)
      : TrackerPortal(trackerPortal)
      , ValidPortal(validPortal)
      , Dims()
    {
    }

    VISKORES_EXEC void operator()(viskores::Id index) const
    {
      if (this->TrackerPortal.Get(index))
      { // this index has already been visited, that's an error
        this->ValidPortal.Set(index, false);
      }
      else
      {
        this->TrackerPortal.Set(index, true);
        this->ValidPortal.Set(index, true);
      }
    }

    VISKORES_EXEC void operator()(viskores::Id3 index) const
    {
      //convert from id3 to id
      viskores::Id flatIndex = index[0] + this->Dims[0] * (index[1] + this->Dims[1] * index[2]);
      this->operator()(flatIndex);
    }

    VISKORES_CONT void SetErrorMessageBuffer(const viskores::exec::internal::ErrorMessageBuffer&) {}
  };

  struct OneErrorKernel
  {
    VISKORES_EXEC void operator()(viskores::Id index) const
    {
      if (index == ARRAY_SIZE / 2)
      {
        this->ErrorMessage.RaiseError(ERROR_MESSAGE);
      }
    }

    VISKORES_CONT void SetErrorMessageBuffer(
      const viskores::exec::internal::ErrorMessageBuffer& errorMessage)
    {
      this->ErrorMessage = errorMessage;
    }

    viskores::exec::internal::ErrorMessageBuffer ErrorMessage;
  };

  struct AllErrorKernel
  {
    VISKORES_EXEC void operator()(viskores::Id viskoresNotUsed(index)) const
    {
      this->ErrorMessage.RaiseError(ERROR_MESSAGE);
    }

    VISKORES_CONT void SetErrorMessageBuffer(
      const viskores::exec::internal::ErrorMessageBuffer& errorMessage)
    {
      this->ErrorMessage = errorMessage;
    }

    viskores::exec::internal::ErrorMessageBuffer ErrorMessage;
  };

  struct OffsetPlusIndexKernel
  {
    VISKORES_CONT
    OffsetPlusIndexKernel(const IdPortalType& array)
      : Array(array)
    {
    }

    VISKORES_EXEC void operator()(viskores::Id index) const
    {
      this->Array.Set(index, OFFSET + index);
    }

    VISKORES_CONT void SetErrorMessageBuffer(const viskores::exec::internal::ErrorMessageBuffer&) {}

    IdPortalType Array;
  };

  struct MarkOddNumbersKernel
  {
    VISKORES_CONT
    MarkOddNumbersKernel(const IdPortalType& array)
      : Array(array)
    {
    }

    VISKORES_EXEC void operator()(viskores::Id index) const { this->Array.Set(index, index % 2); }

    VISKORES_CONT void SetErrorMessageBuffer(const viskores::exec::internal::ErrorMessageBuffer&) {}

    IdPortalType Array;
  };

  struct FuseAll
  {
    template <typename T>
    VISKORES_EXEC bool operator()(const T&, const T&) const
    {
      //binary predicates for unique return true if they are the same
      return true;
    }
  };

  template <typename T>
  struct AtomicKernel
  {
    VISKORES_CONT
    AtomicKernel(const viskores::cont::AtomicArray<T>& array, viskores::cont::Token& token)
      : AArray(array.PrepareForExecution(DeviceAdapterTag(), token))
    {
    }

    VISKORES_EXEC void operator()(viskores::Id index) const
    {
      T value = (T)index;
      this->AArray.Add(0, value);
    }

    VISKORES_CONT void SetErrorMessageBuffer(const viskores::exec::internal::ErrorMessageBuffer&) {}

    viskores::exec::AtomicArrayExecutionObject<T> AArray;
  };

  template <typename T>
  struct AtomicCASKernel
  {
    VISKORES_CONT
    AtomicCASKernel(const viskores::cont::AtomicArray<T>& array, viskores::cont::Token& token)
      : AArray(array.PrepareForExecution(DeviceAdapterTag(), token))
    {
    }

    VISKORES_EXEC void operator()(viskores::Id index) const
    {
      T value = (T)index;
      //Get the old value from the array
      T oldValue = this->AArray.Get(0);
      //Use atomic compare-exchange to atomically add value
      while (!this->AArray.CompareExchange(0, &oldValue, oldValue + value))
        ;
    }

    VISKORES_CONT void SetErrorMessageBuffer(const viskores::exec::internal::ErrorMessageBuffer&) {}

    viskores::exec::AtomicArrayExecutionObject<T> AArray;
  };

  struct CustomPairOp
  {
    using ValueType = viskores::Pair<viskores::Id, viskores::Float32>;

    VISKORES_EXEC
    ValueType operator()(const viskores::Id& a) const { return ValueType(a, 0.0f); }

    VISKORES_EXEC
    ValueType operator()(const viskores::Id& a, const viskores::Id& b) const
    {
      return ValueType(viskores::Max(a, b), 0.0f);
    }

    VISKORES_EXEC
    ValueType operator()(const ValueType& a, const ValueType& b) const
    {
      return ValueType(viskores::Max(a.first, b.first), 0.0f);
    }

    VISKORES_EXEC
    ValueType operator()(const viskores::Id& a, const ValueType& b) const
    {
      return ValueType(viskores::Max(a, b.first), 0.0f);
    }

    VISKORES_EXEC
    ValueType operator()(const ValueType& a, const viskores::Id& b) const
    {
      return ValueType(viskores::Max(a.first, b), 0.0f);
    }
  };

  struct CustomTForReduce
  {
    constexpr CustomTForReduce()
      : Value(0.0f)
    {
    }

    constexpr CustomTForReduce(float f)
      : Value(f)
    {
    }

    VISKORES_EXEC_CONT
    constexpr float value() const { return this->Value; }

    float Value;
  };

  template <typename T>
  struct CustomMinAndMax
  {
    VISKORES_EXEC_CONT
    viskores::Vec<float, 2> operator()(const T& a) const
    {
      return viskores::make_Vec(a.value(), a.value());
    }

    VISKORES_EXEC_CONT
    viskores::Vec<float, 2> operator()(const T& a, const T& b) const
    {
      return viskores::make_Vec(viskores::Min(a.value(), b.value()),
                                viskores::Max(a.value(), b.value()));
    }

    VISKORES_EXEC_CONT
    viskores::Vec<float, 2> operator()(const viskores::Vec<float, 2>& a,
                                       const viskores::Vec<float, 2>& b) const
    {
      return viskores::make_Vec(viskores::Min(a[0], b[0]), viskores::Max(a[1], b[1]));
    }

    VISKORES_EXEC_CONT
    viskores::Vec<float, 2> operator()(const T& a, const viskores::Vec<float, 2>& b) const
    {
      return viskores::make_Vec(viskores::Min(a.value(), b[0]), viskores::Max(a.value(), b[1]));
    }

    VISKORES_EXEC_CONT
    viskores::Vec<float, 2> operator()(const viskores::Vec<float, 2>& a, const T& b) const
    {
      return viskores::make_Vec(viskores::Min(a[0], b.value()), viskores::Max(a[1], b.value()));
    }
  };


private:
  static VISKORES_CONT void TestDeviceAdapterTag()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing device adapter tag" << std::endl;

    constexpr DeviceAdapterTag deviceTag;
    constexpr viskores::cont::DeviceAdapterTagUndefined undefinedTag;

    VISKORES_TEST_ASSERT(deviceTag.GetValue() == deviceTag.GetValue(),
                         "Device adapter Id does not equal itself.");
    VISKORES_TEST_ASSERT(deviceTag.GetValue() != undefinedTag.GetValue(),
                         "Device adapter Id not distinguishable from others.");

    using Traits = viskores::cont::DeviceAdapterTraits<DeviceAdapterTag>;
    VISKORES_TEST_ASSERT(Traits::GetName() == Traits::GetName(),
                         "Device adapter Name does not equal itself.");
  }

  static VISKORES_CONT void TestMemoryTransfer()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Memory Transfer" << std::endl;

    using T = viskores::Id;
    using PortalType = viskores::cont::internal::ArrayPortalFromIterators<T*>;
    auto makePortal = [](const viskores::cont::internal::BufferInfo& buffer)
    {
      return PortalType(static_cast<T*>(buffer.GetPointer()),
                        static_cast<T*>(buffer.GetPointer()) +
                          static_cast<std::size_t>(buffer.GetSize()) / sizeof(T));
    };

    constexpr viskores::BufferSizeType BUFFER_SIZE =
      ARRAY_SIZE * static_cast<viskores::BufferSizeType>(sizeof(T));

    // Set up buffer on host.
    viskores::cont::internal::BufferInfo hostBufferSrc =
      viskores::cont::internal::AllocateOnHost(BUFFER_SIZE);
    VISKORES_TEST_ASSERT(hostBufferSrc.GetSize() == BUFFER_SIZE);
    SetPortal(makePortal(hostBufferSrc));

    viskores::cont::internal::DeviceAdapterMemoryManager<DeviceAdapterTag> memoryManager;

    // Allocate a buffer.
    viskores::cont::internal::BufferInfo allocatedMemory = memoryManager.Allocate(BUFFER_SIZE);
    VISKORES_TEST_ASSERT(allocatedMemory.GetSize() == BUFFER_SIZE);

    // Copy data from host to device.
    allocatedMemory = memoryManager.CopyHostToDevice(hostBufferSrc);

    // Copy data within device.
    viskores::cont::internal::BufferInfo workingMemory =
      memoryManager.CopyDeviceToDevice(allocatedMemory);
    VISKORES_TEST_ASSERT(workingMemory.GetSize() == BUFFER_SIZE);

    // Copy data back to host.
    viskores::cont::internal::BufferInfo hostBufferDest =
      memoryManager.CopyDeviceToHost(workingMemory);
    VISKORES_TEST_ASSERT(hostBufferDest.GetSize() == BUFFER_SIZE);
    CheckPortal(makePortal(hostBufferDest));

    // Shrink a buffer (and preserve memory)
    memoryManager.Reallocate(workingMemory, BUFFER_SIZE / 2);
    hostBufferDest = memoryManager.CopyDeviceToHost(workingMemory);
    VISKORES_TEST_ASSERT(hostBufferDest.GetSize() == BUFFER_SIZE / 2);
    CheckPortal(makePortal(hostBufferDest));

    // Grow a buffer (and preserve memory)
    memoryManager.Reallocate(workingMemory, BUFFER_SIZE * 2);
    hostBufferDest = memoryManager.CopyDeviceToHost(workingMemory);
    VISKORES_TEST_ASSERT(hostBufferDest.GetSize() == BUFFER_SIZE * 2);
    hostBufferDest.Reallocate(BUFFER_SIZE / 2);
    CheckPortal(makePortal(hostBufferDest));

    // Make sure data is actually available on the device.
    // This actually requires running schedule.
    workingMemory = memoryManager.CopyDeviceToDevice(allocatedMemory);
    Algorithm::Schedule(MakeAddArrayKernel(makePortal(workingMemory)), ARRAY_SIZE);

    hostBufferDest = memoryManager.CopyDeviceToHost(workingMemory);

    PortalType portal = makePortal(hostBufferDest);
    VISKORES_TEST_ASSERT(portal.GetNumberOfValues() == ARRAY_SIZE);
    for (viskores::Id index = 0; index < ARRAY_SIZE; ++index)
    {
      T expected = TestValue(index, T()) + T(index);
      T computed = portal.Get(index);
      VISKORES_TEST_ASSERT(test_equal(expected, computed), expected, " != ", computed);
    }
  }

  static VISKORES_CONT void TestOutOfMemory()
  {
// Only test out of memory with 64 bit ids.  If there are 32 bit ids on
// a 64 bit OS (common), it is simply too hard to get a reliable allocation
// that is too much memory.
#ifdef VISKORES_USE_64BIT_IDS
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Out of Memory" << std::endl;
    bool caughtBadAlloc = false;
    try
    {
      std::cout << "Do array allocation that should fail." << std::endl;
      viskores::cont::Token token;
      viskores::cont::ArrayHandle<viskores::Vec4f_32, StorageTagBasic> bigArray;
      const viskores::Id bigSize = 0x7FFFFFFFFFFFFFFELL;
      bigArray.PrepareForOutput(bigSize, DeviceAdapterTag{}, token);
      // It does not seem reasonable to get here.  The previous call should fail.
      VISKORES_TEST_FAIL("A ridiculously sized allocation succeeded.  Either there "
                         "was a failure that was not reported but should have been "
                         "or the width of viskores::Id is not large enough to express all "
                         "array sizes.");
    }
    catch (viskores::cont::ErrorBadAllocation&)
    {
      caughtBadAlloc = true;
    }
    VISKORES_TEST_ASSERT(caughtBadAlloc);
#endif
  }

  VISKORES_CONT
  static void TestTimer()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Timer" << std::endl;
    auto& tracker = viskores::cont::GetRuntimeDeviceTracker();
    if (tracker.CanRunOn(DeviceAdapterTag()))
    {
      viskores::cont::Timer timer{ DeviceAdapterTag() };
      timer.Start();
      Algorithm::Synchronize();

      std::cout << "Timer started. Sleeping..." << std::endl;

      std::this_thread::sleep_for(std::chrono::milliseconds(500));

      std::cout << "Woke up. Check time." << std::endl;

      timer.Stop();
      viskores::Float64 elapsedTime = timer.GetElapsedTime();

      std::cout << "Elapsed time: " << elapsedTime << std::endl;

      VISKORES_TEST_ASSERT(elapsedTime > 0.499, "Timer did not capture full second wait.");
      VISKORES_TEST_ASSERT(elapsedTime < 1.0, "Timer counted too far or system really busy.");
    }
  }

  static VISKORES_CONT void TestAlgorithmSchedule()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing single value Scheduling with viskores::Id" << std::endl;

    {
      // Allocating execution array
      viskores::cont::ArrayHandle<viskores::Id> handle;

      {
        viskores::cont::Token token;
        Algorithm::Schedule(ClearArrayKernel(handle.PrepareForOutput(1, DeviceAdapterTag{}, token)),
                            1);
      }

      {
        viskores::cont::Token token;
        Algorithm::Schedule(MakeAddArrayKernel(handle.PrepareForInPlace(DeviceAdapterTag{}, token)),
                            1);
      }

      auto portal = handle.ReadPortal();
      for (viskores::Id index = 0; index < 1; index++)
      {
        viskores::Id value = portal.Get(index);
        VISKORES_TEST_ASSERT(value == index + OFFSET,
                             "Got bad value for single value scheduled kernel.");
      }
    } //release memory

    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Schedule with viskores::Id" << std::endl;

    {
      viskores::cont::ArrayHandle<viskores::Id> handle;

      {
        viskores::cont::Token token;
        Algorithm::Schedule(
          ClearArrayKernel(handle.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag{}, token)),
          ARRAY_SIZE);
      }

      {
        viskores::cont::Token token;
        Algorithm::Schedule(MakeAddArrayKernel(handle.PrepareForInPlace(DeviceAdapterTag{}, token)),
                            ARRAY_SIZE);
      }

      auto portal = handle.ReadPortal();
      for (viskores::Id index = 0; index < ARRAY_SIZE; index++)
      {
        viskores::Id value = portal.Get(index);
        VISKORES_TEST_ASSERT(value == index + OFFSET, "Got bad value for scheduled kernels.");
      }
    } //release memory

    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Schedule with a vary large Id value" << std::endl;

    {
      // Allocating execution array.
      viskores::cont::ArrayHandle<viskores::Id> handle;

      //size is selected to be larger than the CUDA backend can launch in a
      //single invocation when compiled for SM_2 support
      const viskores::Id size = 8400000;
      {
        viskores::cont::Token token;
        Algorithm::Schedule(
          ClearArrayKernel(handle.PrepareForOutput(size, DeviceAdapterTag{}, token)), size);
      }

      {
        viskores::cont::Token token;
        Algorithm::Schedule(MakeAddArrayKernel(handle.PrepareForInPlace(DeviceAdapterTag{}, token)),
                            size);
      }

      //Rather than testing for correctness every value of a large array,
      // we randomly test a subset of that array.
      std::default_random_engine generator(static_cast<unsigned int>(std::time(nullptr)));
      std::uniform_int_distribution<viskores::Id> distribution(0, size - 1);
      viskores::Id numberOfSamples = size / 100;
      auto portal = handle.ReadPortal();
      for (viskores::Id i = 0; i < numberOfSamples; ++i)
      {
        viskores::Id randomIndex = distribution(generator);
        viskores::Id value = portal.Get(randomIndex);
        VISKORES_TEST_ASSERT(value == randomIndex + OFFSET, "Got bad value for scheduled kernels.");
      }
    } //release memory

    //verify that the schedule call works with id3
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Schedule with viskores::Id3" << std::endl;

    {
      std::cout << "Allocating execution array" << std::endl;
      viskores::cont::ArrayHandle<viskores::Id> handle;
      viskores::Id3 maxRange(DIM_SIZE);

      {
        viskores::cont::Token token;
        Algorithm::Schedule(
          ClearArrayKernel(
            handle.PrepareForOutput(DIM_SIZE * DIM_SIZE * DIM_SIZE, DeviceAdapterTag{}, token),
            maxRange),
          maxRange);
      }

      {
        viskores::cont::Token token;
        Algorithm::Schedule(
          MakeAddArrayKernel(handle.PrepareForInPlace(DeviceAdapterTag{}, token), maxRange),
          maxRange);
      }

      const viskores::Id maxId = DIM_SIZE * DIM_SIZE * DIM_SIZE;
      auto portal = handle.ReadPortal();
      for (viskores::Id index = 0; index < maxId; index++)
      {
        viskores::Id value = portal.Get(index);
        VISKORES_TEST_ASSERT(value == index + OFFSET,
                             "Got bad value for scheduled viskores::Id3 kernels.");
      }
    } //release memory

    // Ensure that each element is only visited once:
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Schedule for overlap" << std::endl;

    {
      using BoolArray = ArrayHandle<bool>;
      using BoolPortal = typename BoolArray::WritePortalType;
      BoolArray tracker;
      BoolArray valid;

      // Initialize tracker with 'false' values
      std::cout << "Allocating and initializing memory" << std::endl;
      {
        viskores::cont::Token token;
        Algorithm::Schedule(
          GenericClearArrayKernel<BoolPortal>(
            tracker.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag(), token), false),
          ARRAY_SIZE);
        Algorithm::Schedule(GenericClearArrayKernel<BoolPortal>(
                              valid.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag(), token), false),
                            ARRAY_SIZE);
      }

      std::cout << "Running Overlap kernel." << std::endl;
      {
        viskores::cont::Token token;
        Algorithm::Schedule(OverlapKernel(tracker.PrepareForInPlace(DeviceAdapterTag(), token),
                                          valid.PrepareForInPlace(DeviceAdapterTag(), token)),
                            ARRAY_SIZE);
      }

      auto vPortal = valid.ReadPortal();
      for (viskores::Id i = 0; i < ARRAY_SIZE; i++)
      {
        bool isValid = vPortal.Get(i);
        VISKORES_TEST_ASSERT(isValid, "Schedule executed some elements more than once.");
      }
    } // release memory

    // Ensure that each element is only visited once:
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Schedule for overlap with viskores::Id3" << std::endl;

    {
      static constexpr viskores::Id numElems{ DIM_SIZE * DIM_SIZE * DIM_SIZE };
      static const viskores::Id3 dims{ DIM_SIZE, DIM_SIZE, DIM_SIZE };

      using BoolArray = ArrayHandle<bool>;
      using BoolPortal = typename BoolArray::WritePortalType;
      BoolArray tracker;
      BoolArray valid;

      // Initialize tracker with 'false' values
      std::cout << "Allocating and initializing memory" << std::endl;
      {
        viskores::cont::Token token;
        Algorithm::Schedule(
          GenericClearArrayKernel<BoolPortal>(
            tracker.PrepareForOutput(numElems, DeviceAdapterTag(), token), dims, false),
          numElems);
        Algorithm::Schedule(
          GenericClearArrayKernel<BoolPortal>(
            valid.PrepareForOutput(numElems, DeviceAdapterTag(), token), dims, false),
          numElems);
      }

      std::cout << "Running Overlap kernel." << std::endl;
      {
        viskores::cont::Token token;
        Algorithm::Schedule(OverlapKernel(tracker.PrepareForInPlace(DeviceAdapterTag(), token),
                                          valid.PrepareForInPlace(DeviceAdapterTag(), token),
                                          dims),
                            dims);
      }

      auto vPortal = valid.ReadPortal();
      for (viskores::Id i = 0; i < numElems; i++)
      {
        bool isValid = vPortal.Get(i);
        VISKORES_TEST_ASSERT(isValid, "Id3 Schedule executed some elements more than once.");
      }
    } // release memory
  }

  static VISKORES_CONT void TestCopyIf()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing CopyIf" << std::endl;

    IdArrayHandle array;
    IdArrayHandle stencil;
    IdArrayHandle result;

    //construct the index array
    {
      viskores::cont::Token token;
      Algorithm::Schedule(
        OffsetPlusIndexKernel(array.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag(), token)),
        ARRAY_SIZE);
      Algorithm::Schedule(
        MarkOddNumbersKernel(stencil.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag(), token)),
        ARRAY_SIZE);
    }

    Algorithm::CopyIf(array, stencil, result);
    VISKORES_TEST_ASSERT(result.GetNumberOfValues() == array.GetNumberOfValues() / 2,
                         "result of CopyIf has an incorrect size");

    auto portal = result.ReadPortal();
    for (viskores::Id index = 0; index < result.GetNumberOfValues(); index++)
    {
      const viskores::Id value = portal.Get(index);
      VISKORES_TEST_ASSERT(value == (OFFSET + (index * 2) + 1),
                           "Incorrect value in CopyIf result.");
    }

    std::cout << "  CopyIf on fancy arrays." << std::endl;
    result.Allocate(0);
    FloatCastHandle arrayCast(array);
    FloatCastHandle resultCast(result);

    Algorithm::CopyIf(arrayCast, stencil, resultCast);
    VISKORES_TEST_ASSERT(result.GetNumberOfValues() == array.GetNumberOfValues() / 2,
                         "result of CopyIf has an incorrect size");

    portal = result.ReadPortal();
    for (viskores::Id index = 0; index < result.GetNumberOfValues(); index++)
    {
      const viskores::Id value = portal.Get(index);
      VISKORES_TEST_ASSERT(value == (OFFSET + (index * 2) + 1),
                           "Incorrect value in CopyIf result.");
    }

    std::cout << "  CopyIf on zero size arrays." << std::endl;
    array.ReleaseResources();
    stencil.ReleaseResources();
    Algorithm::CopyIf(array, stencil, result);
    VISKORES_TEST_ASSERT(result.GetNumberOfValues() == 0, "result of CopyIf has an incorrect size");
  }

  static VISKORES_CONT void TestOrderedUniqueValues()
  {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "Testing Sort, Unique, LowerBounds and UpperBounds" << std::endl;
    std::vector<viskores::Id> testData(ARRAY_SIZE);
    for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
    {
      testData[i] = static_cast<viskores::Id>(OFFSET + (i % 50));
    }

    IdArrayHandle input = viskores::cont::make_ArrayHandle(testData, viskores::CopyFlag::Off);

    //make a deep copy of input and place it into temp
    IdArrayHandle temp;
    Algorithm::Copy(input, temp);

    Algorithm::Sort(temp);
    Algorithm::Unique(temp);

    IdArrayHandle handle;
    IdArrayHandle handle1;

    //verify lower and upper bounds work
    Algorithm::LowerBounds(temp, input, handle);
    Algorithm::UpperBounds(temp, input, handle1);

    // Check to make sure that temp was resized correctly during Unique.
    // (This was a discovered bug at one point.)
    temp.ReadPortal();                // Forces copy back to control.
    temp.ReleaseResourcesExecution(); // Make sure not counting on execution.
    VISKORES_TEST_ASSERT(temp.GetNumberOfValues() == 50,
                         "Unique did not resize array (or size did not copy to control).");

    auto portal = handle.ReadPortal();
    auto portal1 = handle1.ReadPortal();
    for (viskores::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      viskores::Id value = portal.Get(i);
      viskores::Id value1 = portal1.Get(i);
      VISKORES_TEST_ASSERT(value == i % 50, "Got bad value (LowerBounds)");
      VISKORES_TEST_ASSERT(value1 >= i % 50, "Got bad value (UpperBounds)");
    }

    std::cout << "Testing Sort/Unique/LowerBounds/UpperBounds with random values and fancy array"
              << std::endl;
    //now test it works when the id are not incrementing
    const viskores::Id RANDOMDATA_SIZE = 6;
    viskores::Id randomData[RANDOMDATA_SIZE];
    randomData[0] = 500; // 2 (lower), 3 (upper)
    randomData[1] = 955; // 3 (lower), 4 (upper)
    randomData[2] = 955; // 3 (lower), 4 (upper)
    randomData[3] = 120; // 0 (lower), 1 (upper)
    randomData[4] = 320; // 1 (lower), 2 (upper)
    randomData[5] = 955; // 3 (lower), 4 (upper)

    //change the control structure under the handle
    input = viskores::cont::make_ArrayHandle(randomData, RANDOMDATA_SIZE, viskores::CopyFlag::Off);

    FloatCastHandle tempCast(temp);
    Algorithm::Copy(input, tempCast);
    VISKORES_TEST_ASSERT(temp.GetNumberOfValues() == RANDOMDATA_SIZE, "Copy failed");
    Algorithm::Sort(tempCast);
    Algorithm::Unique(tempCast);
    Algorithm::LowerBounds(tempCast, FloatCastHandle(input), handle);
    Algorithm::UpperBounds(tempCast, FloatCastHandle(input), handle1);

    VISKORES_TEST_ASSERT(handle.GetNumberOfValues() == RANDOMDATA_SIZE,
                         "LowerBounds returned incorrect size");

    std::copy(viskores::cont::ArrayPortalToIteratorBegin(handle.ReadPortal()),
              viskores::cont::ArrayPortalToIteratorEnd(handle.ReadPortal()),
              randomData);
    VISKORES_TEST_ASSERT(randomData[0] == 2, "Got bad value - LowerBounds");
    VISKORES_TEST_ASSERT(randomData[1] == 3, "Got bad value - LowerBounds");
    VISKORES_TEST_ASSERT(randomData[2] == 3, "Got bad value - LowerBounds");
    VISKORES_TEST_ASSERT(randomData[3] == 0, "Got bad value - LowerBounds");
    VISKORES_TEST_ASSERT(randomData[4] == 1, "Got bad value - LowerBounds");
    VISKORES_TEST_ASSERT(randomData[5] == 3, "Got bad value - LowerBounds");

    VISKORES_TEST_ASSERT(handle1.GetNumberOfValues() == RANDOMDATA_SIZE,
                         "UppererBounds returned incorrect size");

    std::copy(viskores::cont::ArrayPortalToIteratorBegin(handle1.ReadPortal()),
              viskores::cont::ArrayPortalToIteratorEnd(handle1.ReadPortal()),
              randomData);
    VISKORES_TEST_ASSERT(randomData[0] == 3, "Got bad value - UpperBound");
    VISKORES_TEST_ASSERT(randomData[1] == 4, "Got bad value - UpperBound");
    VISKORES_TEST_ASSERT(randomData[2] == 4, "Got bad value - UpperBound");
    VISKORES_TEST_ASSERT(randomData[3] == 1, "Got bad value - UpperBound");
    VISKORES_TEST_ASSERT(randomData[4] == 2, "Got bad value - UpperBound");
    VISKORES_TEST_ASSERT(randomData[5] == 4, "Got bad value - UpperBound");
  }

  static VISKORES_CONT void TestSort()
  {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "Sort" << std::endl;
    std::vector<viskores::Id> testData(ARRAY_SIZE);
    for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
    {
      testData[i] = static_cast<viskores::Id>(OFFSET + ((ARRAY_SIZE - i) % 50));
    }

    IdArrayHandle unsorted = viskores::cont::make_ArrayHandle(testData, viskores::CopyFlag::Off);
    IdArrayHandle sorted;
    Algorithm::Copy(unsorted, sorted);

    //Validate the standard inplace sort is correct
    Algorithm::Sort(sorted);

    auto portal = sorted.ReadPortal();
    for (viskores::Id i = 0; i < ARRAY_SIZE - 1; ++i)
    {
      viskores::Id sorted1 = portal.Get(i);
      viskores::Id sorted2 = portal.Get(i + 1);
      VISKORES_TEST_ASSERT(sorted1 <= sorted2, "Values not properly sorted.");
    }

    //Try zero sized array
    sorted.Allocate(0);
    Algorithm::Sort(sorted);
  }

  static VISKORES_CONT void TestSortWithComparisonObject()
  {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "Sort with comparison object" << std::endl;
    std::vector<viskores::Id> testData(ARRAY_SIZE);
    for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
    {
      testData[i] = static_cast<viskores::Id>(OFFSET + ((ARRAY_SIZE - i) % 50));
    }

    //sort the users memory in-place
    IdArrayHandle sorted = viskores::cont::make_ArrayHandle(testData, viskores::CopyFlag::Off);
    Algorithm::Sort(sorted);

    //copy the sorted array into our own memory, if use the same user ptr
    //we would also sort the 'sorted' handle
    IdArrayHandle comp_sorted;
    Algorithm::Copy(sorted, comp_sorted);
    Algorithm::Sort(comp_sorted, viskores::SortGreater());

    //Validate that sorted and comp_sorted are sorted in the opposite directions
    auto sorted_portal = sorted.ReadPortal();
    auto comp_sorted_portal = comp_sorted.ReadPortal();
    for (viskores::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      viskores::Id sorted1 = sorted_portal.Get(i);
      viskores::Id sorted2 = comp_sorted_portal.Get(ARRAY_SIZE - (i + 1));
      VISKORES_TEST_ASSERT(sorted1 == sorted2, "Got bad sort values when using SortGreater");
    }

    //validate that sorted and comp_sorted are now equal
    Algorithm::Sort(comp_sorted, viskores::SortLess());
    comp_sorted_portal = comp_sorted.ReadPortal();
    for (viskores::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      viskores::Id sorted1 = sorted_portal.Get(i);
      viskores::Id sorted2 = comp_sorted_portal.Get(i);
      VISKORES_TEST_ASSERT(sorted1 == sorted2, "Got bad sort values when using SortLess");
    }
  }

  static VISKORES_CONT void TestSortWithFancyArrays()
  {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "Sort of a ArrayHandleZip" << std::endl;

    std::vector<viskores::Id> testData(ARRAY_SIZE);
    for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
    {
      testData[i] = static_cast<viskores::Id>(OFFSET + ((ARRAY_SIZE - i) % 50));
    }

    IdArrayHandle unsorted = viskores::cont::make_ArrayHandle(testData, viskores::CopyFlag::Off);
    IdArrayHandle sorted;
    Algorithm::Copy(unsorted, sorted);

    //verify that we can use ArrayHandleZip inplace
    viskores::cont::ArrayHandleZip<IdArrayHandle, IdArrayHandle> zipped(unsorted, sorted);

    //verify we can use sort with zip handle
    Algorithm::Sort(zipped, viskores::SortGreater());
    Algorithm::Sort(zipped);

    auto portal = zipped.ReadPortal();
    for (viskores::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      viskores::Pair<viskores::Id, viskores::Id> kv_sorted = portal.Get(i);
      VISKORES_TEST_ASSERT((OFFSET + (i / (ARRAY_SIZE / 50))) == kv_sorted.first,
                           "ArrayZipHandle improperly sorted");
    }

    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "Sort of a ArrayHandlePermutation" << std::endl;

    //verify that we can use ArrayHandlePermutation inplace
    viskores::cont::ArrayHandleIndex index(ARRAY_SIZE);
    viskores::cont::ArrayHandlePermutation<viskores::cont::ArrayHandleIndex, IdArrayHandle> perm(
      index, sorted);

    //verify we can use a custom operator sort with permutation handle
    Algorithm::Sort(perm, viskores::SortGreater());
    auto perm_portal = perm.ReadPortal();
    for (viskores::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      viskores::Id sorted_value = perm_portal.Get(i);
      VISKORES_TEST_ASSERT((OFFSET + ((ARRAY_SIZE - (i + 1)) / (ARRAY_SIZE / 50))) == sorted_value,
                           "ArrayZipPermutation improperly sorted");
    }

    //verify we can use the default sort with permutation handle
    Algorithm::Sort(perm);
    perm_portal = perm.ReadPortal();
    for (viskores::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      viskores::Id sorted_value = perm_portal.Get(i);
      VISKORES_TEST_ASSERT((OFFSET + (i / (ARRAY_SIZE / 50))) == sorted_value,
                           "ArrayZipPermutation improperly sorted");
    }
  }

  static VISKORES_CONT void TestSortByKey()
  {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "Sort by keys" << std::endl;

    using Vec3 = viskores::Vec<FloatDefault, 3>;
    using Vec3ArrayHandle = viskores::cont::ArrayHandle<viskores::Vec3f, StorageTag>;

    std::vector<viskores::Id> testKeys(ARRAY_SIZE);
    std::vector<Vec3> testValues(testKeys.size());

    for (viskores::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      std::size_t index = static_cast<size_t>(i);
      testKeys[index] = ARRAY_SIZE - i;
      testValues[index] = TestValue(i, Vec3());
    }

    IdArrayHandle keys = viskores::cont::make_ArrayHandle(testKeys, viskores::CopyFlag::Off);
    Vec3ArrayHandle values = viskores::cont::make_ArrayHandle(testValues, viskores::CopyFlag::Off);

    Algorithm::SortByKey(keys, values);

    auto values_portal = values.ReadPortal();
    auto keys_portal = keys.ReadPortal();
    for (viskores::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      //keys should be sorted from 1 to ARRAY_SIZE
      //values should be sorted from (ARRAY_SIZE-1) to 0
      Vec3 sorted_value = values_portal.Get(i);
      viskores::Id sorted_key = keys_portal.Get(i);

      VISKORES_TEST_ASSERT((sorted_key == (i + 1)), "Got bad SortByKeys key");
      VISKORES_TEST_ASSERT(test_equal(sorted_value, TestValue(ARRAY_SIZE - 1 - i, Vec3())),
                           "Got bad SortByKeys value");
    }

    // this will return everything back to what it was before sorting
    Algorithm::SortByKey(keys, values, viskores::SortGreater());
    values_portal = values.ReadPortal();
    keys_portal = keys.ReadPortal();
    for (viskores::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      //keys should be sorted from ARRAY_SIZE to 1
      //values should be sorted from 0 to (ARRAY_SIZE-1)
      Vec3 sorted_value = values_portal.Get(i);
      viskores::Id sorted_key = keys_portal.Get(i);

      VISKORES_TEST_ASSERT((sorted_key == (ARRAY_SIZE - i)), "Got bad SortByKeys key");
      VISKORES_TEST_ASSERT(test_equal(sorted_value, TestValue(i, Vec3())),
                           "Got bad SortByKeys value");
    }

    //this is here to verify we can sort by viskores::Vec
    Algorithm::SortByKey(values, keys);
    values_portal = values.ReadPortal();
    keys_portal = keys.ReadPortal();
    for (viskores::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      //keys should be sorted from ARRAY_SIZE to 1
      //values should be sorted from 0 to (ARRAY_SIZE-1)
      Vec3 sorted_value = values_portal.Get(i);
      viskores::Id sorted_key = keys_portal.Get(i);

      VISKORES_TEST_ASSERT((sorted_key == (ARRAY_SIZE - i)), "Got bad SortByKeys key");
      VISKORES_TEST_ASSERT(test_equal(sorted_value, TestValue(i, Vec3())),
                           "Got bad SortByKeys value");
    }
  }

  static VISKORES_CONT void TestLowerBoundsWithComparisonObject()
  {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "Testing LowerBounds with comparison object" << std::endl;
    std::vector<viskores::Id> testData(ARRAY_SIZE);
    for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
    {
      testData[i] = static_cast<viskores::Id>(OFFSET + (i % 50));
    }
    IdArrayHandle input = viskores::cont::make_ArrayHandle(testData, viskores::CopyFlag::Off);

    //make a deep copy of input and place it into temp
    IdArrayHandle temp;
    Algorithm::Copy(input, temp);

    Algorithm::Sort(temp);
    Algorithm::Unique(temp);

    IdArrayHandle handle;
    //verify lower bounds work
    Algorithm::LowerBounds(temp, input, handle, viskores::SortLess());

    // Check to make sure that temp was resized correctly during Unique.
    // (This was a discovered bug at one point.)
    temp.ReadPortal();                // Forces copy back to control.
    temp.ReleaseResourcesExecution(); // Make sure not counting on execution.
    VISKORES_TEST_ASSERT(temp.GetNumberOfValues() == 50,
                         "Unique did not resize array (or size did not copy to control).");
    auto portal = handle.ReadPortal();
    for (viskores::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      viskores::Id value = portal.Get(i);
      VISKORES_TEST_ASSERT(value == i % 50, "Got bad LowerBounds value with SortLess");
    }
  }

  static VISKORES_CONT void TestUpperBoundsWithComparisonObject()
  {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "Testing UpperBounds with comparison object" << std::endl;
    std::vector<viskores::Id> testData(ARRAY_SIZE);
    for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
    {
      testData[i] = static_cast<viskores::Id>(OFFSET + (i % 50));
    }
    IdArrayHandle input = viskores::cont::make_ArrayHandle(testData, viskores::CopyFlag::Off);

    //make a deep copy of input and place it into temp
    IdArrayHandle temp;
    Algorithm::Copy(input, temp);

    Algorithm::Sort(temp);
    Algorithm::Unique(temp);

    IdArrayHandle handle;
    //verify upper bounds work
    Algorithm::UpperBounds(temp, input, handle, viskores::SortLess());

    // Check to make sure that temp was resized correctly during Unique.
    // (This was a discovered bug at one point.)
    temp.ReadPortal();                // Forces copy back to control.
    temp.ReleaseResourcesExecution(); // Make sure not counting on execution.
    VISKORES_TEST_ASSERT(temp.GetNumberOfValues() == 50,
                         "Unique did not resize array (or size did not copy to control).");

    auto portal = handle.ReadPortal();
    for (viskores::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      viskores::Id value = portal.Get(i);
      VISKORES_TEST_ASSERT(value == (i % 50) + 1, "Got bad UpperBounds value with SortLess");
    }
  }

  static VISKORES_CONT void TestUniqueWithComparisonObject()
  {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "Testing Unique with comparison object" << std::endl;
    IdArrayHandle input;
    input.Allocate(ARRAY_SIZE);
    {
      auto portal = input.WritePortal();
      for (viskores::Id index = 0; index < ARRAY_SIZE; ++index)
      {
        portal.Set(index, OFFSET + (index % 50));
      }
    }

    Algorithm::Sort(input);
    Algorithm::Unique(input, FuseAll());

    // Check to make sure that input was resized correctly during Unique.
    // (This was a discovered bug at one point.)
    input.SyncControlArray();          // Forces copy back to control.
    input.ReleaseResourcesExecution(); // Make sure not counting on execution.
    VISKORES_TEST_ASSERT(input.GetNumberOfValues() == 1,
                         "Unique did not resize array (or size did not copy to control).");

    viskores::Id value = input.ReadPortal().Get(0);
    VISKORES_TEST_ASSERT(value == OFFSET, "Got bad unique value");
  }

  static VISKORES_CONT void TestReduce()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Reduce" << std::endl;

    //construct the index array
    IdArrayHandle array;
    {
      viskores::cont::Token token;
      Algorithm::Schedule(
        ClearArrayKernel(array.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag(), token)),
        ARRAY_SIZE);
    }

    //the output of reduce and scan inclusive should be the same
    std::cout << "  Reduce with initial value of 0." << std::endl;
    viskores::Id reduce_sum = Algorithm::Reduce(array, 0);
    std::cout << "  Reduce with initial value." << std::endl;
    viskores::Id reduce_sum_with_intial_value = Algorithm::Reduce(array, viskores::Id(ARRAY_SIZE));
    std::cout << "  Inclusive scan to check" << std::endl;
    viskores::Id inclusive_sum = Algorithm::ScanInclusive(array, array);
    std::cout << "  Reduce with 1 value." << std::endl;
    array.Allocate(1, viskores::CopyFlag::On);
    viskores::Id reduce_sum_one_value = Algorithm::Reduce(array, 0);
    std::cout << "  Reduce with 0 values." << std::endl;
    array.Allocate(0);
    viskores::Id reduce_sum_no_values = Algorithm::Reduce(array, 0);
    VISKORES_TEST_ASSERT(reduce_sum == OFFSET * ARRAY_SIZE, "Got bad sum from Reduce");
    VISKORES_TEST_ASSERT(reduce_sum_with_intial_value == reduce_sum + ARRAY_SIZE,
                         "Got bad sum from Reduce with initial value");
    VISKORES_TEST_ASSERT(reduce_sum_one_value == OFFSET, "Got bad single sum from Reduce");
    VISKORES_TEST_ASSERT(reduce_sum_no_values == 0, "Got bad empty sum from Reduce");

    VISKORES_TEST_ASSERT(reduce_sum == inclusive_sum,
                         "Got different sums from Reduce and ScanInclusive");
  }

  static VISKORES_CONT void TestReduceWithComparisonObject()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Reduce with comparison object " << std::endl;


    std::cout << "  Reduce viskores::Id array with viskores::MinAndMax to compute range."
              << std::endl;
    //construct the index array. Assign an abnormally large value
    //to the middle of the array, that should be what we see as our sum.
    std::vector<viskores::Id> testData(ARRAY_SIZE);
    const viskores::Id maxValue = ARRAY_SIZE * 2;
    for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
    {
      viskores::Id index = static_cast<viskores::Id>(i);
      testData[i] = index;
    }
    testData[ARRAY_SIZE / 2] = maxValue;

    IdArrayHandle input = viskores::cont::make_ArrayHandle(testData, viskores::CopyFlag::Off);
    viskores::Id2 range =
      Algorithm::Reduce(input, viskores::Id2(0, 0), viskores::MinAndMax<viskores::Id>());

    VISKORES_TEST_ASSERT(maxValue == range[1], "Got bad value from Reduce with comparison object");
    VISKORES_TEST_ASSERT(0 == range[0], "Got bad value from Reduce with comparison object");


    std::cout << "  Reduce viskores::Id array with custom functor that returns viskores::Pair<>."
              << std::endl;
    auto pairInit = viskores::Pair<viskores::Id, viskores::Float32>(0, 0.0f);
    viskores::Pair<viskores::Id, viskores::Float32> pairRange =
      Algorithm::Reduce(input, pairInit, CustomPairOp());

    VISKORES_TEST_ASSERT(maxValue == pairRange.first,
                         "Got bad value from Reduce with pair comparison object");
    VISKORES_TEST_ASSERT(0.0f == pairRange.second,
                         "Got bad value from Reduce with pair comparison object");


    std::cout << "  Reduce bool array with viskores::LogicalAnd to see if all values are true."
              << std::endl;
    //construct an array of bools and verify that they aren't all true
    auto barray =
      viskores::cont::make_ArrayHandle({ true, true, true, true, true, true, false, true, true,
                                         true, true, true, true, true, true, true,  true, true,
                                         true, true, true, true, true, true, true,  true, true,
                                         true, true, true, true, true, true, true,  true, true,
                                         true, true, true, true, true, true, true,  true, true,
                                         true, true, true, true, true, true, true,  true, true,
                                         true, true, true, true, true, true });
    bool all_true = Algorithm::Reduce(barray, true, viskores::LogicalAnd());
    VISKORES_TEST_ASSERT(all_true == false,
                         "reduction with viskores::LogicalAnd should return false");

    std::cout << "  Reduce with custom value type and custom comparison operator." << std::endl;
    //test with a custom value type with the reduction value being a viskores::Vec<float,2>
    auto farray = viskores::cont::make_ArrayHandle<CustomTForReduce>(
      { 13.1f, -2.1f, -1.0f,  13.1f, -2.1f, -1.0f, 413.1f, -2.1f, -1.0f, 13.1f,  -2.1f,   -1.0f,
        13.1f, -2.1f, -1.0f,  13.1f, -2.1f, -1.0f, 13.1f,  -2.1f, -1.0f, 13.1f,  -2.1f,   -1.0f,
        13.1f, -2.1f, -11.0f, 13.1f, -2.1f, -1.0f, 13.1f,  -2.1f, -1.0f, 13.1f,  -2.1f,   -1.0f,
        13.1f, -2.1f, -1.0f,  13.1f, -2.1f, -1.0f, 13.1f,  -2.1f, -1.0f, 13.1f,  -211.1f, -1.0f,
        13.1f, -2.1f, -1.0f,  13.1f, -2.1f, -1.0f, 13.1f,  -2.1f, -1.0f, 113.1f, -2.1f,   -1.0f });
    viskores::Vec2f_32 frange = Algorithm::Reduce(
      farray, viskores::Vec2f_32(0.0f, 0.0f), CustomMinAndMax<CustomTForReduce>());
    VISKORES_TEST_ASSERT(-211.1f == frange[0],
                         "Got bad float value from Reduce with comparison object");
    VISKORES_TEST_ASSERT(413.1f == frange[1],
                         "Got bad float value from Reduce with comparison object");
  }

  static VISKORES_CONT void TestReduceWithFancyArrays()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Reduce with ArrayHandleZip" << std::endl;
    {
      IdArrayHandle keys, values;

      {
        viskores::cont::Token token;
        Algorithm::Schedule(
          ClearArrayKernel(keys.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag(), token)),
          ARRAY_SIZE);

        Algorithm::Schedule(
          ClearArrayKernel(values.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag(), token)),
          ARRAY_SIZE);
      }

      viskores::cont::ArrayHandleZip<IdArrayHandle, IdArrayHandle> zipped(keys, values);

      //the output of reduce and scan inclusive should be the same
      using ResultType = viskores::Pair<viskores::Id, viskores::Id>;
      ResultType reduce_sum_with_intial_value =
        Algorithm::Reduce(viskores::cont::make_ArrayHandleView(zipped, 0, ARRAY_SIZE),
                          ResultType(ARRAY_SIZE, ARRAY_SIZE));

      ResultType expectedResult(OFFSET * ARRAY_SIZE + ARRAY_SIZE, OFFSET * ARRAY_SIZE + ARRAY_SIZE);
      VISKORES_TEST_ASSERT((reduce_sum_with_intial_value == expectedResult),
                           "Got bad sum from Reduce with initial value");
    }

    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Reduce with ArrayHandlePermutation" << std::endl;
    {
      //lastly test with heterogeneous zip values ( vec3, and constant array handle),
      //and a custom reduce binary functor
      using ValueType = viskores::Float32;

      IdArrayHandle indexHandle =
        viskores::cont::make_ArrayHandle<viskores::Id>({ 0, 0, 0, 1, 1, 1, 2, 2, 2, 3,
                                                         3, 3, 4, 4, 4, 5, 5, 5, 1, 4,
                                                         9, 7, 7, 7, 8, 8, 8, 0, 1, 2 });
      viskores::cont::ArrayHandle<ValueType> valueHandle = viskores::cont::make_ArrayHandle(
        { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, -2.0f });

      const ValueType expectedSum = 125;

      viskores::cont::ArrayHandlePermutation<IdArrayHandle, viskores::cont::ArrayHandle<ValueType>>
        perm;
      perm = viskores::cont::make_ArrayHandlePermutation(indexHandle, valueHandle);

      const ValueType sum = Algorithm::Reduce(perm, ValueType(0.0f));

      std::cout << "sum: " << sum << std::endl;
      VISKORES_TEST_ASSERT((sum == expectedSum), "Got bad sum from Reduce with permutation handle");
    }
  }

  static VISKORES_CONT void TestReduceByKey()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Reduce By Key" << std::endl;

    //first test with very basic integer key / values
    {
      const viskores::Id expectedLength = 6;
      viskores::IdComponent expectedKeys[expectedLength] = { 0, 1, 4, 0, 2, -1 };
      viskores::Id expectedValues[expectedLength] = { 10, 2, 0, 3, 10, -42 };

      IdComponentArrayHandle keys = viskores::cont::make_ArrayHandle<viskores::IdComponent>(
        { 0, 0, 0, 1, 1, 4, 0, 2, 2, 2, 2, -1 });
      IdArrayHandle values =
        viskores::cont::make_ArrayHandle<viskores::Id>({ 13, -2, -1, 1, 1, 0, 3, 1, 2, 3, 4, -42 });

      IdComponentArrayHandle keysOut;
      IdArrayHandle valuesOut;
      Algorithm::ReduceByKey(keys, values, keysOut, valuesOut, viskores::Add());

      VISKORES_TEST_ASSERT(keysOut.GetNumberOfValues() == expectedLength,
                           "Got wrong number of output keys");

      VISKORES_TEST_ASSERT(valuesOut.GetNumberOfValues() == expectedLength,
                           "Got wrong number of output values");

      auto keys_portal = keysOut.ReadPortal();
      auto values_portal = valuesOut.ReadPortal();
      for (viskores::Id i = 0; i < expectedLength; ++i)
      {
        const viskores::Id k = keys_portal.Get(i);
        const viskores::Id v = values_portal.Get(i);
        VISKORES_TEST_ASSERT(expectedKeys[i] == k, "Incorrect reduced key");
        VISKORES_TEST_ASSERT(expectedValues[i] == v, "Incorrect reduced value");
      }
    }

    //next test with a single key across the entire set, using vec3 as the
    //value, using a custom reduce binary functor
    {
      IdArrayHandle keys = viskores::cont::make_ArrayHandle<viskores::Id>({ 0, 0, 0 });
      viskores::cont::ArrayHandle<viskores::Vec3f_64, StorageTag> values =
        viskores::cont::make_ArrayHandle({ viskores::Vec3f_64(13.1, 13.3, 13.5),
                                           viskores::Vec3f_64(-2.1, -2.3, -2.5),
                                           viskores::Vec3f_64(-1.0, -1.0, 1.0) });

      const viskores::Id expectedLength = 1;

      viskores::Id expectedKeys[expectedLength] = { 0 };

      viskores::Vec3f_64 expectedValues[expectedLength];
      expectedValues[0] = viskores::make_Vec(27.51, 30.59, -33.75);

      IdArrayHandle keysOut;
      viskores::cont::ArrayHandle<viskores::Vec3f_64, StorageTag> valuesOut;
      Algorithm::ReduceByKey(keys, values, keysOut, valuesOut, viskores::Multiply());

      VISKORES_TEST_ASSERT(keysOut.GetNumberOfValues() == expectedLength,
                           "Got wrong number of output keys");

      VISKORES_TEST_ASSERT(valuesOut.GetNumberOfValues() == expectedLength,
                           "Got wrong number of output values");

      auto keys_portal = keysOut.ReadPortal();
      auto values_portal = valuesOut.ReadPortal();
      for (viskores::Id i = 0; i < expectedLength; ++i)
      {
        const viskores::Id k = keys_portal.Get(i);
        const viskores::Vec3f_64 v = values_portal.Get(i);
        VISKORES_TEST_ASSERT(expectedKeys[i] == k, "Incorrect reduced key");
        VISKORES_TEST_ASSERT(expectedValues[i] == v, "Incorrect reduced vale");
      }
    }
  }

  static VISKORES_CONT void TestReduceByKeyWithFancyArrays()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Reduce By Key with Fancy Arrays" << std::endl;

    IdComponentArrayHandle keys = viskores::cont::make_ArrayHandle<viskores::IdComponent>(
      { 0, 0, 0, 1, 1, 4, 0, 2, 2, 2, 2, -1 });
    IdArrayHandle values =
      viskores::cont::make_ArrayHandle<viskores::Id>({ 13, -2, -1, 1, 1, 0, 3, 1, 2, 3, 4, -42 });
    FloatCastHandle castValues(values);

    const viskores::Id expectedLength = 6;
    viskores::IdComponent expectedKeys[expectedLength] = { 0, 1, 4, 0, 2, -1 };
    viskores::Id expectedValues[expectedLength] = { 10, 2, 0, 3, 10, -42 };

    IdComponentArrayHandle keysOut;
    IdArrayHandle valuesOut;
    FloatCastHandle castValuesOut(valuesOut);
    Algorithm::ReduceByKey(keys, castValues, keysOut, castValuesOut, viskores::Add());

    VISKORES_TEST_ASSERT(keysOut.GetNumberOfValues() == expectedLength,
                         "Got wrong number of output keys");

    VISKORES_TEST_ASSERT(valuesOut.GetNumberOfValues() == expectedLength,
                         "Got wrong number of output values");
    auto keys_portal = keysOut.ReadPortal();
    auto values_portal = valuesOut.ReadPortal();
    for (viskores::Id i = 0; i < expectedLength; ++i)
    {
      const viskores::Id k = keys_portal.Get(i);
      const viskores::Id v = values_portal.Get(i);
      VISKORES_TEST_ASSERT(expectedKeys[i] == k, "Incorrect reduced key");
      VISKORES_TEST_ASSERT(expectedValues[i] == v, "Incorrect reduced value");
    }
  }

  static VISKORES_CONT void TestScanInclusiveByKeyOne()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Scan Inclusive By Key with 1 elements" << std::endl;

    IdArrayHandle keys = viskores::cont::make_ArrayHandle<viskores::Id>({ 0 });
    IdArrayHandle values = viskores::cont::make_ArrayHandle<viskores::Id>({ 5 });

    IdArrayHandle valuesOut;

    Algorithm::ScanInclusiveByKey(keys, values, valuesOut, viskores::Add());

    VISKORES_TEST_ASSERT(valuesOut.GetNumberOfValues() == 1, "Got wrong number of output values");
    const viskores::Id v = valuesOut.ReadPortal().Get(0);
    VISKORES_TEST_ASSERT(5 == v, "Incorrect scanned value");
  }

  static VISKORES_CONT void TestScanInclusiveByKeyTwo()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Scan Exclusive By Key with 2 elements" << std::endl;

    IdArrayHandle keys = viskores::cont::make_ArrayHandle<viskores::Id>({ 0, 1 });
    IdArrayHandle values = viskores::cont::make_ArrayHandle<viskores::Id>({ 1, 1 });

    const viskores::Id expectedLength = 2;
    viskores::Id expectedValues[expectedLength] = { 1, 1 };

    IdArrayHandle valuesOut;

    Algorithm::ScanInclusiveByKey(keys, values, valuesOut, viskores::Add());

    VISKORES_TEST_ASSERT(valuesOut.GetNumberOfValues() == expectedLength,
                         "Got wrong number of output values");
    auto values_portal = valuesOut.ReadPortal();
    for (viskores::Id i = 0; i < expectedLength; i++)
    {
      const viskores::Id v = values_portal.Get(i);
      VISKORES_TEST_ASSERT(expectedValues[static_cast<std::size_t>(i)] == v,
                           "Incorrect scanned value");
    }
  }
  static VISKORES_CONT void TestScanInclusiveByKeyLarge()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Scan Inclusive By Key with " << ARRAY_SIZE << " elements" << std::endl;

    std::vector<viskores::Id> inputKeys(ARRAY_SIZE);

    for (viskores::Id i = 0; i < ARRAY_SIZE; i++)
    {
      if (i % 100 < 98)
        inputKeys[static_cast<std::size_t>(i)] = static_cast<viskores::Id>(i / 100);
      else
        inputKeys[static_cast<std::size_t>(i)] = static_cast<viskores::Id>(i);
    }
    std::vector<viskores::Id> inputValues(ARRAY_SIZE, 1);

    std::vector<viskores::Id> expectedValues(ARRAY_SIZE);
    for (std::size_t i = 0; i < ARRAY_SIZE; i++)
    {
      if (i % 100 < 98)
        expectedValues[i] = static_cast<viskores::Id>(1 + i % 100);
      else
        expectedValues[i] = static_cast<viskores::Id>(1);
    }

    IdArrayHandle keys = viskores::cont::make_ArrayHandle(inputKeys, viskores::CopyFlag::Off);
    IdArrayHandle values = viskores::cont::make_ArrayHandle(inputValues, viskores::CopyFlag::Off);

    IdArrayHandle valuesOut;

    Algorithm::ScanInclusiveByKey(keys, values, valuesOut, viskores::Add());

    VISKORES_TEST_ASSERT(valuesOut.GetNumberOfValues() == ARRAY_SIZE,
                         "Got wrong number of output values");
    auto values_portal = valuesOut.ReadPortal();
    for (auto i = 0; i < ARRAY_SIZE; i++)
    {
      const viskores::Id v = values_portal.Get(i);
      VISKORES_TEST_ASSERT(expectedValues[static_cast<std::size_t>(i)] == v,
                           "Incorrect scanned value");
    }
  }
  static VISKORES_CONT void TestScanInclusiveByKey()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Scan Inclusive By Key" << std::endl;

    IdComponentArrayHandle keys =
      viskores::cont::make_ArrayHandle<viskores::IdComponent>({ 0, 0, 0, 1, 1, 2, 3, 3, 3, 3 });
    IdArrayHandle values =
      viskores::cont::make_ArrayHandle<viskores::Id>({ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 });

    const viskores::Id expectedLength = 10;
    viskores::Id expectedValues[expectedLength] = { 1, 2, 3, 1, 2, 1, 1, 2, 3, 4 };

    IdArrayHandle valuesOut;

    Algorithm::ScanInclusiveByKey(keys, values, valuesOut);
    VISKORES_TEST_ASSERT(valuesOut.GetNumberOfValues() == expectedLength,
                         "Got wrong number of output values");
    auto valuesPortal = valuesOut.ReadPortal();
    for (auto i = 0; i < expectedLength; i++)
    {
      const viskores::Id v = valuesPortal.Get(i);
      VISKORES_TEST_ASSERT(expectedValues[static_cast<std::size_t>(i)] == v,
                           "Incorrect scanned value");
    }
  }
  static VISKORES_CONT void TestScanInclusiveByKeyInPlace()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Scan Inclusive By Key In Place" << std::endl;


    IdComponentArrayHandle keys =
      viskores::cont::make_ArrayHandle<viskores::IdComponent>({ 0, 0, 0, 1, 1, 2, 3, 3, 3, 3 });
    IdArrayHandle values =
      viskores::cont::make_ArrayHandle<viskores::Id>({ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 });

    const viskores::Id expectedLength = 10;
    viskores::Id expectedValues[expectedLength] = { 1, 2, 3, 1, 2, 1, 1, 2, 3, 4 };

    Algorithm::ScanInclusiveByKey(keys, values, values);
    VISKORES_TEST_ASSERT(values.GetNumberOfValues() == expectedLength,
                         "Got wrong number of output values");
    auto valuesPortal = values.ReadPortal();
    for (auto i = 0; i < expectedLength; i++)
    {
      const viskores::Id v = valuesPortal.Get(i);
      VISKORES_TEST_ASSERT(expectedValues[static_cast<std::size_t>(i)] == v,
                           "Incorrect scanned value");
    }
  }
  static VISKORES_CONT void TestScanInclusiveByKeyInPlaceWithFancyArray()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Scan Inclusive By Key In Place with a Fancy Array" << std::endl;


    IdComponentArrayHandle keys =
      viskores::cont::make_ArrayHandle<viskores::IdComponent>({ 0, 0, 0, 1, 1, 2, 3, 3, 3, 3 });
    IdArrayHandle values =
      viskores::cont::make_ArrayHandle<viskores::Id>({ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 });
    FloatCastHandle castValues(values);

    const viskores::Id expectedLength = 10;
    viskores::Id expectedValues[expectedLength] = { 1, 2, 3, 1, 2, 1, 1, 2, 3, 4 };

    Algorithm::ScanInclusiveByKey(keys, castValues, castValues);
    VISKORES_TEST_ASSERT(values.GetNumberOfValues() == expectedLength,
                         "Got wrong number of output values");
    auto valuesPortal = values.ReadPortal();
    for (auto i = 0; i < expectedLength; i++)
    {
      const viskores::Id v = valuesPortal.Get(i);
      VISKORES_TEST_ASSERT(expectedValues[static_cast<std::size_t>(i)] == v,
                           "Incorrect scanned value");
    }
  }

  static VISKORES_CONT void TestScanExclusiveByKeyOne()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Scan Exclusive By Key with 1 elements" << std::endl;

    viskores::Id init = 5;

    const viskores::Id expectedLength = 1;

    IdArrayHandle keys = viskores::cont::make_ArrayHandle<viskores::Id>({ 0 });
    IdArrayHandle values = viskores::cont::make_ArrayHandle<viskores::Id>({ 0 });

    IdArrayHandle valuesOut;

    Algorithm::ScanExclusiveByKey(keys, values, valuesOut, init, viskores::Add());

    VISKORES_TEST_ASSERT(valuesOut.GetNumberOfValues() == expectedLength,
                         "Got wrong number of output values");
    const viskores::Id v = valuesOut.ReadPortal().Get(0);
    VISKORES_TEST_ASSERT(init == v, "Incorrect scanned value");
  }

  static VISKORES_CONT void TestScanExclusiveByKeyTwo()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Scan Exclusive By Key with 2 elements" << std::endl;

    viskores::Id init = 5;

    IdArrayHandle keys = viskores::cont::make_ArrayHandle<viskores::Id>({ 0, 1 });
    IdArrayHandle values = viskores::cont::make_ArrayHandle<viskores::Id>({ 1, 1 });

    const viskores::Id expectedLength = 2;
    viskores::Id expectedValues[expectedLength] = { 5, 5 };

    IdArrayHandle valuesOut;

    Algorithm::ScanExclusiveByKey(keys, values, valuesOut, init, viskores::Add());

    VISKORES_TEST_ASSERT(valuesOut.GetNumberOfValues() == expectedLength,
                         "Got wrong number of output values");
    auto valuesPortal = valuesOut.ReadPortal();
    for (auto i = 0; i < expectedLength; i++)
    {
      const viskores::Id v = valuesPortal.Get(i);
      VISKORES_TEST_ASSERT(expectedValues[i] == v, "Incorrect scanned value");
    }
  }

  static VISKORES_CONT void TestScanExclusiveByKeyLarge()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Scan Exclusive By Key with " << ARRAY_SIZE << " elements" << std::endl;

    std::vector<viskores::Id> inputKeys(ARRAY_SIZE);
    for (std::size_t i = 0; i < ARRAY_SIZE; i++)
    {
      if (i % 100 < 98)
        inputKeys[i] = static_cast<viskores::Id>(i / 100);
      else
        inputKeys[i] = static_cast<viskores::Id>(i);
    }
    std::vector<viskores::Id> inputValues(ARRAY_SIZE, 1);
    viskores::Id init = 5;

    std::vector<viskores::Id> expectedValues(ARRAY_SIZE);
    for (viskores::Id i = 0; i < ARRAY_SIZE; i++)
    {
      if (i % 100 < 98)
        expectedValues[static_cast<std::size_t>(i)] = static_cast<viskores::Id>(init + i % 100);
      else
        expectedValues[static_cast<std::size_t>(i)] = init;
    }

    IdArrayHandle keys = viskores::cont::make_ArrayHandle(inputKeys, viskores::CopyFlag::Off);
    IdArrayHandle values = viskores::cont::make_ArrayHandle(inputValues, viskores::CopyFlag::Off);

    IdArrayHandle valuesOut;

    Algorithm::ScanExclusiveByKey(keys, values, valuesOut, init, viskores::Add());

    VISKORES_TEST_ASSERT(valuesOut.GetNumberOfValues() == ARRAY_SIZE,
                         "Got wrong number of output values");
    auto valuesPortal = valuesOut.ReadPortal();
    for (viskores::Id i = 0; i < ARRAY_SIZE; i++)
    {
      const viskores::Id v = valuesPortal.Get(i);
      VISKORES_TEST_ASSERT(expectedValues[static_cast<std::size_t>(i)] == v,
                           "Incorrect scanned value");
    }
  }

  static VISKORES_CONT void TestScanExclusiveByKey()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Scan Exclusive By Key" << std::endl;

    viskores::Id init = 5;

    IdComponentArrayHandle keys =
      viskores::cont::make_ArrayHandle<viskores::IdComponent>({ 0, 0, 0, 1, 1, 2, 3, 3, 3, 3 });
    IdArrayHandle values =
      viskores::cont::make_ArrayHandle<viskores::Id>({ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 });

    const viskores::Id expectedLength = 10;
    viskores::Id expectedValues[expectedLength] = { 5, 6, 7, 5, 6, 5, 5, 6, 7, 8 };

    IdArrayHandle valuesOut;

    Algorithm::ScanExclusiveByKey(keys, values, valuesOut, init, viskores::Add());

    VISKORES_TEST_ASSERT(valuesOut.GetNumberOfValues() == expectedLength,
                         "Got wrong number of output values");
    auto valuesPortal = valuesOut.ReadPortal();
    for (viskores::Id i = 0; i < expectedLength; i++)
    {
      const viskores::Id v = valuesPortal.Get(i);
      VISKORES_TEST_ASSERT(expectedValues[static_cast<std::size_t>(i)] == v,
                           "Incorrect scanned value");
    }
  }
  static VISKORES_CONT void TestScanExclusiveByKeyInPlace()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Scan Inclusive By Key In Place" << std::endl;


    viskores::Id init = 5;

    IdComponentArrayHandle keys =
      viskores::cont::make_ArrayHandle<viskores::IdComponent>({ 0, 0, 0, 1, 1, 2, 3, 3, 3, 3 });
    IdArrayHandle values =
      viskores::cont::make_ArrayHandle<viskores::Id>({ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 });

    const viskores::Id expectedLength = 10;
    viskores::Id expectedValues[expectedLength] = { 5, 6, 7, 5, 6, 5, 5, 6, 7, 8 };

    Algorithm::ScanExclusiveByKey(keys, values, values, init, viskores::Add());
    VISKORES_TEST_ASSERT(values.GetNumberOfValues() == expectedLength,
                         "Got wrong number of output values");
    auto valuesPortal = values.ReadPortal();
    for (auto i = 0; i < expectedLength; i++)
    {
      const viskores::Id v = valuesPortal.Get(i);
      VISKORES_TEST_ASSERT(expectedValues[static_cast<std::size_t>(i)] == v,
                           "Incorrect scanned value");
    }
  }
  static VISKORES_CONT void TestScanExclusiveByKeyInPlaceWithFancyArray()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Scan Inclusive By Key In Place with a Fancy Array" << std::endl;


    viskores::FloatDefault init = 5;

    IdComponentArrayHandle keys =
      viskores::cont::make_ArrayHandle<viskores::IdComponent>({ 0, 0, 0, 1, 1, 2, 3, 3, 3, 3 });
    IdArrayHandle values =
      viskores::cont::make_ArrayHandle<viskores::Id>({ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 });
    FloatCastHandle castValues(values);

    const viskores::Id expectedLength = 10;
    viskores::Id expectedValues[expectedLength] = { 5, 6, 7, 5, 6, 5, 5, 6, 7, 8 };

    Algorithm::ScanExclusiveByKey(keys, castValues, castValues, init, viskores::Add());
    VISKORES_TEST_ASSERT(values.GetNumberOfValues() == expectedLength,
                         "Got wrong number of output values");
    auto valuesPortal = values.ReadPortal();
    for (auto i = 0; i < expectedLength; i++)
    {
      const viskores::Id v = valuesPortal.Get(i);
      VISKORES_TEST_ASSERT(expectedValues[static_cast<std::size_t>(i)] == v,
                           "Incorrect scanned value");
    }
  }

  static VISKORES_CONT void TestScanInclusive()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Inclusive Scan" << std::endl;

    {
      std::cout << "  size " << ARRAY_SIZE << std::endl;
      //construct the index array
      IdArrayHandle array;
      {
        viskores::cont::Token token;
        Algorithm::Schedule(
          ClearArrayKernel(array.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag(), token)),
          ARRAY_SIZE);
      }

      //we know have an array whose sum is equal to OFFSET * ARRAY_SIZE,
      //let's validate that
      viskores::Id sum = Algorithm::ScanInclusive(array, array);
      VISKORES_TEST_ASSERT(sum == OFFSET * ARRAY_SIZE, "Got bad sum from Inclusive Scan");

      auto portal = array.ReadPortal();
      for (viskores::Id i = 0; i < ARRAY_SIZE; ++i)
      {
        const viskores::Id value = portal.Get(i);
        VISKORES_TEST_ASSERT(value == (i + 1) * OFFSET, "Incorrect partial sum");
      }

      std::cout << "  size 1" << std::endl;
      array.Allocate(1, viskores::CopyFlag::On);
      sum = Algorithm::ScanInclusive(array, array);
      VISKORES_TEST_ASSERT(sum == OFFSET, "Incorrect partial sum");
      const viskores::Id value = array.ReadPortal().Get(0);
      VISKORES_TEST_ASSERT(value == OFFSET, "Incorrect partial sum");

      std::cout << "  size 0" << std::endl;
      array.Allocate(0);
      sum = Algorithm::ScanInclusive(array, array);
      VISKORES_TEST_ASSERT(sum == 0, "Incorrect partial sum");
    }

    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Inclusive Scan with multiplication operator" << std::endl;
    {
      std::vector<viskores::Float64> inputValues(ARRAY_SIZE);
      for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
      {
        inputValues[i] = 1.01;
      }

      std::size_t mid = ARRAY_SIZE / 2;
      inputValues[mid] = 0.0;

      viskores::cont::ArrayHandle<viskores::Float64> array =
        viskores::cont::make_ArrayHandle(inputValues, viskores::CopyFlag::Off);

      viskores::Float64 product = Algorithm::ScanInclusive(array, array, viskores::Multiply());

      VISKORES_TEST_ASSERT(product == 0.0f, "ScanInclusive product result not 0.0");
      auto portal = array.ReadPortal();
      for (std::size_t i = 0; i < mid; ++i)
      {
        viskores::Id index = static_cast<viskores::Id>(i);
        viskores::Float64 expected = pow(1.01, static_cast<viskores::Float64>(i + 1));
        viskores::Float64 got = portal.Get(index);
        VISKORES_TEST_ASSERT(test_equal(got, expected), "Incorrect results for ScanInclusive");
      }
      for (std::size_t i = mid; i < ARRAY_SIZE; ++i)
      {
        viskores::Id index = static_cast<viskores::Id>(i);
        VISKORES_TEST_ASSERT(portal.Get(index) == 0.0f, "Incorrect results for ScanInclusive");
      }
    }

    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Inclusive Scan with a viskores::Vec" << std::endl;

    {
      using Vec3 = viskores::Vec<Float64, 3>;
      using Vec3ArrayHandle = viskores::cont::ArrayHandle<viskores::Vec3f_64, StorageTag>;

      std::vector<Vec3> testValues(ARRAY_SIZE);

      for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
      {
        testValues[i] = TestValue(1, Vec3());
      }
      Vec3ArrayHandle values =
        viskores::cont::make_ArrayHandle(testValues, viskores::CopyFlag::Off);

      Vec3 sum = Algorithm::ScanInclusive(values, values);
      std::cout << "Sum that was returned " << sum << std::endl;
      VISKORES_TEST_ASSERT(test_equal(sum, TestValue(1, Vec3()) * ARRAY_SIZE),
                           "Got bad sum from Inclusive Scan");
    }
  }

  static VISKORES_CONT void TestScanInclusiveWithComparisonObject()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Inclusive Scan with comparison object " << std::endl;

    //construct the index array
    IdArrayHandle array;
    {
      viskores::cont::Token token;
      Algorithm::Schedule(
        ClearArrayKernel(array.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag(), token)),
        ARRAY_SIZE);
      Algorithm::Schedule(
        MakeAddArrayKernel(array.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag(), token)),
        ARRAY_SIZE);
    }

    //we know have an array whose sum is equal to OFFSET * ARRAY_SIZE,
    //let's validate that
    IdArrayHandle result;
    viskores::Id sum = Algorithm::ScanInclusive(array, result, viskores::Maximum());
    VISKORES_TEST_ASSERT(sum == OFFSET + (ARRAY_SIZE - 1),
                         "Got bad sum from Inclusive Scan with comparison object");

    auto array_portal = array.ReadPortal();
    auto result_portal = result.ReadPortal();
    for (viskores::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      const viskores::Id input_value = array_portal.Get(i);
      const viskores::Id result_value = result_portal.Get(i);
      VISKORES_TEST_ASSERT(input_value == result_value, "Incorrect partial sum");
    }

    //now try it inline
    sum = Algorithm::ScanInclusive(array, array, viskores::Maximum());
    VISKORES_TEST_ASSERT(sum == OFFSET + (ARRAY_SIZE - 1),
                         "Got bad sum from Inclusive Scan with comparison object");
    array_portal = array.ReadPortal();
    for (viskores::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      const viskores::Id input_value = array_portal.Get(i);
      const viskores::Id result_value = result_portal.Get(i);
      VISKORES_TEST_ASSERT(input_value == result_value, "Incorrect partial sum");
    }
  }

  static VISKORES_CONT void TestScanExclusive()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Exclusive Scan" << std::endl;

    {
      //construct the index array
      IdArrayHandle array;
      {
        viskores::cont::Token token;
        Algorithm::Schedule(
          ClearArrayKernel(array.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag(), token)),
          ARRAY_SIZE);
      }

      // we know have an array whose sum = (OFFSET * ARRAY_SIZE),
      // let's validate that
      viskores::Id sum = Algorithm::ScanExclusive(array, array);
      VISKORES_TEST_ASSERT(sum == (OFFSET * ARRAY_SIZE), "Got bad sum from Exclusive Scan");

      auto portal = array.ReadPortal();
      for (viskores::Id i = 0; i < ARRAY_SIZE; ++i)
      {
        const viskores::Id value = portal.Get(i);
        VISKORES_TEST_ASSERT(value == i * OFFSET, "Incorrect partial sum");
      }

      std::cout << "  size 1" << std::endl;
      array.Allocate(1, viskores::CopyFlag::On);
      array.WritePortal().Set(0, OFFSET);
      sum = Algorithm::ScanExclusive(array, array);
      VISKORES_TEST_ASSERT(sum == OFFSET, "Incorrect partial sum");
      const viskores::Id value = array.ReadPortal().Get(0);
      VISKORES_TEST_ASSERT(value == 0, "Incorrect partial sum");

      array.Allocate(0);
      sum = Algorithm::ScanExclusive(array, array);
      VISKORES_TEST_ASSERT(sum == 0, "Incorrect partial sum");
    }

    // Enable when Exclusive Scan with custom operator is implemented for all
    // device adaptors
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Exclusive Scan with multiplication operator" << std::endl;
    {
      std::vector<viskores::Float64> inputValues(ARRAY_SIZE);
      for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
      {
        inputValues[i] = 1.01;
      }

      std::size_t mid = ARRAY_SIZE / 2;
      inputValues[mid] = 0.0;

      viskores::cont::ArrayHandle<viskores::Float64> array =
        viskores::cont::make_ArrayHandle(inputValues, viskores::CopyFlag::Off);

      viskores::Float64 initialValue = 2.00;
      viskores::Float64 product =
        Algorithm::ScanExclusive(array, array, viskores::Multiply(), initialValue);

      VISKORES_TEST_ASSERT(product == 0.0f, "ScanExclusive product result not 0.0");
      VISKORES_TEST_ASSERT(array.ReadPortal().Get(0) == initialValue,
                           "ScanExclusive result's first value != initialValue");
      auto portal = array.ReadPortal();
      for (std::size_t i = 1; i <= mid; ++i)
      {
        viskores::Id index = static_cast<viskores::Id>(i);
        viskores::Float64 expected = pow(1.01, static_cast<viskores::Float64>(i)) * initialValue;
        viskores::Float64 got = portal.Get(index);
        VISKORES_TEST_ASSERT(test_equal(got, expected), "Incorrect results for ScanExclusive");
      }
      for (std::size_t i = mid + 1; i < ARRAY_SIZE; ++i)
      {
        viskores::Id index = static_cast<viskores::Id>(i);
        VISKORES_TEST_ASSERT(portal.Get(index) == 0.0f, "Incorrect results for ScanExclusive");
      }
    }

    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Exclusive Scan with a viskores::Vec" << std::endl;

    {
      using Vec3 = viskores::Vec<Float64, 3>;
      using Vec3ArrayHandle = viskores::cont::ArrayHandle<viskores::Vec3f_64, StorageTag>;

      std::vector<Vec3> testValues(ARRAY_SIZE);

      for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
      {
        testValues[i] = TestValue(1, Vec3());
      }
      Vec3ArrayHandle values =
        viskores::cont::make_ArrayHandle(testValues, viskores::CopyFlag::Off);

      Vec3 sum = Algorithm::ScanExclusive(values, values);
      VISKORES_TEST_ASSERT(test_equal(sum, (TestValue(1, Vec3()) * ARRAY_SIZE)),
                           "Got bad sum from Exclusive Scan");
    }
  }

  static VISKORES_CONT void TestScanExtended()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Extended Scan" << std::endl;

    {

      //construct the index array
      IdArrayHandle array;
      {
        viskores::cont::Token token;
        Algorithm::Schedule(
          ClearArrayKernel(array.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag(), token)),
          ARRAY_SIZE);
      }

      // we now have an array whose sum = (OFFSET * ARRAY_SIZE),
      // let's validate that
      Algorithm::ScanExtended(array, array);
      VISKORES_TEST_ASSERT(array.GetNumberOfValues() == ARRAY_SIZE + 1, "Output size incorrect.");
      {
        auto portal = array.ReadPortal();
        for (viskores::Id i = 0; i < ARRAY_SIZE + 1; ++i)
        {
          const viskores::Id value = portal.Get(i);
          VISKORES_TEST_ASSERT(value == i * OFFSET, "Incorrect partial sum");
        }
      }

      array.Allocate(1, viskores::CopyFlag::On);
      array.WritePortal().Set(0, OFFSET);
      Algorithm::ScanExtended(array, array);
      VISKORES_TEST_ASSERT(array.GetNumberOfValues() == 2);
      {
        auto portal = array.ReadPortal();
        VISKORES_TEST_ASSERT(portal.Get(0) == 0, "Incorrect initial value");
        VISKORES_TEST_ASSERT(portal.Get(1) == OFFSET, "Incorrect total sum");
      }

      array.Allocate(0);
      Algorithm::ScanExtended(array, array);
      VISKORES_TEST_ASSERT(array.GetNumberOfValues() == 1);
      {
        auto portal = array.ReadPortal();
        VISKORES_TEST_ASSERT(portal.Get(0) == 0, "Incorrect initial value");
      }
    }

    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Extended Scan with multiplication operator" << std::endl;
    {
      std::vector<viskores::Float64> inputValues(ARRAY_SIZE);
      for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
      {
        inputValues[i] = 1.01;
      }

      std::size_t mid = ARRAY_SIZE / 2;
      inputValues[mid] = 0.0;

      viskores::cont::ArrayHandle<viskores::Float64> array =
        viskores::cont::make_ArrayHandle(inputValues, viskores::CopyFlag::On);

      viskores::Float64 initialValue = 2.00;
      Algorithm::ScanExtended(array, array, viskores::Multiply(), initialValue);

      VISKORES_TEST_ASSERT(array.GetNumberOfValues() == ARRAY_SIZE + 1,
                           "ScanExtended output size incorrect.");

      auto portal = array.ReadPortal();
      VISKORES_TEST_ASSERT(portal.Get(0) == initialValue,
                           "ScanExtended result's first value != initialValue");

      for (std::size_t i = 1; i <= mid; ++i)
      {
        viskores::Id index = static_cast<viskores::Id>(i);
        viskores::Float64 expected = pow(1.01, static_cast<viskores::Float64>(i)) * initialValue;
        viskores::Float64 got = portal.Get(index);
        VISKORES_TEST_ASSERT(test_equal(got, expected), "Incorrect results for ScanExtended");
      }
      for (std::size_t i = mid + 1; i < ARRAY_SIZE + 1; ++i)
      {
        viskores::Id index = static_cast<viskores::Id>(i);
        VISKORES_TEST_ASSERT(portal.Get(index) == 0.0f, "Incorrect results for ScanExtended");
      }
    }

    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Extended Scan with a viskores::Vec" << std::endl;

    {
      using Vec3 = viskores::Vec3f_64;
      using Vec3ArrayHandle = viskores::cont::ArrayHandle<Vec3, StorageTag>;

      std::vector<Vec3> testValues(ARRAY_SIZE);

      for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
      {
        testValues[i] = TestValue(1, Vec3());
      }
      Vec3ArrayHandle values = viskores::cont::make_ArrayHandle(testValues, viskores::CopyFlag::On);

      Algorithm::ScanExtended(values, values);
      VISKORES_TEST_ASSERT(test_equal(viskores::cont::ArrayGetValue(ARRAY_SIZE, values),
                                      (TestValue(1, Vec3()) * ARRAY_SIZE)),
                           "Got bad sum from ScanExtended");
    }
  }

  static VISKORES_CONT void TestErrorExecution()
  {
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "Testing Exceptions in Execution Environment" << std::endl;

    std::string message;
    try
    {
      Algorithm::Schedule(OneErrorKernel(), ARRAY_SIZE);
      Algorithm::Synchronize();
    }
    catch (viskores::cont::ErrorExecution& error)
    {
      message = error.GetMessage();
    }
    VISKORES_TEST_ASSERT(message == ERROR_MESSAGE, "Did not get expected error message.");

    message = "";
    try
    {
      Algorithm::Schedule(AllErrorKernel(), ARRAY_SIZE);
      Algorithm::Synchronize();
    }
    catch (viskores::cont::ErrorExecution& error)
    {
      message = error.GetMessage();
    }
    VISKORES_TEST_ASSERT(message == ERROR_MESSAGE, "Did not get expected error message.");

    // This is spcifically to test the cuda-backend but should pass for all backends
    std::cout << "Testing if execution errors are eventually propagated to the host "
              << "without explicit synchronization\n";
    message = "";
    int nkernels = 0;
    try
    {
      viskores::cont::Token token;

      IdArrayHandle idArray;
      auto portal = idArray.PrepareForOutput(ARRAY_SIZE, DeviceAdapterTag{}, token);

      Algorithm::Schedule(OneErrorKernel(), ARRAY_SIZE);
      for (; nkernels < 100; ++nkernels)
      {
        Algorithm::Schedule(MakeAddArrayKernel(portal), ARRAY_SIZE);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
      }
      Algorithm::Synchronize();
    }
    catch (viskores::cont::ErrorExecution& error)
    {
      std::cout << "Got expected error: \"" << error.GetMessage() << "\" ";
      if (nkernels < 100)
      {
        std::cout << "after " << nkernels << " invocations of other kernel" << std::endl;
      }
      else
      {
        std::cout << "only after explicit synchronization" << std::endl;
      }
      message = error.GetMessage();
    }
    std::cout << "\n";
    VISKORES_TEST_ASSERT(message == ERROR_MESSAGE, "Did not get expected error message.");
  }

  template <typename T, int N = 0>
  struct TestCopy
  {
  };

  template <typename T>
  struct TestCopy<T>
  {
    static T get(viskores::Id i) { return static_cast<T>(i); }
  };

  template <typename T, int N>
  struct TestCopy<viskores::Vec<T, N>>
  {
    static viskores::Vec<T, N> get(viskores::Id i)
    {
      viskores::Vec<T, N> temp;
      for (int j = 0; j < N; ++j)
      {
        temp[j] = static_cast<T>(OFFSET + (i % 50));
      }
      return temp;
    }
  };

  template <typename T, typename U>
  struct TestCopy<viskores::Pair<T, U>>
  {
    static viskores::Pair<T, U> get(viskores::Id i)
    {
      return viskores::make_Pair(TestCopy<T>::get(i), TestCopy<U>::get(i));
    }
  };

  template <typename T>
  static VISKORES_CONT void TestCopyArrays()
  {
#define COPY_ARRAY_SIZE 10000

    std::vector<T> testData(COPY_ARRAY_SIZE);
    std::default_random_engine generator(static_cast<unsigned int>(std::time(nullptr)));

    viskores::Id index = 0;
    for (std::size_t i = 0; i < COPY_ARRAY_SIZE; ++i, ++index)
    {
      testData[i] = TestCopy<T>::get(index);
    }

    viskores::cont::ArrayHandle<T> input =
      viskores::cont::make_ArrayHandle(testData, viskores::CopyFlag::Off);

    //make a deep copy of input and place it into temp
    {
      viskores::cont::ArrayHandle<T> temp;
      temp.Allocate(COPY_ARRAY_SIZE * 2);
      Algorithm::Copy(input, temp);
      VISKORES_TEST_ASSERT(temp.GetNumberOfValues() == COPY_ARRAY_SIZE,
                           "Copy Needs to Resize Array");

      const auto& portal = temp.ReadPortal();

      std::uniform_int_distribution<viskores::Id> distribution(0, COPY_ARRAY_SIZE - 1);
      viskores::Id numberOfSamples = COPY_ARRAY_SIZE / 50;
      for (viskores::Id i = 0; i < numberOfSamples; ++i)
      {
        viskores::Id randomIndex = distribution(generator);
        T value = portal.Get(randomIndex);
        VISKORES_TEST_ASSERT(value == testData[static_cast<size_t>(randomIndex)],
                             "Got bad value (Copy)");
      }
    }

    //Verify copy of empty array works
    {
      viskores::cont::ArrayHandle<T> tempIn;
      viskores::cont::ArrayHandle<T> tempOut;

      tempOut.Allocate(COPY_ARRAY_SIZE);
      Algorithm::Copy(tempIn, tempOut);
      VISKORES_TEST_ASSERT(tempIn.GetNumberOfValues() == tempOut.GetNumberOfValues(),
                           "Copy sized wrong");

      // Actually allocate input array to 0 in case that makes a difference.
      tempIn.Allocate(0);
      tempOut.Allocate(COPY_ARRAY_SIZE);
      Algorithm::Copy(tempIn, tempOut);
      VISKORES_TEST_ASSERT(tempIn.GetNumberOfValues() == tempOut.GetNumberOfValues(),
                           "Copy sized wrong");
    }

    //CopySubRange tests:

    //1. Verify invalid input start position fails
    {
      viskores::cont::ArrayHandle<T> output;
      bool result = Algorithm::CopySubRange(input, COPY_ARRAY_SIZE * 4, 1, output);
      VISKORES_TEST_ASSERT(result == false, "CopySubRange when given bad input offset");
    }

    //2. Verify unallocated output gets allocated
    {
      viskores::cont::ArrayHandle<T> output;
      bool result = Algorithm::CopySubRange(input, 0, COPY_ARRAY_SIZE, output);
      VISKORES_TEST_ASSERT(result == true, "CopySubRange should succeed");
      VISKORES_TEST_ASSERT(output.GetNumberOfValues() == COPY_ARRAY_SIZE,
                           "CopySubRange needs to allocate output");
    }

    //3. Verify under allocated output gets resized properly
    {
      viskores::cont::ArrayHandle<T> output;
      output.Allocate(2);
      bool result = Algorithm::CopySubRange(input, 0, COPY_ARRAY_SIZE, output);
      VISKORES_TEST_ASSERT(result == true, "CopySubRange should succeed");
      VISKORES_TEST_ASSERT(output.GetNumberOfValues() == COPY_ARRAY_SIZE,
                           "CopySubRange needs to re-allocate output");
    }

    //4. Verify invalid input length gets shortened
    {
      viskores::cont::ArrayHandle<T> output;
      bool result = Algorithm::CopySubRange(input, 100, COPY_ARRAY_SIZE, output);
      VISKORES_TEST_ASSERT(result == true, "CopySubRange needs to shorten input range");
      VISKORES_TEST_ASSERT(output.GetNumberOfValues() == (COPY_ARRAY_SIZE - 100),
                           "CopySubRange needs to shorten input range");

      std::uniform_int_distribution<viskores::Id> distribution(0, COPY_ARRAY_SIZE - 100 - 1);
      viskores::Id numberOfSamples = (COPY_ARRAY_SIZE - 100) / 100;
      auto outputPortal = output.ReadPortal();
      for (viskores::Id i = 0; i < numberOfSamples; ++i)
      {
        viskores::Id randomIndex = distribution(generator);
        T value = outputPortal.Get(randomIndex);
        VISKORES_TEST_ASSERT(value == testData[static_cast<size_t>(randomIndex) + 100],
                             "Got bad value (CopySubRange 2)");
      }
    }

    //5. Verify sub range copy works when copying into a larger output
    {
      viskores::cont::ArrayHandle<T> output;
      output.Allocate(COPY_ARRAY_SIZE * 2);
      Algorithm::CopySubRange(input, 0, COPY_ARRAY_SIZE, output);
      Algorithm::CopySubRange(input, 0, COPY_ARRAY_SIZE, output, COPY_ARRAY_SIZE);
      VISKORES_TEST_ASSERT(output.GetNumberOfValues() == (COPY_ARRAY_SIZE * 2),
                           "CopySubRange needs to not resize array");

      std::uniform_int_distribution<viskores::Id> distribution(0, COPY_ARRAY_SIZE - 1);
      viskores::Id numberOfSamples = COPY_ARRAY_SIZE / 50;
      auto portal = output.ReadPortal();
      for (viskores::Id i = 0; i < numberOfSamples; ++i)
      {
        viskores::Id randomIndex = distribution(generator);
        T value = portal.Get(randomIndex);
        VISKORES_TEST_ASSERT(value == testData[static_cast<size_t>(randomIndex)],
                             "Got bad value (CopySubRange 5)");
        value = portal.Get(COPY_ARRAY_SIZE + randomIndex);
        VISKORES_TEST_ASSERT(value == testData[static_cast<size_t>(randomIndex)],
                             "Got bad value (CopySubRange 5)");
      }
    }

    //6. Verify that whey sub range needs to reallocate the output it
    // properly copies the original data instead of clearing it
    {
      viskores::cont::ArrayHandle<T> output;
      output.Allocate(COPY_ARRAY_SIZE);
      Algorithm::CopySubRange(input, 0, COPY_ARRAY_SIZE, output);
      Algorithm::CopySubRange(input, 0, COPY_ARRAY_SIZE, output, COPY_ARRAY_SIZE);
      VISKORES_TEST_ASSERT(output.GetNumberOfValues() == (COPY_ARRAY_SIZE * 2),
                           "CopySubRange needs too resize Array");
      std::uniform_int_distribution<viskores::Id> distribution(0, COPY_ARRAY_SIZE - 1);
      viskores::Id numberOfSamples = COPY_ARRAY_SIZE / 50;
      auto portal = output.ReadPortal();
      for (viskores::Id i = 0; i < numberOfSamples; ++i)
      {
        viskores::Id randomIndex = distribution(generator);
        T value = portal.Get(randomIndex);
        VISKORES_TEST_ASSERT(value == testData[static_cast<size_t>(randomIndex)],
                             "Got bad value (CopySubRange 6)");
        value = portal.Get(COPY_ARRAY_SIZE + randomIndex);
        VISKORES_TEST_ASSERT(value == testData[static_cast<size_t>(randomIndex)],
                             "Got bad value (CopySubRange 6)");
      }
    }

    // 7. Test that overlapping ranges trigger a failure:
    // 7.1 output starts inside input range:
    {
      const viskores::Id inBegin = 100;
      const viskores::Id inEnd = 200;
      const viskores::Id outBegin = 150;

      const viskores::Id numVals = inEnd - inBegin;
      bool result = Algorithm::CopySubRange(input, inBegin, numVals, input, outBegin);
      VISKORES_TEST_ASSERT(result == false, "Overlapping subrange did not fail.");
    }

    // 7.2 input starts inside output range
    {
      const viskores::Id inBegin = 100;
      const viskores::Id inEnd = 200;
      const viskores::Id outBegin = 50;

      const viskores::Id numVals = inEnd - inBegin;
      bool result = Algorithm::CopySubRange(input, inBegin, numVals, input, outBegin);
      VISKORES_TEST_ASSERT(result == false, "Overlapping subrange did not fail.");
    }

    {
      viskores::cont::ArrayHandle<T> output;

      //7. Verify negative input index returns false
      bool result = Algorithm::CopySubRange(input, -1, COPY_ARRAY_SIZE, output);
      VISKORES_TEST_ASSERT(result == false, "CopySubRange negative index should fail");

      //8. Verify negative input numberOfElementsToCopy returns false
      result = Algorithm::CopySubRange(input, 0, -COPY_ARRAY_SIZE, output);
      VISKORES_TEST_ASSERT(result == false, "CopySubRange negative number elements should fail");

      //9. Verify negative output index return false
      result = Algorithm::CopySubRange(input, 0, COPY_ARRAY_SIZE, output, -2);
      VISKORES_TEST_ASSERT(result == false, "CopySubRange negative output index should fail");
    }

#undef COPY_ARRAY_SIZE
  }

  static VISKORES_CONT void TestCopyArraysMany()
  {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "Testing Copy to same array type" << std::endl;
    TestCopyArrays<viskores::Vec3f_32>();
    TestCopyArrays<viskores::Vec4ui_8>();
    //
    TestCopyArrays<viskores::Pair<viskores::Id, viskores::Float32>>();
    TestCopyArrays<viskores::Pair<viskores::Id, viskores::Vec3f_32>>();
    //
    TestCopyArrays<viskores::Float32>();
    TestCopyArrays<viskores::Float64>();
    //
    TestCopyArrays<viskores::Int32>();
    TestCopyArrays<viskores::Int64>();
    //
    TestCopyArrays<viskores::UInt8>();
    TestCopyArrays<viskores::UInt16>();
    TestCopyArrays<viskores::UInt32>();
    TestCopyArrays<viskores::UInt64>();
  }

  static VISKORES_CONT void TestCopyArraysInDiffTypes()
  {
    std::cout << "-------------------------------------------------" << std::endl;
    std::cout << "Testing Copy to a different array type" << std::endl;
    std::vector<viskores::Id> testData(ARRAY_SIZE);
    for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
    {
      testData[i] = static_cast<viskores::Id>(OFFSET + (i % 50));
    }

    IdArrayHandle input =
      viskores::cont::make_ArrayHandle<viskores::Id>(testData, viskores::CopyFlag::Off);

    //make a deep copy of input and place it into temp
    viskores::cont::ArrayHandle<viskores::Float64> temp;
    Algorithm::Copy(input, temp);

    std::vector<viskores::Id>::const_iterator c = testData.begin();
    auto portal = temp.ReadPortal();
    for (viskores::Id i = 0; i < ARRAY_SIZE; ++i, ++c)
    {
      viskores::Float64 value = portal.Get(i);
      VISKORES_TEST_ASSERT(value == static_cast<viskores::Float64>(*c), "Got bad value (Copy)");
    }
  }

  static VISKORES_CONT void TestAtomicArray()
  {
    //we can't use ARRAY_SIZE as that would cause a overflow
    viskores::Int32 SHORT_ARRAY_SIZE = 10000;

    viskores::Int32 atomicCount = 0;
    for (viskores::Int32 i = 0; i < SHORT_ARRAY_SIZE; i++)
    {
      atomicCount += i;
    }
    std::cout << "-------------------------------------------" << std::endl;
    // To test the atomics, SHORT_ARRAY_SIZE number of threads will all increment
    // a single atomic value.
    std::cout << "Testing Atomic Add with viskores::Int32" << std::endl;
    {
      viskores::cont::ArrayHandle<viskores::Int32> atomicElement =
        viskores::cont::make_ArrayHandle<viskores::Int32>({ 0 });

      viskores::cont::AtomicArray<viskores::Int32> atomic(atomicElement);
      {
        viskores::cont::Token token;
        Algorithm::Schedule(AtomicKernel<viskores::Int32>(atomic, token), SHORT_ARRAY_SIZE);
      }
      viskores::Int32 expected = viskores::Int32(atomicCount);
      viskores::Int32 actual = atomicElement.WritePortal().Get(0);
      VISKORES_TEST_ASSERT(expected == actual, "Did not get expected value: Atomic add Int32");
    }

    std::cout << "Testing Atomic Add with viskores::Int64" << std::endl;
    {
      viskores::cont::ArrayHandle<viskores::Int64> atomicElement =
        viskores::cont::make_ArrayHandle<viskores::Int64>({ 0 });

      viskores::cont::AtomicArray<viskores::Int64> atomic(atomicElement);
      {
        viskores::cont::Token token;
        Algorithm::Schedule(AtomicKernel<viskores::Int64>(atomic, token), SHORT_ARRAY_SIZE);
      }
      viskores::Int64 expected = viskores::Int64(atomicCount);
      viskores::Int64 actual = atomicElement.WritePortal().Get(0);
      VISKORES_TEST_ASSERT(expected == actual, "Did not get expected value: Atomic add Int64");
    }

    std::cout << "Testing Atomic CAS with viskores::Int32" << std::endl;
    {
      viskores::cont::ArrayHandle<viskores::Int32> atomicElement =
        viskores::cont::make_ArrayHandle<viskores::Int32>({ 0 });

      viskores::cont::AtomicArray<viskores::Int32> atomic(atomicElement);
      {
        viskores::cont::Token token;
        Algorithm::Schedule(AtomicCASKernel<viskores::Int32>(atomic, token), SHORT_ARRAY_SIZE);
      }
      viskores::Int32 expected = viskores::Int32(atomicCount);
      viskores::Int32 actual = atomicElement.WritePortal().Get(0);
      VISKORES_TEST_ASSERT(expected == actual, "Did not get expected value: Atomic CAS Int32");
    }

    std::cout << "Testing Atomic CAS with viskores::Int64" << std::endl;
    {
      viskores::cont::ArrayHandle<viskores::Int64> atomicElement =
        viskores::cont::make_ArrayHandle<viskores::Int64>({ 0 });

      viskores::cont::AtomicArray<viskores::Int64> atomic(atomicElement);
      {
        viskores::cont::Token token;
        Algorithm::Schedule(AtomicCASKernel<viskores::Int64>(atomic, token), SHORT_ARRAY_SIZE);
      }
      viskores::Int64 expected = viskores::Int64(atomicCount);
      viskores::Int64 actual = atomicElement.WritePortal().Get(0);
      VISKORES_TEST_ASSERT(expected == actual, "Did not get expected value: Atomic CAS Int64");
    }
  }

  static VISKORES_CONT void TestBitFieldToUnorderedSet()
  {
    using IndexArray = viskores::cont::ArrayHandle<viskores::Id>;
    using WordType = WordTypeDefault;

    // Test that everything works correctly with a partial word at the end.
    static constexpr viskores::Id BitsPerWord =
      static_cast<viskores::Id>(sizeof(WordType) * CHAR_BIT);
    // +5 to get a partial word:
    static constexpr viskores::Id NumBits = 1024 * BitsPerWord + 5;
    static constexpr viskores::Id NumWords = (NumBits + BitsPerWord - 1) / BitsPerWord;

    auto testIndexArray = [](const BitField& bits)
    {
      const viskores::Id numBits = bits.GetNumberOfBits();
      IndexArray indices;
      Algorithm::BitFieldToUnorderedSet(bits, indices);
      Algorithm::Sort(indices);

      auto bitPortal = bits.ReadPortal();
      auto indexPortal = indices.ReadPortal();

      const viskores::Id numIndices = indices.GetNumberOfValues();
      viskores::Id curIndex = 0;
      for (viskores::Id curBit = 0; curBit < numBits; ++curBit)
      {
        const bool markedSet = curIndex < numIndices ? indexPortal.Get(curIndex) == curBit : false;
        const bool isSet = bitPortal.GetBit(curBit);

        //        std::cout << "curBit: " << curBit
        //                  << " activeIndex: "
        //                  << (curIndex < numIndices ? indexPortal.Get(curIndex) : -1)
        //                  << " isSet: " << isSet << " markedSet: " << markedSet << "\n";

        VISKORES_TEST_ASSERT(
          markedSet == isSet, "Bit ", curBit, " is set? ", isSet, " Marked set? ", markedSet);

        if (markedSet)
        {
          curIndex++;
        }
      }

      VISKORES_TEST_ASSERT(curIndex == indices.GetNumberOfValues(),
                           "Index array has extra values.");
    };

    auto testRepeatedMask = [&](WordType mask)
    {
      std::cout << "Testing BitFieldToUnorderedSet with repeated 32-bit word 0x" << std::hex << mask
                << std::dec << std::endl;

      BitField bits;
      {
        bits.Allocate(NumBits);
        auto fillPortal = bits.WritePortal();
        for (viskores::Id i = 0; i < NumWords; ++i)
        {
          fillPortal.SetWord(i, mask);
        }
      }

      testIndexArray(bits);
    };

    auto testRandomMask = [&](WordType seed)
    {
      std::cout << "Testing BitFieldToUnorderedSet with random sequence seeded with 0x" << std::hex
                << seed << std::dec << std::endl;

      std::mt19937 mt{ seed };
      std::uniform_int_distribution<std::mt19937::result_type> rng;

      BitField bits;
      {
        bits.Allocate(NumBits);
        auto fillPortal = bits.WritePortal();
        for (viskores::Id i = 0; i < NumWords; ++i)
        {
          fillPortal.SetWord(i, static_cast<WordType>(rng(mt)));
        }
      }

      testIndexArray(bits);
    };

    testRepeatedMask(0x00000000);
    testRepeatedMask(0xeeeeeeee);
    testRepeatedMask(0xffffffff);
    testRepeatedMask(0x1c0fd395);
    testRepeatedMask(0xdeadbeef);

    testRandomMask(0x00000000);
    testRandomMask(0xeeeeeeee);
    testRandomMask(0xffffffff);
    testRandomMask(0x1c0fd395);
    testRandomMask(0xdeadbeef);

    // This case was causing issues on CUDA:
    {
      BitField bits;
      Algorithm::Fill(bits, false, 32 * 32);
      auto portal = bits.WritePortal();
      portal.SetWord(2, 0x00100000ul);
      portal.SetWord(8, 0x00100010ul);
      portal.SetWord(11, 0x10000000ul);
      testIndexArray(bits);
    }
  }

  static VISKORES_CONT void TestCountSetBits()
  {
    using WordType = WordTypeDefault;

    // Test that everything works correctly with a partial word at the end.
    static constexpr viskores::Id BitsPerWord =
      static_cast<viskores::Id>(sizeof(WordType) * CHAR_BIT);
    // +5 to get a partial word:
    static constexpr viskores::Id NumFullWords = 1024;
    static constexpr viskores::Id NumBits = NumFullWords * BitsPerWord + 5;
    static constexpr viskores::Id NumWords = (NumBits + BitsPerWord - 1) / BitsPerWord;

    auto verifyPopCount = [](const BitField& bits)
    {
      viskores::Id refPopCount = 0;
      const viskores::Id numBits = bits.GetNumberOfBits();
      auto portal = bits.ReadPortal();
      for (viskores::Id idx = 0; idx < numBits; ++idx)
      {
        if (portal.GetBit(idx))
        {
          ++refPopCount;
        }
      }

      const viskores::Id popCount = Algorithm::CountSetBits(bits);

      VISKORES_TEST_ASSERT(
        refPopCount == popCount, "CountSetBits returned ", popCount, ", expected ", refPopCount);
    };

    auto testRepeatedMask = [&](WordType mask)
    {
      std::cout << "Testing CountSetBits with repeated word 0x" << std::hex << mask << std::dec
                << std::endl;

      BitField bits;
      {
        bits.Allocate(NumBits);
        auto fillPortal = bits.WritePortal();
        for (viskores::Id i = 0; i < NumWords; ++i)
        {
          fillPortal.SetWord(i, mask);
        }
      }

      verifyPopCount(bits);
    };

    auto testRandomMask = [&](WordType seed)
    {
      std::cout << "Testing CountSetBits with random sequence seeded with 0x" << std::hex << seed
                << std::dec << std::endl;

      std::mt19937 mt{ seed };
      std::uniform_int_distribution<std::mt19937::result_type> rng;

      BitField bits;
      {
        bits.Allocate(NumBits);
        auto fillPortal = bits.WritePortal();
        for (viskores::Id i = 0; i < NumWords; ++i)
        {
          fillPortal.SetWord(i, static_cast<WordType>(rng(mt)));
        }
      }

      verifyPopCount(bits);
    };

    testRepeatedMask(0x00000000);
    testRepeatedMask(0xeeeeeeee);
    testRepeatedMask(0xffffffff);
    testRepeatedMask(0x1c0fd395);
    testRepeatedMask(0xdeadbeef);

    testRandomMask(0x00000000);
    testRandomMask(0xeeeeeeee);
    testRandomMask(0xffffffff);
    testRandomMask(0x1c0fd395);
    testRandomMask(0xdeadbeef);

    // This case was causing issues on CUDA:
    {
      BitField bits;
      Algorithm::Fill(bits, false, 32 * 32);
      auto portal = bits.WritePortal();
      portal.SetWord(2, 0x00100000ul);
      portal.SetWord(8, 0x00100010ul);
      portal.SetWord(11, 0x10000000ul);
      verifyPopCount(bits);
    }
  }

  template <typename WordType>
  static VISKORES_CONT void TestFillBitFieldMask(WordType mask)
  {
    std::cout << "Testing Fill with " << (sizeof(WordType) * CHAR_BIT) << " bit mask: " << std::hex
              << viskores::UInt64{ mask } << std::dec << std::endl;

    // Test that everything works correctly with a partial word at the end.
    static constexpr viskores::Id BitsPerWord =
      static_cast<viskores::Id>(sizeof(WordType) * CHAR_BIT);
    // +5 to get a partial word:
    static constexpr viskores::Id NumFullWords = 1024;
    static constexpr viskores::Id NumBits = NumFullWords * BitsPerWord + 5;
    static constexpr viskores::Id NumWords = (NumBits + BitsPerWord - 1) / BitsPerWord;

    viskores::cont::BitField bits;
    {
      Algorithm::Fill(bits, mask, NumBits);

      viskores::Id numBits = bits.GetNumberOfBits();
      VISKORES_TEST_ASSERT(numBits == NumBits, "Unexpected number of bits.");
      viskores::Id numWords = bits.GetNumberOfWords<WordType>();
      VISKORES_TEST_ASSERT(numWords == NumWords, "Unexpected number of words.");

      auto portal = bits.ReadPortal();
      for (viskores::Id wordIdx = 0; wordIdx < NumWords; ++wordIdx)
      {
        VISKORES_TEST_ASSERT(portal.GetWord<WordType>(wordIdx) == mask,
                             "Incorrect word in result BitField; expected 0x",
                             std::hex,
                             viskores::UInt64{ mask },
                             ", got 0x",
                             viskores::UInt64{ portal.GetWord<WordType>(wordIdx) },
                             std::dec,
                             " for word ",
                             wordIdx,
                             "/",
                             NumWords);
      }
    }

    // Now fill the BitField with the reversed mask to test the no-alloc
    // overload:
    {
      WordType invWord = static_cast<WordType>(~mask);
      Algorithm::Fill(bits, invWord);

      viskores::Id numBits = bits.GetNumberOfBits();
      VISKORES_TEST_ASSERT(numBits == NumBits, "Unexpected number of bits.");
      viskores::Id numWords = bits.GetNumberOfWords<WordType>();
      VISKORES_TEST_ASSERT(numWords == NumWords, "Unexpected number of words.");

      auto portal = bits.ReadPortal();
      for (viskores::Id wordIdx = 0; wordIdx < NumWords; ++wordIdx)
      {
        VISKORES_TEST_ASSERT(portal.GetWord<WordType>(wordIdx) == invWord,
                             "Incorrect word in result BitField; expected 0x",
                             std::hex,
                             viskores::UInt64{ invWord },
                             ", got 0x",
                             viskores::UInt64{ portal.GetWord<WordType>(wordIdx) },
                             std::dec,
                             " for word ",
                             wordIdx,
                             "/",
                             NumWords);
      }
    }
  }

  static VISKORES_CONT void TestFillBitFieldBool(bool value)
  {
    std::cout << "Testing Fill with bool: " << value << std::endl;

    // Test that everything works correctly with a partial word at the end.
    // +5 to get a partial word:
    static constexpr viskores::Id NumBits = 1024 * 32 + 5;

    viskores::cont::BitField bits;
    {
      Algorithm::Fill(bits, value, NumBits);

      viskores::Id numBits = bits.GetNumberOfBits();
      VISKORES_TEST_ASSERT(numBits == NumBits, "Unexpected number of bits.");

      auto portal = bits.ReadPortal();
      for (viskores::Id bitIdx = 0; bitIdx < NumBits; ++bitIdx)
      {
        VISKORES_TEST_ASSERT(portal.GetBit(bitIdx) == value, "Incorrect bit in result BitField.");
      }
    }

    // Now fill the BitField with the reversed mask to test the no-alloc
    // overload:
    {
      Algorithm::Fill(bits, !value);

      viskores::Id numBits = bits.GetNumberOfBits();
      VISKORES_TEST_ASSERT(numBits == NumBits, "Unexpected number of bits.");

      auto portal = bits.ReadPortal();
      for (viskores::Id bitIdx = 0; bitIdx < NumBits; ++bitIdx)
      {
        VISKORES_TEST_ASSERT(portal.GetBit(bitIdx) == !value, "Incorrect bit in result BitField.");
      }
    }
  }

  static VISKORES_CONT void TestFillBitField()
  {
    TestFillBitFieldBool(true);
    TestFillBitFieldBool(false);
    TestFillBitFieldMask<viskores::UInt8>(viskores::UInt8{ 0 });
    TestFillBitFieldMask<viskores::UInt8>(static_cast<viskores::UInt8>(~viskores::UInt8{ 0 }));
    TestFillBitFieldMask<viskores::UInt8>(viskores::UInt8{ 0xab });
    TestFillBitFieldMask<viskores::UInt8>(viskores::UInt8{ 0x4f });
    TestFillBitFieldMask<viskores::UInt16>(viskores::UInt16{ 0 });
    TestFillBitFieldMask<viskores::UInt16>(static_cast<viskores::UInt16>(~viskores::UInt16{ 0 }));
    TestFillBitFieldMask<viskores::UInt16>(viskores::UInt16{ 0xfade });
    TestFillBitFieldMask<viskores::UInt16>(viskores::UInt16{ 0xbeef });
    TestFillBitFieldMask<viskores::UInt32>(viskores::UInt32{ 0 });
    TestFillBitFieldMask<viskores::UInt32>(static_cast<viskores::UInt32>(~viskores::UInt32{ 0 }));
    TestFillBitFieldMask<viskores::UInt32>(viskores::UInt32{ 0xfacecafe });
    TestFillBitFieldMask<viskores::UInt32>(viskores::UInt32{ 0xbaddecaf });
    TestFillBitFieldMask<viskores::UInt64>(viskores::UInt64{ 0 });
    TestFillBitFieldMask<viskores::UInt64>(static_cast<viskores::UInt64>(~viskores::UInt64{ 0 }));
    TestFillBitFieldMask<viskores::UInt64>(viskores::UInt64{ 0xbaddefacedfacade });
    TestFillBitFieldMask<viskores::UInt64>(viskores::UInt64{ 0xfeeddeadbeef2dad });
  }

  static VISKORES_CONT void TestFillArrayHandle()
  {
    viskores::cont::ArrayHandle<viskores::Int32> handle;
    Algorithm::Fill(handle, 867, ARRAY_SIZE);

    {
      auto portal = handle.ReadPortal();
      VISKORES_TEST_ASSERT(portal.GetNumberOfValues() == ARRAY_SIZE);
      for (viskores::Id i = 0; i < ARRAY_SIZE; ++i)
      {
        VISKORES_TEST_ASSERT(portal.Get(i) == 867);
      }
    }

    Algorithm::Fill(handle, 5309);
    {
      auto portal = handle.ReadPortal();
      VISKORES_TEST_ASSERT(portal.GetNumberOfValues() == ARRAY_SIZE);
      for (viskores::Id i = 0; i < ARRAY_SIZE; ++i)
      {
        VISKORES_TEST_ASSERT(portal.Get(i) == 5309);
      }
    }
  }

  struct TestAll
  {
    VISKORES_CONT void operator()() const
    {
      std::cout << "Doing DeviceAdapter tests" << std::endl;

      TestMemoryTransfer();
      TestOutOfMemory();
      TestTimer();

      TestAlgorithmSchedule();
      TestErrorExecution();

      TestReduce();
      TestReduceWithComparisonObject();
      TestReduceWithFancyArrays();

      TestReduceByKey();
      TestReduceByKeyWithFancyArrays();

      TestScanExclusive();
      TestScanExtended();

      TestScanInclusive();
      TestScanInclusiveWithComparisonObject();

      TestScanInclusiveByKeyOne();
      TestScanInclusiveByKeyTwo();
      TestScanInclusiveByKeyLarge();
      TestScanInclusiveByKey();
      TestScanInclusiveByKeyInPlace();
      TestScanInclusiveByKeyInPlaceWithFancyArray();

      TestScanExclusiveByKeyOne();
      TestScanExclusiveByKeyTwo();
      TestScanExclusiveByKeyLarge();
      TestScanExclusiveByKey();
      TestScanExclusiveByKeyInPlace();
      TestScanExclusiveByKeyInPlaceWithFancyArray();

      TestSort();
      TestSortWithComparisonObject();
      TestSortWithFancyArrays();
      TestSortByKey();

      TestLowerBoundsWithComparisonObject();

      TestUpperBoundsWithComparisonObject();

      TestUniqueWithComparisonObject();

      TestOrderedUniqueValues(); //tests Copy, LowerBounds, Sort, Unique
      TestCopyIf();

      TestCopyArraysMany();
      TestCopyArraysInDiffTypes();

      TestAtomicArray();

      TestBitFieldToUnorderedSet();
      TestCountSetBits();
      TestFillBitField();

      TestFillArrayHandle();
    }
  };

public:
  /// Run a suite of tests to check to see if a DeviceAdapter properly supports
  /// all members and classes required for driving viskores algorithms. Returns an
  /// error code that can be returned from the main function of a test.
  ///
  static VISKORES_CONT int Run(int argc, char* argv[])
  {
    return viskores::cont::testing::Testing::Run(TestAll(), argc, argv);
  }
};

#undef ERROR_MESSAGE
#undef ARRAY_SIZE
#undef OFFSET
#undef DIM
}
}
} // namespace viskores::cont::testing

#endif //viskores_cont_testing_TestingDeviceAdapter_h
