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

#include <string>
#include <viskores/cont/testing/Testing.h>
#include <viskores/io/BOVDataSetReader.h>
#include <viskores/io/ErrorIO.h>

namespace
{

inline viskores::cont::DataSet readBOVDataSet(const char* fname)
{
  viskores::cont::DataSet ds;
  viskores::io::BOVDataSetReader reader(fname);
  try
  {
    ds = reader.ReadDataSet();
  }
  catch (viskores::io::ErrorIO& e)
  {
    std::string message("Error reading ");
    message += fname;
    message += ", ";
    message += e.GetMessage();

    VISKORES_TEST_FAIL(message.c_str());
  }

  return ds;
}

} // anonymous namespace

void TestReadingBOVDataSet()
{
  std::string bovFile =
    viskores::cont::testing::Testing::DataPath("third_party/visit/example_temp.bov");

  auto const& ds = readBOVDataSet(bovFile.data());

  VISKORES_TEST_ASSERT(ds.GetNumberOfFields() == 2, "Incorrect number of fields");
  // See the .bov file: DATA SIZE: 50 50 50
  VISKORES_TEST_ASSERT(ds.GetNumberOfPoints() == 50 * 50 * 50, "Incorrect number of points");
  VISKORES_TEST_ASSERT(ds.GetCellSet().GetNumberOfPoints() == 50 * 50 * 50,
                       "Incorrect number of points (from cell set)");
  VISKORES_TEST_ASSERT(ds.GetNumberOfCells() == 49 * 49 * 49, "Incorrect number of cells");
  // See the .bov file: VARIABLE: "var"
  VISKORES_TEST_ASSERT(ds.HasField("var"), "Should have field 'var', but does not.");
  VISKORES_TEST_ASSERT(ds.GetNumberOfFields() == 2, "There is only one field in noise.bov");
  VISKORES_TEST_ASSERT(ds.GetNumberOfCoordinateSystems() == 1,
                       "There is only one coordinate system in noise.bov");

  auto const& field = ds.GetField("var");
  // I'm pretty sure that all .bov files have their fields associated with points . . .
  VISKORES_TEST_ASSERT(field.GetAssociation() == viskores::cont::Field::Association::Points,
                       "The field should be associated with points.");
}


int UnitTestBOVDataSetReader(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestReadingBOVDataSet, argc, argv);
}
