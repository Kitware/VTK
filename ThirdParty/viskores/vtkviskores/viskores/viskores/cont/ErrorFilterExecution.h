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
#ifndef viskores_cont_ErrorFilterExecution_h
#define viskores_cont_ErrorFilterExecution_h

#include <viskores/cont/Error.h>

namespace viskores
{
namespace cont
{

VISKORES_SILENCE_WEAK_VTABLE_WARNING_START

/// This class is primarily intended to filters to throw in the control
/// environment to indicate an execution failure due to misconfiguration e.g.
/// incorrect parameters, etc. This is a device independent exception i.e. when
/// thrown, unlike most other exceptions, Viskores will not try to re-execute the
/// filter on another available device.
class VISKORES_ALWAYS_EXPORT ErrorFilterExecution : public viskores::cont::Error
{
public:
  ErrorFilterExecution(const std::string& message)
    : Error(message, /*is_device_independent=*/true)
  {
  }
};

VISKORES_SILENCE_WEAK_VTABLE_WARNING_END
}
} // namespace viskores::cont

#endif
