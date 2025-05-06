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

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/DeviceAdapterTag.h>
#include <viskores/cont/RuntimeDeviceTracker.h>
#include <viskores/cont/Token.h>

#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

namespace
{

// These should be constructed early in program startup and destroyed late on
// program shutdown. They will likely be destroyed after any device is cleaned up.
struct Data
{
  viskores::cont::ArrayHandle<viskores::Id> Array;
  viskores::cont::DataSet DataSet;

  ~Data() { std::cout << "Destroying global data." << std::endl; }
};
Data Globals;

void AllocateDeviceMemory()
{
  // Load data.
  viskores::cont::ArrayCopy(viskores::cont::ArrayHandleIndex(10), Globals.Array);
  Globals.DataSet = viskores::cont::testing::MakeTestDataSet{}.Make3DExplicitDataSet0();

  viskores::cont::CellSetExplicit<> cellSet;
  Globals.DataSet.GetCellSet().AsCellSet(cellSet);

  // Put data on devices.
  auto& tracker = viskores::cont::GetRuntimeDeviceTracker();
  for (viskores::Int8 deviceIndex = 0; deviceIndex < VISKORES_MAX_DEVICE_ADAPTER_ID; ++deviceIndex)
  {
    viskores::cont::DeviceAdapterId device = viskores::cont::make_DeviceAdapterId(deviceIndex);
    if (device.IsValueValid() && tracker.CanRunOn(device))
    {
      std::cout << "Loading data on " << device.GetName() << std::endl;

      viskores::cont::Token token;
      Globals.Array.PrepareForInput(device, token);
      cellSet.PrepareForInput(
        device, viskores::TopologyElementTagPoint{}, viskores::TopologyElementTagCell{}, token);
    }
  }
}

} // anonymous namespace

int UnitTestLateDeallocate(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(AllocateDeviceMemory, argc, argv);

  // After this test returns, the global data structures will be deallocated. This will likely
  // happen after all the devices are deallocated. You may get a warning, but you should not
  // get a crash.
}
