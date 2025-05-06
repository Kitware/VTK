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
#ifndef viskores_cont_tbb_internal_DeviceAdapterTagTBB_h
#define viskores_cont_tbb_internal_DeviceAdapterTagTBB_h

#include <viskores/cont/DeviceAdapterTag.h>

/// @struct viskores::cont::DeviceAdapterTagTBB
/// @brief Tag for a device adapter that uses the Intel Threading Building Blocks
/// library to run algorithms on multiple threads.
///
/// For this device to work, Viskores must be configured to use TBB and the executable
/// must be linked to the TBB library. This tag is defined in
/// `viskores/cont/tbb/DeviceAdapterTBB.h`.

//We always create the tbb tag when included, but we only mark it as
//a valid tag when VISKORES_ENABLE_TBB is true. This is for easier development
//of multi-backend systems
#ifdef VISKORES_ENABLE_TBB
VISKORES_VALID_DEVICE_ADAPTER(TBB, VISKORES_DEVICE_ADAPTER_TBB);
#else
VISKORES_INVALID_DEVICE_ADAPTER(TBB, VISKORES_DEVICE_ADAPTER_TBB);
#endif

#endif //viskores_cont_tbb_internal_DeviceAdapterTagTBB_h
