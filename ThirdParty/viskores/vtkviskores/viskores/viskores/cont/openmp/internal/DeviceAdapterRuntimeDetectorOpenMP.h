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
#ifndef viskores_cont_openmp_internal_DeviceAdapterRuntimeDetector_h
#define viskores_cont_openmp_internal_DeviceAdapterRuntimeDetector_h

#include <viskores/cont/openmp/internal/DeviceAdapterTagOpenMP.h>
#include <viskores/cont/viskores_cont_export.h>

namespace viskores
{
namespace cont
{

template <class DeviceAdapterTag>
class DeviceAdapterRuntimeDetector;

/// Determine if this machine supports Serial backend
///
template <>
class VISKORES_CONT_EXPORT DeviceAdapterRuntimeDetector<viskores::cont::DeviceAdapterTagOpenMP>
{
public:
  /// Returns true if the given device adapter is supported on the current
  /// machine.
  VISKORES_CONT bool Exists() const;
};
}
}

#endif
