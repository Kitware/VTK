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
#include <viskores/cont/cuda/internal/DeviceAdapterRuntimeDetectorCuda.h>

#ifdef VISKORES_CUDA

#include <mutex>

#include <cuda.h>
#include <viskores/Math.h>
#include <viskores/cont/cuda/ErrorCuda.h>

namespace
{
static std::once_flag deviceQueryFlag;
static int numDevices = 0;
static int archVersion = 0;

void queryNumberOfDevicesandHighestArchSupported(viskores::Int32& nod, viskores::Int32& has)
{
  // We currently cannot use RuntimeDeviceInformation{}.GetRuntimeConfiguration(
  // viskores::cont::DeviceAdapterTagCuda()) in this function due to constraints in
  // initialize that query device Existence before we initialize the Runtime
  // Configuration. Once those constraints are removed/fixed this file can be
  // updated to use that call instead of directly querying the cuda device
  std::call_once(deviceQueryFlag,
                 []()
                 {
                   //first query for the number of devices
                   auto res = cudaGetDeviceCount(&numDevices);
                   if (res != cudaSuccess)
                   {
                     numDevices = 0;
                   }

                   for (viskores::Int32 i = 0; i < numDevices; i++)
                   {
                     cudaDeviceProp prop;
                     res = cudaGetDeviceProperties(&prop, i);
                     if (res == cudaSuccess)
                     {
                       const viskores::Int32 arch = (prop.major * 10) + prop.minor;
                       archVersion = viskores::Max(arch, archVersion);
                     }
                   }
                 });
  nod = numDevices;
  has = archVersion;
}
} // anonymous namspace

#endif

namespace viskores
{
namespace cont
{

DeviceAdapterRuntimeDetector<viskores::cont::DeviceAdapterTagCuda>::DeviceAdapterRuntimeDetector()
  : NumberOfDevices(0)
  , HighestArchSupported(0)
{
#ifdef VISKORES_CUDA
  queryNumberOfDevicesandHighestArchSupported(this->NumberOfDevices, this->HighestArchSupported);
#endif
}

bool DeviceAdapterRuntimeDetector<viskores::cont::DeviceAdapterTagCuda>::Exists() const
{
  return this->NumberOfDevices > 0 && this->HighestArchSupported >= 30;
}
}
} // namespace viskores::cont
