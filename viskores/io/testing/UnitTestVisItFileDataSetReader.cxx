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
#include <viskores/io/ErrorIO.h>
#include <viskores/io/VTKVisItFileReader.h>

namespace
{

inline viskores::cont::PartitionedDataSet readVisItFileDataSet(const std::string& fname)
{
  viskores::cont::PartitionedDataSet pds;
  viskores::io::VTKVisItFileReader reader(fname);
  try
  {
    pds = reader.ReadPartitionedDataSet();
  }
  catch (viskores::io::ErrorIO& e)
  {
    std::string message("Error reading ");
    message += fname;
    message += ", ";
    message += e.GetMessage();

    VISKORES_TEST_FAIL(message.c_str());
  }

  return pds;
}

} // anonymous namespace

void TestReadingVisItFileDataSet()
{
  std::string visItFile = viskores::cont::testing::Testing::DataPath("uniform/venn250.visit");

  auto const& pds = readVisItFileDataSet(visItFile);
  VISKORES_TEST_ASSERT(pds.GetNumberOfPartitions() == 2, "Incorrect number of partitions");

  for (const auto& ds : pds)
  {
    VISKORES_TEST_ASSERT(ds.GetNumberOfPoints() == 63001, "Wrong number of points in partition");
    VISKORES_TEST_ASSERT(ds.GetNumberOfFields() == 5, "Wrong number of fields in partition");
  }
}


int UnitTestVisItFileDataSetReader(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestReadingVisItFileDataSet, argc, argv);
}
