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
#ifndef viskores_cont_cuda_internal_DeviceAdapterRuntimeDetectorCuda_h
#define viskores_cont_cuda_internal_DeviceAdapterRuntimeDetectorCuda_h

#include <viskores/cont/viskores_cont_export.h>

#include <viskores/Types.h>

#include <viskores/cont/cuda/internal/DeviceAdapterTagCuda.h>

namespace viskores
{
namespace cont
{

template <class DeviceAdapterTag>
class DeviceAdapterRuntimeDetector;


/// \brief Class providing a CUDA runtime support detector.
///
/// The class provide the actual implementation used by
/// viskores::cont::RuntimeDeviceInformation for the CUDA backend.
///
/// We will verify at runtime that the machine has at least one CUDA
/// capable device, and said device is from the 'fermi' (SM_20) generation
/// or newer.
///
template <>
class VISKORES_CONT_EXPORT DeviceAdapterRuntimeDetector<viskores::cont::DeviceAdapterTagCuda>
{
public:
  VISKORES_CONT DeviceAdapterRuntimeDetector();

  /// Returns true if the given device adapter is supported on the current
  /// machine.
  ///
  /// Only returns true if we have at-least one CUDA capable device of SM_20 or
  /// greater ( fermi ).
  ///
  VISKORES_CONT bool Exists() const;

private:
  viskores::Int32 NumberOfDevices;
  viskores::Int32 HighestArchSupported;
};
}
} // namespace viskores::cont


#endif
