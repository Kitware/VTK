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

#include <viskores/cont/ArrayHandleXGCCoordinates.h>
#include <viskores/cont/CellSetExtrude.h>
#include <viskores/cont/testing/Testing.h>

#include <viskores/filter/field_conversion/PointAverage.h>

namespace
{
std::vector<float> points_rz = { 1.72485139f, 0.020562f,   1.73493571f,
                                 0.02052826f, 1.73478011f, 0.02299051f }; //really a vec<float,2>
std::vector<int> topology = { 0, 2, 1 };
std::vector<int> nextNode = { 0, 1, 2 };

int TestCellSetExtrude()
{
  const std::size_t numPlanes = 8;

  auto coords = viskores::cont::make_ArrayHandleXGCCoordinates(points_rz, numPlanes, false);
  auto cells = viskores::cont::make_CellSetExtrude(topology, coords, nextNode);
  VISKORES_TEST_ASSERT(cells.GetNumberOfPoints() == coords.GetNumberOfValues(),
                       "number of points don't match between cells and coordinates");

  //test a filter
  viskores::cont::DataSet dataset;

  dataset.AddCoordinateSystem(viskores::cont::CoordinateSystem("coords", coords));
  dataset.SetCellSet(cells);

  // verify that a constant value point field can be accessed
  std::vector<float> pvalues(static_cast<size_t>(coords.GetNumberOfValues()), 42.0f);
  viskores::cont::Field pfield = viskores::cont::make_Field(
    "pfield", viskores::cont::Field::Association::Points, pvalues, viskores::CopyFlag::Off);
  dataset.AddField(pfield);

  // verify that a constant cell value can be accessed
  std::vector<float> cvalues(static_cast<size_t>(cells.GetNumberOfCells()), 42.0f);
  viskores::cont::Field cfield = viskores::cont::make_Field(
    "cfield", viskores::cont::Field::Association::Cells, cvalues, viskores::CopyFlag::Off);
  dataset.AddField(cfield);

  viskores::filter::field_conversion::PointAverage avg;
  try
  {
    avg.SetActiveField("cfield");
    auto result = avg.Execute(dataset);
    VISKORES_TEST_ASSERT(result.HasPointField("cfield"),
                         "filter resulting dataset should be valid");
  }
  catch (const viskores::cont::Error& err)
  {
    std::cout << err.GetMessage() << std::endl;
    VISKORES_TEST_ASSERT(false, "Filter execution threw an exception");
  }


  return 0;
}
}

int UnitTestPointAverageCellSetExtrude(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestCellSetExtrude, argc, argv);
}
