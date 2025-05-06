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
#ifndef viskores_cont_internal_DIYMemoryManagement_h
#define viskores_cont_internal_DIYMemoryManagement_h

#include <viskores/cont/DeviceAdapterTag.h>
#include <viskores/cont/viskores_cont_export.h>
#include <viskores/thirdparty/diy/diy.h>

namespace viskores
{
namespace cont
{

VISKORES_CONT_EXPORT viskores::cont::DeviceAdapterId GetDIYDeviceAdapter();

/// \brief Wraps viskoresdiy::Master::exchange by setting its appropiate viskoresdiy::MemoryManagement.
VISKORES_CONT_EXPORT void DIYMasterExchange(viskoresdiy::Master& master, bool remote = false);

}
}

#endif
