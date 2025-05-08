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
#ifndef viskores_cont_cuda_ChooseCudaDevice_h
#define viskores_cont_cuda_ChooseCudaDevice_h

#include <viskores/cont/ErrorExecution.h>

#include <viskores/cont/RuntimeDeviceInformation.h>
#include <viskores/cont/cuda/ErrorCuda.h>
#include <viskores/cont/cuda/internal/DeviceAdapterTagCuda.h>
#include <viskores/cont/cuda/internal/RuntimeDeviceConfigurationCuda.h>

#include <algorithm>
#include <set>
#include <vector>

VISKORES_THIRDPARTY_PRE_INCLUDE
#include <cuda.h>
VISKORES_THIRDPARTY_POST_INCLUDE

namespace viskores
{
namespace cont
{
namespace cuda
{

namespace
{
struct compute_info
{
  compute_info(cudaDeviceProp prop, int index)
  {
    this->Index = index;
    this->Major = prop.major;

    this->MemorySize = prop.totalGlobalMem;
    this->Performance =
      prop.multiProcessorCount * prop.maxThreadsPerMultiProcessor * (prop.clockRate / 100000.0);

    //9999 is equal to emulation make sure it is a super bad device
    if (this->Major >= 9999)
    {
      this->Major = -1;
      this->Performance = -1;
    }
  }

  //sort from fastest to slowest
  bool operator<(const compute_info other) const
  {
    //if we are both SM3 or greater check performance
    //if we both the same SM level check performance
    if ((this->Major >= 3 && other.Major >= 3) || (this->Major == other.Major))
    {
      return betterPerformance(other);
    }
    //prefer the greater SM otherwise
    return this->Major > other.Major;
  }

  bool betterPerformance(const compute_info other) const
  {
    if (this->Performance == other.Performance)
    {
      if (this->MemorySize == other.MemorySize)
      {
        //prefer first device over second device
        //this will be subjective I bet
        return this->Index < other.Index;
      }
      return this->MemorySize > other.MemorySize;
    }
    return this->Performance > other.Performance;
  }

  int GetIndex() const { return Index; }

private:
  int Index;
  int Major;
  size_t MemorySize;
  double Performance;
};
}

///Returns the fastest cuda device id that the current system has
///A result of zero means no cuda device has been found
static int FindFastestDeviceId()
{
  auto cudaDeviceConfig = dynamic_cast<
    viskores::cont::internal::RuntimeDeviceConfiguration<viskores::cont::DeviceAdapterTagCuda>&>(
    viskores::cont::RuntimeDeviceInformation{}.GetRuntimeConfiguration(
      viskores::cont::DeviceAdapterTagCuda()));
  viskores::Id numDevices;
  cudaDeviceConfig.GetMaxDevices(numDevices);

  // multiset stores elements in sorted order (allows duplicate values)
  std::multiset<compute_info> devices;
  std::vector<cudaDeviceProp> cudaProp;
  cudaDeviceConfig.GetCudaDeviceProp(cudaProp);
  for (int i = 0; i < numDevices; ++i)
  {
    if (cudaProp[i].computeMode != cudaComputeModeProhibited)
    {
      devices.emplace(cudaProp[i], i);
    }
  }

  return devices.size() > 0 ? devices.begin()->GetIndex() : 0;
}

/// Sets the current cuda device to the value returned by FindFastestDeviceId
static void SetFastestDeviceId()
{
  auto deviceId = FindFastestDeviceId();
  viskores::cont::RuntimeDeviceInformation{}
    .GetRuntimeConfiguration(viskores::cont::DeviceAdapterTagCuda())
    .SetDeviceInstance(deviceId);
}

}
}
} //namespace

#endif
