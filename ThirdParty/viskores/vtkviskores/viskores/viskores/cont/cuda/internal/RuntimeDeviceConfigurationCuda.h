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
#ifndef viskores_cont_cuda_internal_RuntimeDeviceConfigurationCuda_h
#define viskores_cont_cuda_internal_RuntimeDeviceConfigurationCuda_h

#include <viskores/cont/cuda/internal/DeviceAdapterRuntimeDetectorCuda.h>
#include <viskores/cont/cuda/internal/DeviceAdapterTagCuda.h>
#include <viskores/cont/internal/RuntimeDeviceConfiguration.h>

#include <viskores/cont/Logging.h>
#include <viskores/cont/cuda/ErrorCuda.h>

VISKORES_THIRDPARTY_PRE_INCLUDE
#include <cuda.h>
VISKORES_THIRDPARTY_POST_INCLUDE

#include <vector>

namespace viskores
{
namespace cont
{
namespace internal
{

template <>
class RuntimeDeviceConfiguration<viskores::cont::DeviceAdapterTagCuda>
  : public viskores::cont::internal::RuntimeDeviceConfigurationBase
{
public:
  RuntimeDeviceConfiguration<viskores::cont::DeviceAdapterTagCuda>()
  {
    this->CudaDeviceCount = 0;
    this->CudaProp.clear();
    viskores::cont::DeviceAdapterRuntimeDetector<viskores::cont::DeviceAdapterTagCuda> detector;
    if (detector.Exists())
    {
      try
      {
        int tmp;
        VISKORES_CUDA_CALL(cudaGetDeviceCount(&tmp));
        this->CudaDeviceCount = tmp;
        this->CudaProp.resize(this->CudaDeviceCount);
        for (int i = 0; i < this->CudaDeviceCount; ++i)
        {
          VISKORES_CUDA_CALL(cudaGetDeviceProperties(&this->CudaProp[i], i));
        }
      }
      catch (...)
      {
        VISKORES_LOG_F(viskores::cont::LogLevel::Error,
                       "Error retrieving CUDA device information. Disabling.");
        this->CudaDeviceCount = 0;
      }
    }
  }

  VISKORES_CONT viskores::cont::DeviceAdapterId GetDevice() const override final
  {
    return viskores::cont::DeviceAdapterTagCuda{};
  }

  VISKORES_CONT virtual RuntimeDeviceConfigReturnCode SetDeviceInstance(
    const viskores::Id& value) override final
  {
    if (value >= this->CudaDeviceCount)
    {
      VISKORES_LOG_S(
        viskores::cont::LogLevel::Error,
        "Failed to set CudaDeviceInstance, supplied id exceeds the number of available devices: "
          << value << " >= " << this->CudaDeviceCount);
      return RuntimeDeviceConfigReturnCode::INVALID_VALUE;
    }
    VISKORES_CUDA_CALL(cudaSetDevice(value));
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }

  VISKORES_CONT virtual RuntimeDeviceConfigReturnCode GetDeviceInstance(
    viskores::Id& value) const override final
  {
    int tmp;
    VISKORES_CUDA_CALL(cudaGetDevice(&tmp));
    value = tmp;
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }

  VISKORES_CONT virtual RuntimeDeviceConfigReturnCode GetMaxDevices(
    viskores::Id& value) const override final
  {
    value = this->CudaDeviceCount;
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }

  /// A function only available for use by the Cuda instance of this class
  /// Used to grab the CudaDeviceProp structs for all available devices
  VISKORES_CONT RuntimeDeviceConfigReturnCode
  GetCudaDeviceProp(std::vector<cudaDeviceProp>& value) const
  {
    value = CudaProp;
    return RuntimeDeviceConfigReturnCode::SUCCESS;
  }

private:
  std::vector<cudaDeviceProp> CudaProp;
  viskores::Id CudaDeviceCount;
};
} // namespace viskores::cont::internal
} // namespace viskores::cont
} // namespace viskores

#endif //viskores_cont_cuda_internal_RuntimeDeviceConfigurationCuda_h
