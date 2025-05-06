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

#include <viskores/cont/DataSetBuilderExplicit.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/geometry_refinement/Tube.h>

namespace
{

void appendPts(viskores::cont::DataSetBuilderExplicitIterative& dsb,
               const viskores::Vec3f& pt,
               std::vector<viskores::Id>& ids)
{
  viskores::Id pid = dsb.AddPoint(pt);
  ids.push_back(pid);
}

void TestTubeFilters()
{
  using VecType = viskores::Vec3f;

  viskores::cont::DataSetBuilderExplicitIterative dsb;
  std::vector<viskores::Id> ids;

  ids.clear();
  appendPts(dsb, VecType(0, 0, 0), ids);
  appendPts(dsb, VecType(1, 0, 0), ids);
  appendPts(dsb, VecType(2, 0, 0), ids);
  dsb.AddCell(viskores::CELL_SHAPE_POLY_LINE, ids);

  ids.clear();
  appendPts(dsb, VecType(0, 1, 0), ids);
  appendPts(dsb, VecType(1, 1, 0), ids);
  appendPts(dsb, VecType(2, 1, 0), ids);
  dsb.AddCell(viskores::CELL_SHAPE_POLY_LINE, ids);

  //add some degenerate polylines.
  //polyline with 1 point.
  ids.clear();
  appendPts(dsb, VecType(0, 0, 0), ids);
  dsb.AddCell(viskores::CELL_SHAPE_POLY_LINE, ids);

  //polyline with coincident points.
  ids.clear();
  appendPts(dsb, VecType(0, 0, 0), ids);
  appendPts(dsb, VecType(0, 0, 0), ids);
  dsb.AddCell(viskores::CELL_SHAPE_POLY_LINE, ids);

  viskores::cont::DataSet ds = dsb.Create();
  std::vector<viskores::FloatDefault> ptVar, cellVar;

  //Polyline 1.
  ptVar.push_back(0);
  ptVar.push_back(1);
  ptVar.push_back(2);
  cellVar.push_back(100);

  //Polyline 2.
  ptVar.push_back(10);
  ptVar.push_back(11);
  ptVar.push_back(12);
  cellVar.push_back(110);

  //Add some degenerate polylines.
  //Polyline 3: (only 1 point)
  ptVar.push_back(-1);
  cellVar.push_back(-1);
  //Polyline 4: (2 coincident points)
  ptVar.push_back(-1);
  ptVar.push_back(-1);
  cellVar.push_back(-1);

  ds.AddPointField("pointVar", ptVar);
  ds.AddCellField("cellVar", cellVar);

  viskores::filter::geometry_refinement::Tube tubeFilter;
  tubeFilter.SetCapping(true);
  tubeFilter.SetNumberOfSides(3);
  tubeFilter.SetRadius(static_cast<viskores::FloatDefault>(0.2));

  auto output = tubeFilter.Execute(ds);

  //Validate the result is correct.
  VISKORES_TEST_ASSERT(output.GetNumberOfCoordinateSystems() == 1,
                       "Wrong number of coordinate systems in the output dataset");

  viskores::cont::CoordinateSystem coords = output.GetCoordinateSystem();
  VISKORES_TEST_ASSERT(coords.GetNumberOfPoints() == 22, "Wrong number of coordinates");

  viskores::cont::UnknownCellSet dcells = output.GetCellSet();
  VISKORES_TEST_ASSERT(dcells.GetNumberOfCells() == 36, "Wrong number of cells");

  //Validate the point field
  viskores::cont::ArrayHandle<viskores::FloatDefault> ptArr;
  output.GetField("pointVar").GetData().AsArrayHandle(ptArr);
  VISKORES_TEST_ASSERT(ptArr.GetNumberOfValues() == 22, "Wrong number of values in point field");

  std::vector<viskores::FloatDefault> ptVals = { 0,  0,  0,  0,  1,  1,  1,  2,  2,  2,  2,
                                                 10, 10, 10, 10, 11, 11, 11, 12, 12, 12, 12 };
  auto portal = ptArr.ReadPortal();
  for (viskores::Id i = 0; i < 22; i++)
    VISKORES_TEST_ASSERT(portal.Get(i) == ptVals[static_cast<std::size_t>(i)],
                         "Wrong value for point field");

  //Validate the cell field
  viskores::cont::ArrayHandle<viskores::FloatDefault> cellArr;
  output.GetField("cellVar").GetData().AsArrayHandle(cellArr);
  VISKORES_TEST_ASSERT(cellArr.GetNumberOfValues() == 36, "Wrong number of values in cell field");
  std::vector<viskores::FloatDefault> cellVals = { 100, 100, 100, 100, 100, 100, 100, 100, 100,
                                                   100, 100, 100, 100, 100, 100, 100, 100, 100,
                                                   110, 110, 110, 110, 110, 110, 110, 110, 110,
                                                   110, 110, 110, 110, 110, 110, 110, 110, 110 };
  portal = cellArr.ReadPortal();
  for (viskores::Id i = 0; i < 36; i++)
    VISKORES_TEST_ASSERT(portal.Get(i) == cellVals[static_cast<std::size_t>(i)],
                         "Wrong value for cell field");
}
}

int UnitTestTubeFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestTubeFilters, argc, argv);
}
