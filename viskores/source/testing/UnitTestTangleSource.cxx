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

#include <viskores/source/Tangle.h>

#include <viskores/cont/Timer.h>
#include <viskores/cont/testing/Testing.h>

void TangleSourceTest()
{
  viskores::cont::Timer timer;
  timer.Start();

  viskores::source::Tangle source;
  source.SetCellDimensions({ 20, 20, 20 });
  viskores::cont::DataSet ds = source.Execute();


  double time = timer.GetElapsedTime();

  std::cout << "Default tangle took " << time << "s.\n";

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
    using ScalarHandleType = viskores::cont::ArrayHandle<viskores::Float32>;

    auto field = ds.GetPointField("tangle");
    auto dynData = field.GetData();
    VISKORES_TEST_ASSERT(dynData.IsType<ScalarHandleType>(), "Invalid scalar handle type.");
    ScalarHandleType handle = dynData.AsArrayHandle<ScalarHandleType>();
    auto data = handle.ReadPortal();

    VISKORES_TEST_ASSERT(test_equal(data.GetNumberOfValues(), 9261),
                         "Incorrect number of scalars.");

    VISKORES_TEST_ASSERT(test_equal(data.Get(0), 24.46), "Incorrect scalar value.");
    VISKORES_TEST_ASSERT(test_equal(data.Get(16), 16.1195), "Incorrect scalar value.");
    VISKORES_TEST_ASSERT(test_equal(data.Get(21), 20.5988), "Incorrect scalar value.");
    VISKORES_TEST_ASSERT(test_equal(data.Get(256), 8.58544), "Incorrect scalar value.");
    VISKORES_TEST_ASSERT(test_equal(data.Get(1024), 1.56976), "Incorrect scalar value.");
    VISKORES_TEST_ASSERT(test_equal(data.Get(1987), 1.04074), "Incorrect scalar value.");
    VISKORES_TEST_ASSERT(test_equal(data.Get(2048), 0.95236), "Incorrect scalar value.");
    VISKORES_TEST_ASSERT(test_equal(data.Get(3110), 6.39556), "Incorrect scalar value.");
    VISKORES_TEST_ASSERT(test_equal(data.Get(4097), 2.62186), "Incorrect scalar value.");
    VISKORES_TEST_ASSERT(test_equal(data.Get(6599), 7.79722), "Incorrect scalar value.");
    VISKORES_TEST_ASSERT(test_equal(data.Get(7999), 7.94986), "Incorrect scalar value.");
  }
}

int UnitTestTangleSource(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TangleSourceTest, argc, argv);
}
