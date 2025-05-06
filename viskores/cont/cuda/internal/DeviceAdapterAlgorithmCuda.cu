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

#include <viskores/cont/cuda/internal/DeviceAdapterAlgorithmCuda.h>

#include <viskores/cont/RuntimeDeviceInformation.h>
#include <viskores/cont/cuda/internal/DeviceAdapterTagCuda.h>
#include <viskores/cont/cuda/internal/RuntimeDeviceConfigurationCuda.h>

#include <atomic>
#include <cstring>
#include <functional>
#include <mutex>

#include <cuda.h>

// minwindef.h on Windows creates a preprocessor macro named PASCAL, which messes this up.
#ifdef PASCAL
#undef PASCAL
#endif

namespace viskores
{
namespace cont
{
namespace cuda
{

static viskores::cont::cuda::ScheduleParameters (
  *ComputeFromEnv)(const char*, int, int, int, int, int) = nullptr;

//Use the provided function as the the compute function for ScheduleParameterBuilder
VISKORES_CONT_EXPORT void InitScheduleParameters(
  viskores::cont::cuda::ScheduleParameters (*function)(const char*, int, int, int, int, int))
{
  ComputeFromEnv = function;
}

namespace internal
{

//These represent the best block/threads-per for scheduling on each GPU
static std::vector<std::pair<int, int>> scheduling_1d_parameters;
static std::vector<std::pair<int, dim3>> scheduling_2d_parameters;
static std::vector<std::pair<int, dim3>> scheduling_3d_parameters;

struct VISKORES_CONT_EXPORT ScheduleParameterBuilder
{
  //This represents information that is used to compute the best
  //ScheduleParameters for a given GPU
  enum struct GPU_STRATA
  {
    ENV = 0,
    OLDER = 5,
    PASCAL = 6,
    VOLTA = 7,
    PASCAL_HPC = 6000,
    VOLTA_HPC = 7000
  };

  std::map<GPU_STRATA, viskores::cont::cuda::ScheduleParameters> Presets;
  std::function<viskores::cont::cuda::ScheduleParameters(const char*, int, int, int, int, int)>
    Compute;

  // clang-format off
  // The presets for [one,two,three]_d_blocks are before we multiply by the number of SMs on the hardware
  ScheduleParameterBuilder()
    : Presets{
      { GPU_STRATA::ENV,        {  0,   0,  0, {  0,  0, 0 },  0, { 0, 0, 0 } } }, //use env settings
      { GPU_STRATA::OLDER,
                                { 32, 128,  8, { 16, 16, 1 }, 32, { 8, 8, 4 } } }, //Viskores default for less than pascal
      { GPU_STRATA::PASCAL,     { 32, 128,  8, { 16, 16, 1 }, 32, { 8, 8, 4 } } }, //Viskores default for pascal
      { GPU_STRATA::VOLTA,      { 32, 128,  8, { 16, 16, 1 }, 32, { 8, 8, 4 } } }, //Viskores default for volta
      { GPU_STRATA::PASCAL_HPC, { 32, 256, 16, { 16, 16, 1 }, 64, { 8, 8, 4 } } }, //P100
      { GPU_STRATA::VOLTA_HPC,  { 32, 256, 16, { 16, 16, 1 }, 64, { 8, 8, 4 } } }, //V100
    }
    , Compute(nullptr)
  {
    if (viskores::cont::cuda::ComputeFromEnv != nullptr)
    {
      this->Compute = viskores::cont::cuda::ComputeFromEnv;
    }
    else
    {
      this->Compute = [=] (const char* name, int major, int minor,
                          int numSMs, int maxThreadsPerSM, int maxThreadsPerBlock) -> ScheduleParameters  {
        return this->ComputeFromPreset(name, major, minor, numSMs, maxThreadsPerSM, maxThreadsPerBlock); };
    }
  }
  // clang-format on

  viskores::cont::cuda::ScheduleParameters ComputeFromPreset(const char* name,
                                                             int major,
                                                             int minor,
                                                             int numSMs,
                                                             int maxThreadsPerSM,
                                                             int maxThreadsPerBlock)
  {
    (void)minor;
    (void)maxThreadsPerSM;
    (void)maxThreadsPerBlock;

    const constexpr int GPU_STRATA_MAX_GEN = 7;
    const constexpr int GPU_STRATA_MIN_GEN = 5;
    int strataAsInt = std::min(major, GPU_STRATA_MAX_GEN);
    strataAsInt = std::max(strataAsInt, GPU_STRATA_MIN_GEN);
    if (strataAsInt > GPU_STRATA_MIN_GEN)
    { //only pascal and above have fancy

      //Currently the only
      bool is_tesla = (0 == std::strncmp("Tesla", name, 4)); //see if the name starts with Tesla
      if (is_tesla)
      {
        strataAsInt *= 1000; //tesla modifier
      }
    }

    auto preset = this->Presets.find(static_cast<GPU_STRATA>(strataAsInt));
    ScheduleParameters params = preset->second;
    params.one_d_blocks = params.one_d_blocks * numSMs;
    params.two_d_blocks = params.two_d_blocks * numSMs;
    params.three_d_blocks = params.three_d_blocks * numSMs;
    return params;
  }
};

VISKORES_CONT_EXPORT void SetupKernelSchedulingParameters()
{
  //check flag
  static std::once_flag lookupBuiltFlag;

  std::call_once(
    lookupBuiltFlag,
    []()
    {
      ScheduleParameterBuilder builder;
      auto cudaDeviceConfig = dynamic_cast<viskores::cont::internal::RuntimeDeviceConfiguration<
        viskores::cont::DeviceAdapterTagCuda>&>(
        viskores::cont::RuntimeDeviceInformation{}.GetRuntimeConfiguration(
          viskores::cont::DeviceAdapterTagCuda()));
      std::vector<cudaDeviceProp> cudaDevices;
      cudaDeviceConfig.GetCudaDeviceProp(cudaDevices);
      for (const auto& deviceProp : cudaDevices)
      {
        ScheduleParameters params = builder.Compute(deviceProp.name,
                                                    deviceProp.major,
                                                    deviceProp.minor,
                                                    deviceProp.multiProcessorCount,
                                                    deviceProp.maxThreadsPerMultiProcessor,
                                                    deviceProp.maxThreadsPerBlock);
        scheduling_1d_parameters.emplace_back(params.one_d_blocks, params.one_d_threads_per_block);
        scheduling_2d_parameters.emplace_back(params.two_d_blocks, params.two_d_threads_per_block);
        scheduling_3d_parameters.emplace_back(params.three_d_blocks,
                                              params.three_d_threads_per_block);
      }
    });
}
}
} // end namespace cuda::internal

// we use cuda pinned memory to reduce the amount of synchronization
// and mem copies between the host and device.
auto DeviceAdapterAlgorithm<viskores::cont::DeviceAdapterTagCuda>::GetPinnedErrorArray()
  -> const PinnedErrorArray&
{
  constexpr viskores::Id ERROR_ARRAY_SIZE = 1024;
  static thread_local PinnedErrorArray local;

  if (!local.HostPtr)
  {
    VISKORES_CUDA_CALL(
      cudaMallocHost((void**)&local.HostPtr, ERROR_ARRAY_SIZE, cudaHostAllocMapped));
    VISKORES_CUDA_CALL(cudaHostGetDevicePointer(&local.DevicePtr, local.HostPtr, 0));
    local.HostPtr[0] = '\0'; // clear
    local.Size = ERROR_ARRAY_SIZE;
  }

  return local;
}

void DeviceAdapterAlgorithm<viskores::cont::DeviceAdapterTagCuda>::SetupErrorBuffer(
  viskores::exec::cuda::internal::TaskStrided& functor)
{
  auto pinnedArray = GetPinnedErrorArray();
  viskores::exec::internal::ErrorMessageBuffer errorMessage(pinnedArray.DevicePtr,
                                                            pinnedArray.Size);
  functor.SetErrorMessageBuffer(errorMessage);
}

void DeviceAdapterAlgorithm<viskores::cont::DeviceAdapterTagCuda>::CheckForErrors()
{
  auto pinnedArray = GetPinnedErrorArray();
  if (pinnedArray.HostPtr[0] != '\0')
  {
    VISKORES_CUDA_CALL(cudaStreamSynchronize(cudaStreamPerThread));
    auto excep = viskores::cont::ErrorExecution(pinnedArray.HostPtr);
    pinnedArray.HostPtr[0] = '\0'; // clear
    throw excep;
  }
}

void DeviceAdapterAlgorithm<viskores::cont::DeviceAdapterTagCuda>::GetBlocksAndThreads(
  viskores::UInt32& blocks,
  viskores::UInt32& threadsPerBlock,
  viskores::Id size,
  viskores::IdComponent maxThreadsPerBlock)
{
  (void)size;
  viskores::cont::cuda::internal::SetupKernelSchedulingParameters();

  viskores::Id deviceId;
  viskores::cont::RuntimeDeviceInformation()
    .GetRuntimeConfiguration(viskores::cont::DeviceAdapterTagCuda())
    .GetDeviceInstance(deviceId);
  const auto& params = cuda::internal::scheduling_1d_parameters[static_cast<size_t>(deviceId)];
  blocks = static_cast<viskores::UInt32>(params.first);
  threadsPerBlock = static_cast<viskores::UInt32>(params.second);
  if ((maxThreadsPerBlock > 0) &&
      (threadsPerBlock < static_cast<viskores::UInt32>(maxThreadsPerBlock)))
  {
    threadsPerBlock = static_cast<viskores::UInt32>(maxThreadsPerBlock);
  }
}

void DeviceAdapterAlgorithm<viskores::cont::DeviceAdapterTagCuda>::GetBlocksAndThreads(
  viskores::UInt32& blocks,
  dim3& threadsPerBlock,
  const dim3& size,
  viskores::IdComponent maxThreadsPerBlock)
{
  viskores::cont::cuda::internal::SetupKernelSchedulingParameters();

  viskores::Id deviceId;
  viskores::cont::RuntimeDeviceInformation()
    .GetRuntimeConfiguration(viskores::cont::DeviceAdapterTagCuda())
    .GetDeviceInstance(deviceId);
  if (size.z <= 1)
  { //2d images
    const auto& params = cuda::internal::scheduling_2d_parameters[static_cast<size_t>(deviceId)];
    blocks = static_cast<viskores::UInt32>(params.first);
    threadsPerBlock = params.second;
  }
  else
  { //3d images
    const auto& params = cuda::internal::scheduling_3d_parameters[static_cast<size_t>(deviceId)];
    blocks = static_cast<viskores::UInt32>(params.first);
    threadsPerBlock = params.second;
  }

  if (maxThreadsPerBlock > 0)
  {
    while ((threadsPerBlock.x * threadsPerBlock.y * threadsPerBlock.z) >
           static_cast<viskores::UInt32>(maxThreadsPerBlock))
    {
      // Reduce largest element until threads are small enough.
      if (threadsPerBlock.x > threadsPerBlock.y)
      {
        threadsPerBlock.x /= 2;
      }
      else if (threadsPerBlock.y > threadsPerBlock.z)
      {
        threadsPerBlock.y /= 2;
      }
      else
      {
        threadsPerBlock.z /= 2;
      }
    }
  }
}

void DeviceAdapterAlgorithm<viskores::cont::DeviceAdapterTagCuda>::LogKernelLaunch(
  const cudaFuncAttributes& func_attrs,
  const std::type_info& worklet_info,
  viskores::UInt32 blocks,
  viskores::UInt32 threadsPerBlock,
  viskores::Id)
{
  (void)func_attrs;
  (void)blocks;
  (void)threadsPerBlock;
  std::string name = viskores::cont::TypeToString(worklet_info);
  VISKORES_LOG_F(viskores::cont::LogLevel::KernelLaunches,
                 "Launching 1D kernel %s on CUDA [ptx=%i, blocks=%i, threadsPerBlock=%i]",
                 name.c_str(),
                 (func_attrs.ptxVersion * 10),
                 blocks,
                 threadsPerBlock);
}

void DeviceAdapterAlgorithm<viskores::cont::DeviceAdapterTagCuda>::LogKernelLaunch(
  const cudaFuncAttributes& func_attrs,
  const std::type_info& worklet_info,
  viskores::UInt32 blocks,
  dim3 threadsPerBlock,
  const dim3&)
{
  (void)func_attrs;
  (void)blocks;
  (void)threadsPerBlock;
  std::string name = viskores::cont::TypeToString(worklet_info);
  VISKORES_LOG_F(viskores::cont::LogLevel::KernelLaunches,
                 "Launching 3D kernel %s on CUDA [ptx=%i, blocks=%i, threadsPerBlock=%i, %i, %i]",
                 name.c_str(),
                 (func_attrs.ptxVersion * 10),
                 blocks,
                 threadsPerBlock.x,
                 threadsPerBlock.y,
                 threadsPerBlock.z);
}
}
} // end namespace viskores::cont
