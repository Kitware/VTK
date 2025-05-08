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

#include <viskores/cont/MergePartitionedDataSet.h>
#include <viskores/filter/entity_extraction/ExternalFaces.h>
#include <viskores/filter/entity_extraction/Threshold.h>
#include <viskores/source/Amr.h>

#include <viskores/rendering/testing/RenderTest.h>
#include <viskores/rendering/testing/Testing.h>

namespace
{

void TestAmrArraysExecute(int dim, int numberOfLevels, int cellsPerDimension)
{
  std::cout << "Generate Image for AMR" << std::endl;

  // Generate AMR
  viskores::source::Amr source;
  source.SetDimension(dim);
  source.SetNumberOfLevels(numberOfLevels);
  source.SetCellsPerDimension(cellsPerDimension);
  viskores::cont::PartitionedDataSet amrDataSet = source.Execute();
  //  std::cout << "amr " << std::endl;
  //  amrDataSet.PrintSummary(std::cout);

  // Remove blanked cells
  viskores::filter::entity_extraction::Threshold threshold;
  threshold.SetLowerThreshold(0);
  threshold.SetUpperThreshold(1);
  threshold.SetActiveField(viskores::cont::GetGlobalGhostCellFieldName());
  viskores::cont::PartitionedDataSet derivedDataSet = threshold.Execute(amrDataSet);
  //  std::cout << "derived " << std::endl;
  //  derivedDataSet.PrintSummary(std::cout);

  // Extract surface for efficient 3D pipeline
  viskores::filter::entity_extraction::ExternalFaces surface;
  surface.SetFieldsToPass("RTDataCells");
  derivedDataSet = surface.Execute(derivedDataSet);

  // Merge dataset
  viskores::cont::DataSet result = viskores::cont::MergePartitionedDataSet(derivedDataSet);
  //  std::cout << "merged " << std::endl;
  //  result.PrintSummary(std::cout);

  viskores::rendering::testing::RenderTestOptions testOptions;
  testOptions.AllowedPixelErrorRatio = 0.001f;
  testOptions.ColorTable = viskores::cont::ColorTable("inferno");
  testOptions.EnableAnnotations = false;
  viskores::rendering::testing::RenderTest(
    result, "RTDataCells", "filter/amrArrays" + std::to_string(dim) + "D.png", testOptions);
}

void TestAmrArrays()
{
  int numberOfLevels = 5;
  int cellsPerDimension = 6;
  TestAmrArraysExecute(2, numberOfLevels, cellsPerDimension);
  TestAmrArraysExecute(3, numberOfLevels, cellsPerDimension);
}
} // namespace

int RenderTestAmrArrays(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestAmrArrays, argc, argv);
}
