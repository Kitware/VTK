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

#include <viskores/cont/Error.h>
#include <viskores/cont/ErrorBadValue.h>
#include <viskores/cont/Initialize.h>
#include <viskores/cont/RuntimeDeviceInformation.h>
#include <viskores/cont/RuntimeDeviceTracker.h>

//------------------------------------------------------------------------------
// This test ensures that exceptions thrown internally by the viskores_cont library
// can be correctly caught across library boundaries.
int UnitTestExceptions(int argc, char* argv[])
{
  viskores::cont::Initialize(argc, argv);
  auto& tracker = viskores::cont::GetRuntimeDeviceTracker();

  try
  {
    // This throws a ErrorBadValue from RuntimeDeviceTracker::CheckDevice,
    // which is compiled into the viskores_cont library:
    tracker.ResetDevice(viskores::cont::DeviceAdapterTagUndefined());
  }
  catch (viskores::cont::ErrorBadValue&)
  {
    return EXIT_SUCCESS;
  }

  std::cerr << "Did not catch expected ErrorBadValue exception.\n";
  return EXIT_FAILURE;
}
