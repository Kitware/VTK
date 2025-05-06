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

#include <viskores/cont/cuda/internal/testing/Testing.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/RuntimeDeviceTracker.h>

#include <viskores/cont/cuda/DeviceAdapterCuda.h>
#include <viskores/cont/cuda/ErrorCuda.h>

#include <viskores/cont/cuda/internal/CudaAllocator.h>
#include <viskores/cont/cuda/internal/testing/Testing.h>

#include <cuda_runtime.h>

using viskores::cont::cuda::internal::CudaAllocator;

namespace
{

template <typename ValueType>
ValueType* AllocateManagedPointer(viskores::Id numValues)
{
  void* result;
  VISKORES_CUDA_CALL(
    cudaMallocManaged(&result, static_cast<size_t>(numValues) * sizeof(ValueType)));
  // Some sanity checks:
  VISKORES_TEST_ASSERT(CudaAllocator::IsDevicePointer(result),
                       "Allocated pointer is not a device pointer.");
  VISKORES_TEST_ASSERT(CudaAllocator::IsManagedPointer(result),
                       "Allocated pointer is not managed.");
  return static_cast<ValueType*>(result);
}

void DeallocateManagedPointer(void* ptr)
{
  VISKORES_TEST_ASSERT(CudaAllocator::IsDevicePointer(ptr),
                       "Pointer to delete is not device pointer.");
  VISKORES_TEST_ASSERT(CudaAllocator::IsManagedPointer(ptr), "Pointer to delete is not managed.");
  VISKORES_CUDA_CALL(cudaFree(ptr));
}

template <typename ValueType>
ValueType* AllocateDevicePointer(viskores::Id numValues)
{
  void* result;
  VISKORES_CUDA_CALL(cudaMalloc(&result, static_cast<size_t>(numValues) * sizeof(ValueType)));
  // Some sanity checks:
  VISKORES_TEST_ASSERT(CudaAllocator::IsDevicePointer(result),
                       "Allocated pointer is not a device pointer.");
  VISKORES_TEST_ASSERT(!CudaAllocator::IsManagedPointer(result), "Allocated pointer is managed.");
  return static_cast<ValueType*>(result);
}

void DeallocateDevicePointer(void* ptr)
{
  VISKORES_TEST_ASSERT(CudaAllocator::IsDevicePointer(ptr),
                       "Pointer to delete is not a device pointer.");
  VISKORES_TEST_ASSERT(!CudaAllocator::IsManagedPointer(ptr), "Pointer to delete is managed.");
  VISKORES_CUDA_CALL(cudaFree(ptr));
}

template <typename ValueType>
viskores::cont::ArrayHandle<ValueType> CreateArrayHandle(viskores::Id numValues, bool managed)
{
  if (managed)
  {
    return viskores::cont::ArrayHandleBasic<ValueType>(AllocateManagedPointer<ValueType>(numValues),
                                                       numValues,
                                                       [](void* ptr)
                                                       { DeallocateManagedPointer(ptr); });
  }
  else
  {
    return viskores::cont::ArrayHandleBasic<ValueType>(AllocateDevicePointer<ValueType>(numValues),
                                                       numValues,
                                                       viskores::cont::DeviceAdapterTagCuda{},
                                                       [](void* ptr)
                                                       { DeallocateDevicePointer(ptr); });
  }
}

template <typename ValueType>
void TestPrepareForInput(bool managed)
{
  viskores::cont::ArrayHandle<ValueType> handle = CreateArrayHandle<ValueType>(32, managed);
  viskores::cont::Token token;
  auto portal = handle.PrepareForInput(viskores::cont::DeviceAdapterTagCuda(), token);
  const void* execArray = portal.GetIteratorBegin();
  VISKORES_TEST_ASSERT(execArray != nullptr, "No execution array after PrepareForInput.");
  if (managed)
  {
    VISKORES_TEST_ASSERT(CudaAllocator::IsManagedPointer(execArray));
  }
  token.DetachFromAll();

  VISKORES_TEST_ASSERT(handle.IsOnDevice(viskores::cont::DeviceAdapterTagCuda{}),
                       "No execution array after PrepareForInput.");
  if (managed)
  {
    const void* contArray = handle.ReadPortal().GetIteratorBegin();
    VISKORES_TEST_ASSERT(CudaAllocator::IsManagedPointer(contArray), "Control array unmanaged.");
    VISKORES_TEST_ASSERT(execArray == contArray, "PrepareForInput managed arrays not shared.");
  }
}

template <typename ValueType>
void TestPrepareForInPlace(bool managed)
{
  viskores::cont::ArrayHandle<ValueType> handle = CreateArrayHandle<ValueType>(32, managed);
  viskores::cont::Token token;
  auto portal = handle.PrepareForInPlace(viskores::cont::DeviceAdapterTagCuda(), token);
  const void* execArray = portal.GetIteratorBegin();
  VISKORES_TEST_ASSERT(execArray != nullptr, "No execution array after PrepareForInPlace.");
  if (managed)
  {
    VISKORES_TEST_ASSERT(CudaAllocator::IsManagedPointer(execArray));
  }
  token.DetachFromAll();

  VISKORES_TEST_ASSERT(!handle.IsOnHost(), "Control array still exists after PrepareForInPlace.");
  VISKORES_TEST_ASSERT(handle.IsOnDevice(viskores::cont::DeviceAdapterTagCuda{}),
                       "No execution array after PrepareForInPlace.");
  if (managed)
  {
    const void* contArray = handle.ReadPortal().GetIteratorBegin();
    VISKORES_TEST_ASSERT(CudaAllocator::IsManagedPointer(contArray), "Control array unmanaged.");
    VISKORES_TEST_ASSERT(execArray == contArray, "PrepareForInPlace managed arrays not shared.");
  }
}

template <typename ValueType>
void TestPrepareForOutput(bool managed)
{
  // Should reuse a managed control pointer if buffer is large enough.
  viskores::cont::ArrayHandle<ValueType> handle = CreateArrayHandle<ValueType>(32, managed);
  viskores::cont::Token token;
  auto portal = handle.PrepareForOutput(32, viskores::cont::DeviceAdapterTagCuda(), token);
  const void* execArray = portal.GetIteratorBegin();
  VISKORES_TEST_ASSERT(execArray != nullptr, "No execution array after PrepareForOutput.");
  if (managed)
  {
    VISKORES_TEST_ASSERT(CudaAllocator::IsManagedPointer(execArray));
  }
  token.DetachFromAll();

  VISKORES_TEST_ASSERT(!handle.IsOnHost(), "Control array still exists after PrepareForOutput.");
  VISKORES_TEST_ASSERT(handle.IsOnDevice(viskores::cont::DeviceAdapterTagCuda{}),
                       "No execution array after PrepareForOutput.");
  if (managed)
  {
    const void* contArray = handle.ReadPortal().GetIteratorBegin();
    VISKORES_TEST_ASSERT(CudaAllocator::IsManagedPointer(contArray), "Control array unmanaged.");
    VISKORES_TEST_ASSERT(execArray == contArray, "PrepareForOutput managed arrays not shared.");
  }
}

template <typename ValueType>
void TestReleaseResourcesExecution(bool managed)
{
  viskores::cont::ArrayHandle<ValueType> handle = CreateArrayHandle<ValueType>(32, managed);
  viskores::cont::Token token;
  auto portal = handle.PrepareForInput(viskores::cont::DeviceAdapterTagCuda(), token);
  const void* origArray = portal.GetIteratorBegin();
  token.DetachFromAll();

  handle.ReleaseResourcesExecution();

  VISKORES_TEST_ASSERT(handle.IsOnHost(),
                       "Control array does not exist after ReleaseResourcesExecution.");
  VISKORES_TEST_ASSERT(!handle.IsOnDevice(viskores::cont::DeviceAdapterTagCuda{}),
                       "Execution array still exists after ReleaseResourcesExecution.");

  if (managed)
  {
    const void* contArray = handle.ReadPortal().GetIteratorBegin();
    VISKORES_TEST_ASSERT(CudaAllocator::IsManagedPointer(contArray), "Control array unmanaged.");
    VISKORES_TEST_ASSERT(origArray == contArray, "Managed arrays not shared.");
  }
}

template <typename ValueType>
void TestRoundTrip(bool managed)
{
  viskores::cont::ArrayHandle<ValueType> handle = CreateArrayHandle<ValueType>(32, managed);
  const void* origExecArray;
  {
    viskores::cont::Token token;
    auto portal = handle.PrepareForOutput(32, viskores::cont::DeviceAdapterTagCuda(), token);
    origExecArray = portal.GetIteratorBegin();
  }

  VISKORES_TEST_ASSERT(!handle.IsOnHost());
  VISKORES_TEST_ASSERT(handle.IsOnDevice(viskores::cont::DeviceAdapterTagCuda{}));

  const void* contArray;
  {
    auto portal = handle.WritePortal();
    contArray = portal.GetIteratorBegin();
  }

  VISKORES_TEST_ASSERT(handle.IsOnHost());
  VISKORES_TEST_ASSERT(!handle.IsOnDevice(viskores::cont::DeviceAdapterTagCuda{}));
  if (managed)
  {
    VISKORES_TEST_ASSERT(CudaAllocator::IsManagedPointer(contArray));
    VISKORES_TEST_ASSERT(contArray == origExecArray);
  }

  const void* execArray;
  {
    viskores::cont::Token token;
    auto portal = handle.PrepareForInput(viskores::cont::DeviceAdapterTagCuda(), token);
    execArray = portal.GetIteratorBegin();
  }

  VISKORES_TEST_ASSERT(handle.IsOnHost());
  VISKORES_TEST_ASSERT(handle.IsOnDevice(viskores::cont::DeviceAdapterTagCuda{}));
  if (managed)
  {
    VISKORES_TEST_ASSERT(CudaAllocator::IsManagedPointer(execArray));
    VISKORES_TEST_ASSERT(execArray == contArray);
  }
}

template <typename ValueType>
void DoTests()
{
  TestPrepareForInput<ValueType>(false);
  TestPrepareForInPlace<ValueType>(false);
  TestPrepareForOutput<ValueType>(false);
  TestReleaseResourcesExecution<ValueType>(false);
  TestRoundTrip<ValueType>(false);


  // If this device does not support managed memory, skip the managed tests.
  if (!CudaAllocator::UsingManagedMemory())
  {
    std::cerr << "Skipping some tests -- device does not support managed memory.\n";
  }
  else
  {
    TestPrepareForInput<ValueType>(true);
    TestPrepareForInPlace<ValueType>(true);
    TestPrepareForOutput<ValueType>(true);
    TestReleaseResourcesExecution<ValueType>(true);
    TestRoundTrip<ValueType>(true);
  }
}

struct ArgToTemplateType
{
  template <typename ValueType>
  void operator()(ValueType) const
  {
    DoTests<ValueType>();
  }
};

void Launch()
{
  using Types = viskores::List<viskores::UInt8,
                               viskores::Vec<viskores::UInt8, 3>,
                               viskores::Float32,
                               viskores::Vec<viskores::Float32, 4>,
                               viskores::Float64,
                               viskores::Vec<viskores::Float64, 4>>;
  viskores::testing::Testing::TryTypes(ArgToTemplateType(), Types());
}

} // end anon namespace

int UnitTestCudaShareUserProvidedManagedMemory(int argc, char* argv[])
{
  auto& tracker = viskores::cont::GetRuntimeDeviceTracker();
  tracker.ForceDevice(viskores::cont::DeviceAdapterTagCuda{});
  int ret = viskores::cont::testing::Testing::Run(Launch, argc, argv);
  return viskores::cont::cuda::internal::Testing::CheckCudaBeforeExit(ret);
}
