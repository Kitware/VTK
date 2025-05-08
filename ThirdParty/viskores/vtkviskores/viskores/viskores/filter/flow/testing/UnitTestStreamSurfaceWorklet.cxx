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

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/DataSetBuilderExplicit.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/flow/worklet/StreamSurface.h>
#include <viskores/io/VTKDataSetWriter.h>

namespace
{
void appendPts(viskores::cont::DataSetBuilderExplicitIterative& dsb,
               const viskores::Vec3f& pt,
               std::vector<viskores::Id>& ids)
{
  viskores::Id pid = dsb.AddPoint(pt);
  ids.push_back(pid);
}

void TestSameNumPolylines()
{
  using VecType = viskores::Vec3f;

  viskores::cont::DataSetBuilderExplicitIterative dsb;
  std::vector<viskores::Id> ids;

  ids.clear();
  appendPts(dsb, VecType(0, 0, 0), ids);
  appendPts(dsb, VecType(1, 1, 0), ids);
  appendPts(dsb, VecType(2, 1, 0), ids);
  appendPts(dsb, VecType(3, 0, 0), ids);
  dsb.AddCell(viskores::CELL_SHAPE_POLY_LINE, ids);

  ids.clear();
  appendPts(dsb, VecType(0, 0, 1), ids);
  appendPts(dsb, VecType(1, 1, 1), ids);
  appendPts(dsb, VecType(2, 1, 1), ids);
  appendPts(dsb, VecType(3, 0, 1), ids);
  dsb.AddCell(viskores::CELL_SHAPE_POLY_LINE, ids);

  ids.clear();
  appendPts(dsb, VecType(0, 0, 2), ids);
  appendPts(dsb, VecType(1, 1, 2), ids);
  appendPts(dsb, VecType(2, 1, 2), ids);
  appendPts(dsb, VecType(3, 0, 2), ids);
  dsb.AddCell(viskores::CELL_SHAPE_POLY_LINE, ids);

  viskores::cont::DataSet ds = dsb.Create();
  viskores::worklet::flow::StreamSurface streamSurfaceWorklet;
  viskores::cont::ArrayHandle<viskores::Vec3f> newPoints;
  viskores::cont::CellSetSingleType<> newCells;
  streamSurfaceWorklet.Run(ds.GetCoordinateSystem(0), ds.GetCellSet(), newPoints, newCells);

  VISKORES_TEST_ASSERT(newPoints.GetNumberOfValues() ==
                         ds.GetCoordinateSystem(0).GetNumberOfValues(),
                       "Wrong number of points in StreamSurface worklet");
  VISKORES_TEST_ASSERT(newCells.GetNumberOfCells() == 12,
                       "Wrong number of cells in StreamSurface worklet");
}

void TestUnequalNumPolylines(int unequalType)
{
  using VecType = viskores::Vec3f;

  viskores::cont::DataSetBuilderExplicitIterative dsb;
  std::vector<viskores::Id> ids;

  ids.clear();
  appendPts(dsb, VecType(0, 0, 0), ids);
  appendPts(dsb, VecType(1, 1, 0), ids);
  appendPts(dsb, VecType(2, 1, 0), ids);
  appendPts(dsb, VecType(3, 0, 0), ids);
  if (unequalType == 0)
  {
    appendPts(dsb, VecType(4, 0, 0), ids);
    appendPts(dsb, VecType(5, 0, 0), ids);
    appendPts(dsb, VecType(6, 0, 0), ids);
  }
  dsb.AddCell(viskores::CELL_SHAPE_POLY_LINE, ids);

  ids.clear();
  appendPts(dsb, VecType(0, 0, 1), ids);
  appendPts(dsb, VecType(1, 1, 1), ids);
  appendPts(dsb, VecType(2, 1, 1), ids);
  appendPts(dsb, VecType(3, 0, 1), ids);
  if (unequalType == 1)
  {
    appendPts(dsb, VecType(4, 0, 1), ids);
    appendPts(dsb, VecType(5, 0, 1), ids);
    appendPts(dsb, VecType(6, 0, 1), ids);
  }
  dsb.AddCell(viskores::CELL_SHAPE_POLY_LINE, ids);

  ids.clear();
  appendPts(dsb, VecType(0, 0, 2), ids);
  appendPts(dsb, VecType(1, 1, 2), ids);
  appendPts(dsb, VecType(2, 1, 2), ids);
  appendPts(dsb, VecType(3, 0, 2), ids);
  if (unequalType == 2)
  {
    appendPts(dsb, VecType(4, 0, 2), ids);
    appendPts(dsb, VecType(5, 0, 2), ids);
    appendPts(dsb, VecType(6, 0, 2), ids);
  }
  dsb.AddCell(viskores::CELL_SHAPE_POLY_LINE, ids);

  viskores::cont::DataSet ds = dsb.Create();
  viskores::worklet::flow::StreamSurface streamSurfaceWorklet;
  viskores::cont::ArrayHandle<viskores::Vec3f> newPoints;
  viskores::cont::CellSetSingleType<> newCells;
  streamSurfaceWorklet.Run(ds.GetCoordinateSystem(0), ds.GetCellSet(), newPoints, newCells);

  viskores::Id numRequiredCells = (unequalType == 1 ? 18 : 15);

  VISKORES_TEST_ASSERT(newPoints.GetNumberOfValues() ==
                         ds.GetCoordinateSystem(0).GetNumberOfValues(),
                       "Wrong number of points in StreamSurface worklet");
  VISKORES_TEST_ASSERT(newCells.GetNumberOfCells() == numRequiredCells,
                       "Wrong number of cells in StreamSurface worklet");
}

void TestStreamSurfaceWorklet()
{
  std::cout << "Testing Stream Surface Worklet" << std::endl;
  TestSameNumPolylines();
  TestUnequalNumPolylines(0);
  TestUnequalNumPolylines(1);
  TestUnequalNumPolylines(2);
}
}

int UnitTestStreamSurfaceWorklet(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestStreamSurfaceWorklet, argc, argv);
}
