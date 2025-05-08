//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2019 UT-Battelle, LLC.
//  Copyright 2019 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================

#include <viskores/worklet/TriangleWinding.h>

#include <viskores/Types.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleGroupVecVariable.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/Field.h>

#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

using MyNormalT = viskores::Vec<viskores::Float32, 3>;

namespace
{

viskores::cont::DataSet GenerateDataSet()
{
  auto ds = viskores::cont::testing::MakeTestDataSet{}.Make3DExplicitDataSetPolygonal();
  const auto numCells = ds.GetNumberOfCells();

  viskores::cont::ArrayHandle<MyNormalT> cellNormals;
  cellNormals.AllocateAndFill(numCells, MyNormalT{ 1., 0., 0. });

  ds.AddField(viskores::cont::make_FieldCell("normals", cellNormals));
  return ds;
}

void Validate(viskores::cont::DataSet dataSet)
{
  const auto cellSet = dataSet.GetCellSet().AsCellSet<viskores::cont::CellSetExplicit<>>();
  const auto coordsArray = dataSet.GetCoordinateSystem().GetDataAsMultiplexer();
  const auto conn = cellSet.GetConnectivityArray(viskores::TopologyElementTagCell{},
                                                 viskores::TopologyElementTagPoint{});
  const auto offsets = cellSet.GetOffsetsArray(viskores::TopologyElementTagCell{},
                                               viskores::TopologyElementTagPoint{});
  const auto cellArray = viskores::cont::make_ArrayHandleGroupVecVariable(conn, offsets);
  const auto cellNormalsVar = dataSet.GetCellField("normals").GetData();
  const auto cellNormalsArray =
    cellNormalsVar.AsArrayHandle<viskores::cont::ArrayHandle<MyNormalT>>();

  const auto cellPortal = cellArray.ReadPortal();
  const auto cellNormals = cellNormalsArray.ReadPortal();
  const auto coords = coordsArray.ReadPortal();

  const auto numCells = cellPortal.GetNumberOfValues();
  VISKORES_TEST_ASSERT(numCells == cellNormals.GetNumberOfValues());

  for (viskores::Id cellId = 0; cellId < numCells; ++cellId)
  {
    const auto cell = cellPortal.Get(cellId);
    if (cell.GetNumberOfComponents() != 3)
    { // Triangles only!
      continue;
    }

    const MyNormalT cellNormal = cellNormals.Get(cellId);
    const MyNormalT p0 = coords.Get(cell[0]);
    const MyNormalT p1 = coords.Get(cell[1]);
    const MyNormalT p2 = coords.Get(cell[2]);
    const MyNormalT v01 = p1 - p0;
    const MyNormalT v02 = p2 - p0;
    const MyNormalT triangleNormal = viskores::Cross(v01, v02);
    VISKORES_TEST_ASSERT(viskores::Dot(triangleNormal, cellNormal) > 0,
                         "Triangle at index ",
                         cellId,
                         " incorrectly wound.");
  }
}

void DoTest()
{
  auto ds = GenerateDataSet();

  // Ensure that the test dataset needs to be rewound:
  bool threw = false;
  try
  {
    std::cerr << "Expecting an exception...\n";
    Validate(ds);
  }
  catch (...)
  {
    threw = true;
  }

  VISKORES_TEST_ASSERT(threw, "Test dataset is already wound consistently wrt normals.");

  auto cellSet = ds.GetCellSet().AsCellSet<viskores::cont::CellSetExplicit<>>();
  const auto coords = ds.GetCoordinateSystem().GetData();
  const auto cellNormalsVar = ds.GetCellField("normals").GetData();
  const auto cellNormals = cellNormalsVar.AsArrayHandle<viskores::cont::ArrayHandle<MyNormalT>>();


  auto newCells = viskores::worklet::TriangleWinding::Run(cellSet, coords, cellNormals);

  viskores::cont::DataSet result;
  result.AddCoordinateSystem(ds.GetCoordinateSystem());
  result.SetCellSet(newCells);
  for (viskores::Id i = 0; i < ds.GetNumberOfFields(); ++i)
  {
    result.AddField(ds.GetField(i));
  }

  Validate(result);
}

} // end anon namespace

int UnitTestTriangleWinding(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(DoTest, argc, argv);
}
