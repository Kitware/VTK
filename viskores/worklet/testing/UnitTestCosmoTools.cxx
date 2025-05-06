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

#include <viskores/worklet/CosmoTools.h>
#include <viskores/worklet/DispatcherMapField.h>

#include <viskores/Pair.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>
#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/DataSetBuilderExplicit.h>
#include <viskores/cont/testing/Testing.h>

#include <fstream>

namespace
{

template <typename T, typename Storage>
bool TestArrayHandle(const viskores::cont::ArrayHandle<T, Storage>& ah,
                     const viskores::cont::ArrayHandle<T, Storage>& expected,
                     viskores::Id size)
{
  if (size != ah.GetNumberOfValues())
  {
    return false;
  }

  auto ahPortal = ah.ReadPortal();
  auto expectedPortal = expected.ReadPortal();
  for (viskores::Id i = 0; i < size; ++i)
  {
    if (ahPortal.Get(i) != expectedPortal.Get(i))
    {
      return false;
    }
  }
  return true;
}

//
// Test 2D explicit dataset of particles
//
viskores::cont::DataSet MakeCosmo_2DDataSet_0()
{
  viskores::cont::DataSet dataSet;
  viskores::cont::DataSetBuilderExplicit dsb;

  // Coordinates
  const int nVerts = 17;
  const int nCells = 17;
  using CoordType = viskores::Vec3f_32;
  std::vector<CoordType> coords(nVerts);

  coords[0] = CoordType(1, 1, 0);
  coords[1] = CoordType(1, 2, 0);
  coords[2] = CoordType(2, 6, 0);
  coords[3] = CoordType(1, 3, 0);
  coords[4] = CoordType(3, 5, 0);
  coords[5] = CoordType(1, 4, 0);
  coords[6] = CoordType(1, 5, 0);
  coords[7] = CoordType(3, 6, 0);
  coords[8] = CoordType(2, 3, 0);
  coords[9] = CoordType(3, 3, 0);
  coords[10] = CoordType(4, 3, 0);
  coords[11] = CoordType(3, 7, 0);
  coords[12] = CoordType(5, 2, 0);
  coords[13] = CoordType(5, 3, 0);
  coords[14] = CoordType(4, 6, 0);
  coords[15] = CoordType(5, 4, 0);
  coords[16] = CoordType(6, 3, 0);

  // Connectivity
  std::vector<viskores::UInt8> shapes;
  std::vector<viskores::IdComponent> numindices;
  std::vector<viskores::Id> conn;

  for (viskores::Id pt = 0; pt < nCells; pt++)
  {
    shapes.push_back(viskores::CELL_SHAPE_VERTEX);
    numindices.push_back(1);
    conn.push_back(pt);
  }
  dataSet = dsb.Create(coords, shapes, numindices, conn, "coordinates");

  // Field data
  viskores::Float32 xLocation[nCells] = { 1, 1, 2, 1, 3, 1, 1, 3, 2, 3, 4, 3, 5, 5, 4, 5, 6 };
  viskores::Float32 yLocation[nCells] = { 1, 2, 6, 3, 5, 4, 5, 6, 3, 3, 3, 7, 2, 3, 6, 4, 3 };
  viskores::Float32 zLocation[nCells] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

  viskores::Id haloId[nCells] = { 0, 0, 2, 0, 2, 0, 0, 2, 0, 0, 0, 2, 0, 0, 2, 0, 0 };
  viskores::Id mbp[nCells] = { 8, 8, 7, 8, 7, 8, 8, 7, 8, 8, 8, 7, 8, 8, 7, 8, 8 };

  dataSet.AddCellField("xLocation", xLocation, nCells);
  dataSet.AddCellField("yLocation", yLocation, nCells);
  dataSet.AddCellField("zLocation", zLocation, nCells);
  dataSet.AddCellField("haloId", haloId, nCells);
  dataSet.AddCellField("mbp", mbp, nCells);
  return dataSet;
}

//
// Test 3D explicit dataset of particles
//
viskores::cont::DataSet MakeCosmo_3DDataSet_0()
{
  viskores::cont::DataSet dataSet;
  viskores::cont::DataSetBuilderExplicit dsb;

  // Coordinates
  const int nVerts = 14;
  const int nCells = 14;
  using CoordType = viskores::Vec3f_32;
  std::vector<CoordType> coords(nVerts);

  coords[0] = CoordType(20.8125f, 10.8864f, 0.309784f);
  coords[1] = CoordType(29.6871f, 15.4445f, 1.55953f);
  coords[2] = CoordType(29.724f, 15.4766f, 1.51077f);
  coords[3] = CoordType(29.6783f, 15.4766f, 1.5324f);
  coords[4] = CoordType(29.7051f, 15.5052f, 1.52008f);
  coords[5] = CoordType(20.8172f, 10.8534f, 0.23461f);
  coords[6] = CoordType(20.8665f, 10.8679f, 0.254398f);
  coords[7] = CoordType(20.8271f, 10.8677f, 0.234255f);
  coords[8] = CoordType(20.8592f, 10.9505f, 0.248716f);
  coords[9] = CoordType(20.819f, 10.8949f, 0.304834f);
  coords[10] = CoordType(29.708f, 15.4251f, 1.53951f);
  coords[11] = CoordType(20.8829f, 10.9144f, 0.261517f);
  coords[12] = CoordType(20.8379f, 10.877f, 0.27677f);
  coords[13] = CoordType(29.7278f, 15.5267f, 1.50798f);

  // Connectivity
  std::vector<viskores::UInt8> shapes;
  std::vector<viskores::IdComponent> numindices;
  std::vector<viskores::Id> conn;

  for (viskores::Id pt = 0; pt < nCells; pt++)
  {
    shapes.push_back(viskores::CELL_SHAPE_VERTEX);
    numindices.push_back(1);
    conn.push_back(pt);
  }
  dataSet = dsb.Create(coords, shapes, numindices, conn, "coordinates");

  // Field data
  viskores::Float32 xLocation[nCells] = { 20.8125f, 29.6871f, 29.724f,  29.6783f, 29.7051f,
                                          20.8172f, 20.8665f, 20.8271f, 20.8592f, 20.819f,
                                          29.708f,  20.8829f, 20.8379f, 29.7278f };
  viskores::Float32 yLocation[nCells] = { 10.8864f, 15.4445f, 15.4766f, 15.4766f, 15.5052f,
                                          10.8534f, 10.8679f, 10.8677f, 10.9505f, 10.8949f,
                                          15.4251f, 10.9144f, 10.877f,  15.5267f };
  viskores::Float32 zLocation[nCells] = { 0.309784f, 1.55953f,  1.51077f,  1.5324f,   1.52008f,
                                          0.23461f,  0.254398f, 0.234255f, 0.248716f, 0.304834f,
                                          1.53951f,  0.261517f, 0.27677f,  1.50798f };

  viskores::Id haloId[nCells] = { 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1 };
  viskores::Id mbp[nCells] = { 9, 4, 4, 4, 4, 9, 9, 9, 9, 9, 4, 9, 9, 4 };

  dataSet.AddCellField("xLocation", xLocation, nCells);
  dataSet.AddCellField("yLocation", yLocation, nCells);
  dataSet.AddCellField("zLocation", zLocation, nCells);
  dataSet.AddCellField("haloId", haloId, nCells);
  dataSet.AddCellField("mbp", mbp, nCells);
  return dataSet;
}

} // namespace


////////////////////////////////////////////////////////////////////////////////////
//
// Create an explicit 2D particle set and find halos and minimum potential particle of each halo
//
////////////////////////////////////////////////////////////////////////////////////

void TestCosmo_2DHaloFind()
{
  std::cout << "Testing Halo Finder 2D" << std::endl;

  // Create the input 2D particle dataset
  viskores::cont::DataSet dataSet = MakeCosmo_2DDataSet_0();
  viskores::Id nCells = dataSet.GetNumberOfCells();

  viskores::cont::ArrayHandle<viskores::Float32> xLocArray;
  viskores::cont::ArrayHandle<viskores::Float32> yLocArray;
  viskores::cont::ArrayHandle<viskores::Float32> zLocArray;
  viskores::cont::ArrayHandle<viskores::Id> haloIdArray;
  viskores::cont::ArrayHandle<viskores::Id> mbpArray;

  dataSet.GetField("xLocation").GetData().AsArrayHandle(xLocArray);
  dataSet.GetField("yLocation").GetData().AsArrayHandle(yLocArray);
  dataSet.GetField("zLocation").GetData().AsArrayHandle(zLocArray);
  dataSet.GetField("haloId").GetData().AsArrayHandle(haloIdArray);
  dataSet.GetField("mbp").GetData().AsArrayHandle(mbpArray);

  // Output haloId, MBP, potential per particle
  viskores::cont::ArrayHandle<viskores::Id> resultHaloId;
  viskores::cont::ArrayHandle<viskores::Id> resultMBP;
  viskores::cont::ArrayHandle<viskores::Float32> resultPot;

  // Create the worklet and run it
  viskores::Id minHaloSize = 3;
  viskores::Float32 linkingLength = 1.0f;
  viskores::Float32 particleMass = 1.0f;

  viskores::worklet::CosmoTools cosmoTools;
  cosmoTools.RunHaloFinder(xLocArray,
                           yLocArray,
                           zLocArray,
                           nCells,
                           particleMass,
                           minHaloSize,
                           linkingLength,
                           resultHaloId,
                           resultMBP,
                           resultPot);

  VISKORES_TEST_ASSERT(TestArrayHandle(haloIdArray, resultHaloId, nCells), "Incorrect Halo Ids");
  VISKORES_TEST_ASSERT(TestArrayHandle(mbpArray, resultMBP, nCells), "Incorrect MBP Ids");
}

////////////////////////////////////////////////////////////////////////////////////
//
// Create an explicit 3D particle set and find halos and minimum potential particle of each halo
//
////////////////////////////////////////////////////////////////////////////////////

void TestCosmo_3DHaloFind()
{
  std::cout << "Testing Halo Finder 3D" << std::endl;

  // Create the input 3D particle dataset
  viskores::cont::DataSet dataSet = MakeCosmo_3DDataSet_0();
  viskores::Id nCells = dataSet.GetNumberOfCells();

  viskores::cont::ArrayHandle<viskores::Float32> xLocArray;
  viskores::cont::ArrayHandle<viskores::Float32> yLocArray;
  viskores::cont::ArrayHandle<viskores::Float32> zLocArray;
  viskores::cont::ArrayHandle<viskores::Id> haloIdArray;
  viskores::cont::ArrayHandle<viskores::Id> mbpArray;

  dataSet.GetField("xLocation").GetData().AsArrayHandle(xLocArray);
  dataSet.GetField("yLocation").GetData().AsArrayHandle(yLocArray);
  dataSet.GetField("zLocation").GetData().AsArrayHandle(zLocArray);
  dataSet.GetField("haloId").GetData().AsArrayHandle(haloIdArray);
  dataSet.GetField("mbp").GetData().AsArrayHandle(mbpArray);

  // Output haloId, MBP, potential per particle
  viskores::cont::ArrayHandle<viskores::Id> resultHaloId;
  viskores::cont::ArrayHandle<viskores::Id> resultMBP;
  viskores::cont::ArrayHandle<viskores::Float32> resultPot;

  // Create the worklet and run it
  viskores::Id minHaloSize = 3;
  viskores::Float32 linkingLength = 0.2f;
  viskores::Float32 particleMass = 1.0f;

  viskores::worklet::CosmoTools cosmoTools;
  cosmoTools.RunHaloFinder(xLocArray,
                           yLocArray,
                           zLocArray,
                           nCells,
                           particleMass,
                           minHaloSize,
                           linkingLength,
                           resultHaloId,
                           resultMBP,
                           resultPot);

  VISKORES_TEST_ASSERT(TestArrayHandle(haloIdArray, resultHaloId, nCells), "Incorrect Halo Ids");
  VISKORES_TEST_ASSERT(TestArrayHandle(mbpArray, resultMBP, nCells), "Incorrect MBP Ids");
}

////////////////////////////////////////////////////////////////////////////////////
//
// Create an explicit 3D particle set and find halos and minimum potential particle of each halo
//
////////////////////////////////////////////////////////////////////////////////////

void TestCosmo_3DCenterFind()
{
  std::cout << "Testing Center Finder 3D" << std::endl;

  // Create the input 3D particle dataset
  viskores::cont::DataSet dataSet = MakeCosmo_3DDataSet_0();
  viskores::Id nCells = dataSet.GetNumberOfCells();

  viskores::cont::ArrayHandle<viskores::Float32> xLocArray;
  viskores::cont::ArrayHandle<viskores::Float32> yLocArray;
  viskores::cont::ArrayHandle<viskores::Float32> zLocArray;
  viskores::cont::ArrayHandle<viskores::Id> haloIdArray;
  dataSet.GetField("xLocation").GetData().AsArrayHandle(xLocArray);
  dataSet.GetField("yLocation").GetData().AsArrayHandle(yLocArray);
  dataSet.GetField("zLocation").GetData().AsArrayHandle(zLocArray);

  // Output haloId MBP particleId pairs array
  viskores::Pair<viskores::Id, viskores::Float32> nxnResult;
  viskores::Pair<viskores::Id, viskores::Float32> mxnResult;

  // Create the worklet and run it
  viskores::Float32 particleMass = 1.0f;

  viskores::worklet::CosmoTools cosmoTools;
  cosmoTools.RunMBPCenterFinderNxN(
    xLocArray, yLocArray, zLocArray, nCells, particleMass, nxnResult);

  cosmoTools.RunMBPCenterFinderMxN(
    xLocArray, yLocArray, zLocArray, nCells, particleMass, mxnResult);

  VISKORES_TEST_ASSERT(test_equal(nxnResult.first, mxnResult.first),
                       "NxN and MxN got different results");
}

void TestCosmoTools()
{
  TestCosmo_2DHaloFind();
  TestCosmo_3DHaloFind();

  TestCosmo_3DCenterFind();
}

int UnitTestCosmoTools(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestCosmoTools, argc, argv);
}
