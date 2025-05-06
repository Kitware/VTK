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

#include <viskores/source/Oscillator.h>

#include <viskores/cont/Timer.h>
#include <viskores/cont/testing/Testing.h>

void OscillatorSourceTest()
{
  viskores::cont::Timer timer;
  timer.Start();

  viskores::source::Oscillator source;
  source.SetPointDimensions({ 21, 21, 21 });
  source.SetTime(0.5);
  source.AddDamped(0.25f, 0.25f, 0.25f, 0.5f, 0.1f, 0.2f);
  source.AddDecaying(0.5f, 0.5f, 0.5f, 0.35f, 0.2f, 0.1f);
  source.AddPeriodic(0.6f, 0.2f, 0.7f, 0.15f, 0.1f, 0.2f);

  viskores::cont::DataSet ds = source.Execute();

  double time = timer.GetElapsedTime();

  std::cout << "Default oscillator took " << time << "s.\n";

  {
    auto coords = ds.GetCoordinateSystem("coordinates");
    auto data = coords.GetData();
    VISKORES_TEST_ASSERT(test_equal(data.GetNumberOfValues(), 9261), "Incorrect number of points.");
  }

  {
    auto cells = ds.GetCellSet();
    VISKORES_TEST_ASSERT(test_equal(cells.GetNumberOfCells(), 8000), "Incorrect number of cells.");
  }

  // Spot check some node scalars
  {
    using ScalarHandleType = viskores::cont::ArrayHandle<viskores::FloatDefault>;

    auto field = ds.GetPointField("oscillating");
    auto dynData = field.GetData();
    VISKORES_TEST_ASSERT(dynData.IsType<ScalarHandleType>(), "Invalid scalar handle type.");
    ScalarHandleType handle = dynData.AsArrayHandle<ScalarHandleType>();
    auto data = handle.ReadPortal();

    VISKORES_TEST_ASSERT(test_equal(data.GetNumberOfValues(), 9261),
                         "Incorrect number of scalars.");

    VISKORES_TEST_ASSERT(test_equal(data.Get(0), -0.0163996), "Incorrect scalar value.");
    VISKORES_TEST_ASSERT(test_equal(data.Get(16), -0.0182232), "Incorrect scalar value.");
    VISKORES_TEST_ASSERT(test_equal(data.Get(21), -0.0181952), "Incorrect scalar value.");
    VISKORES_TEST_ASSERT(test_equal(data.Get(3110), -0.0404135), "Incorrect scalar value.");
  }
}

int UnitTestOscillatorSource(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(OscillatorSourceTest, argc, argv);
}
