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

#include <viskores/cont/DataSet.h>
#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

#include <set>

namespace
{

template <typename T, typename Storage>
bool TestArrayHandle(const viskores::cont::ArrayHandle<T, Storage>& ah,
                     const T* expected,
                     viskores::Id size)
{
  if (size != ah.GetNumberOfValues())
  {
    return false;
  }

  auto ahPortal = ah.ReadPortal();
  for (viskores::Id i = 0; i < size; ++i)
  {
    if (ahPortal.Get(i) != expected[i])
    {
      return false;
    }
  }

  return true;
}

void TestDataSet_Explicit()
{
  viskores::cont::testing::MakeTestDataSet tds;
  viskores::cont::DataSet ds = tds.Make3DExplicitDataSet0();

  VISKORES_TEST_ASSERT(ds.GetNumberOfFields() == 3, "Incorrect number of fields");

  // test various field-getting methods and associations
  const viskores::cont::Field& f1 = ds.GetField("pointvar");
  VISKORES_TEST_ASSERT(f1.GetAssociation() == viskores::cont::Field::Association::Points,
                       "Association of 'pointvar' was not Association::Points");
  try
  {
    ds.GetCellField("cellvar");
  }
  catch (...)
  {
    VISKORES_TEST_FAIL("Failed to get field 'cellvar' with Association::Cells.");
  }

  try
  {
    ds.GetPointField("cellvar");
    VISKORES_TEST_FAIL("Failed to get expected error for association mismatch.");
  }
  catch (viskores::cont::ErrorBadValue& error)
  {
    std::cout << "Caught expected error for association mismatch: " << std::endl
              << "    " << error.GetMessage() << std::endl;
  }

  VISKORES_TEST_ASSERT(ds.GetNumberOfCoordinateSystems() == 1,
                       "Incorrect number of coordinate systems");

  // test cell-to-point connectivity
  viskores::cont::CellSetExplicit<> cellset;
  ds.GetCellSet().AsCellSet(cellset);

  viskores::Id connectivitySize = 7;
  viskores::Id numPoints = 5;

  viskores::UInt8 correctShapes[] = { 1, 1, 1, 1, 1 };
  viskores::IdComponent correctNumIndices[] = { 1, 2, 2, 1, 1 };
  viskores::Id correctConnectivity[] = { 0, 0, 1, 0, 1, 1, 1 };

  viskores::cont::ArrayHandleConstant<viskores::UInt8> shapes =
    cellset.GetShapesArray(viskores::TopologyElementTagPoint(), viskores::TopologyElementTagCell());
  auto numIndices = cellset.GetNumIndicesArray(viskores::TopologyElementTagPoint(),
                                               viskores::TopologyElementTagCell());
  viskores::cont::ArrayHandle<viskores::Id> conn = cellset.GetConnectivityArray(
    viskores::TopologyElementTagPoint(), viskores::TopologyElementTagCell());

  VISKORES_TEST_ASSERT(TestArrayHandle(shapes, correctShapes, numPoints), "Got incorrect shapes");
  VISKORES_TEST_ASSERT(TestArrayHandle(numIndices, correctNumIndices, numPoints),
                       "Got incorrect numIndices");

  // Some device adapters have unstable sorts, which may cause the order of
  // the indices for each point to be different but still correct. Iterate
  // over all the points and check the connectivity for each one.
  VISKORES_TEST_ASSERT(conn.GetNumberOfValues() == connectivitySize,
                       "Connectivity array wrong size.");
  viskores::Id connectivityIndex = 0;
  auto connPortal = conn.ReadPortal();
  for (viskores::Id pointIndex = 0; pointIndex < numPoints; pointIndex++)
  {
    viskores::IdComponent numIncidentCells = correctNumIndices[pointIndex];
    std::set<viskores::Id> correctIncidentCells;
    for (viskores::IdComponent cellIndex = 0; cellIndex < numIncidentCells; cellIndex++)
    {
      correctIncidentCells.insert(correctConnectivity[connectivityIndex + cellIndex]);
    }
    for (viskores::IdComponent cellIndex = 0; cellIndex < numIncidentCells; cellIndex++)
    {
      viskores::Id expectedCell = connPortal.Get(connectivityIndex + cellIndex);
      std::set<viskores::Id>::iterator foundCell = correctIncidentCells.find(expectedCell);
      VISKORES_TEST_ASSERT(foundCell != correctIncidentCells.end(),
                           "An incident cell in the connectivity list is wrong or repeated.");
      correctIncidentCells.erase(foundCell);
    }
    connectivityIndex += numIncidentCells;
  }

  //verify that GetIndices works properly
  viskores::Id expectedPointIds[4] = { 2, 1, 3, 4 };
  viskores::Id4 retrievedPointIds;
  cellset.GetIndices(1, retrievedPointIds);
  for (viskores::IdComponent i = 0; i < 4; i++)
  {
    VISKORES_TEST_ASSERT(retrievedPointIds[i] == expectedPointIds[i],
                         "Incorrect point ID for quad cell");
  }
}

void TestAll()
{
  TestDataSet_Explicit();
};

} // anonymous namespace

int UnitTestDataSetExplicit(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestAll, argc, argv);
}
