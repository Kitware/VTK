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

#include <viskores/source/Wavelet.h>

#include <viskores/cont/testing/Testing.h>

void WaveletSourceTest()
{
  viskores::source::Wavelet source;
  viskores::cont::DataSet ds = source.Execute();

  {
    auto coords = ds.GetCoordinateSystem("coordinates");
    auto data = coords.GetData();
    VISKORES_TEST_ASSERT(test_equal(data.GetNumberOfValues(), 9261), "Incorrect number of points.");
  }

  {
    auto cells = ds.GetCellSet();
    VISKORES_TEST_ASSERT(test_equal(cells.GetNumberOfCells(), 8000), "Incorrect number of cells.");
  }

  // Spot check some scalars
  {
    using ScalarHandleType = viskores::cont::ArrayHandle<viskores::FloatDefault>;

    auto field = ds.GetPointField("RTData");
    auto dynData = field.GetData();
    VISKORES_TEST_ASSERT(dynData.IsType<ScalarHandleType>(), "Invalid scalar handle type.");
    ScalarHandleType handle = dynData.AsArrayHandle<ScalarHandleType>();
    auto data = handle.ReadPortal();

    VISKORES_TEST_ASSERT(test_equal(data.GetNumberOfValues(), 9261),
                         "Incorrect number of scalars.");

    VISKORES_TEST_ASSERT(test_equal(data.Get(0), 60.7635), "Incorrect scalar value.");
    VISKORES_TEST_ASSERT(test_equal(data.Get(16), 99.6115), "Incorrect scalar value.");
    VISKORES_TEST_ASSERT(test_equal(data.Get(21), 69.1968), "Incorrect scalar value.");
    VISKORES_TEST_ASSERT(test_equal(data.Get(256), 118.620), "Incorrect scalar value.");
    VISKORES_TEST_ASSERT(test_equal(data.Get(1024), 140.466), "Incorrect scalar value.");
    VISKORES_TEST_ASSERT(test_equal(data.Get(1987), 203.720), "Incorrect scalar value.");
    VISKORES_TEST_ASSERT(test_equal(data.Get(2048), 223.010), "Incorrect scalar value.");
    VISKORES_TEST_ASSERT(test_equal(data.Get(3110), 128.282), "Incorrect scalar value.");
    VISKORES_TEST_ASSERT(test_equal(data.Get(4097), 153.913), "Incorrect scalar value.");
    VISKORES_TEST_ASSERT(test_equal(data.Get(6599), 120.068), "Incorrect scalar value.");
    VISKORES_TEST_ASSERT(test_equal(data.Get(7999), 65.6710), "Incorrect scalar value.");
  }
}

int UnitTestWaveletSource(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(WaveletSourceTest, argc, argv);
}
