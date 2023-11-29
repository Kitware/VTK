// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPolyData.h"
#include "vtkTesting.h"
#include "vtkXMLMultiBlockDataReader.h"
#include "vtkXMLMultiBlockDataWriter.h"
#include <iostream>
#include <string>

int TestMultiBlockDataSetWithWriteWithEmptyPolyData(int argc, char* argv[])
{
  std::cout << "Start testing multi block data set " << std::endl;
  vtkNew<vtkMultiBlockDataSet> dataset;
  dataset->SetNumberOfBlocks(2);

  dataset->SetBlock(0, vtkNew<vtkPolyData>());
  std::string block0Name = "foo";
  dataset->GetMetaData(static_cast<unsigned int>(0))->Set(vtkCompositeDataSet::NAME(), block0Name);

  dataset->SetBlock(1, vtkNew<vtkPolyData>());
  std::string block1Name = "bar";
  dataset->GetMetaData(static_cast<unsigned int>(1))->Set(vtkCompositeDataSet::NAME(), block1Name);

  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);
  std::string tempFile = testing->GetTempDirectory();
  tempFile += "/test.vtm";

  vtkNew<vtkXMLMultiBlockDataWriter> writer;
  writer->SetFileName(tempFile.c_str());
  writer->SetInputData(dataset);
  writer->Write();

  // read back the MultiBlockDataSet
  vtkNew<vtkXMLMultiBlockDataReader> reader;
  reader->SetFileName(tempFile.c_str());
  reader->Update();
  vtkMultiBlockDataSet* writtenDataset = vtkMultiBlockDataSet::SafeDownCast(reader->GetOutput());
  // Need to check if the Names of the blocks are correctly written
  std::string writtenBlock0Name =
    writtenDataset->GetMetaData(static_cast<unsigned int>(0))->Get(vtkMultiBlockDataSet::NAME());
  std::string writtenBlock1Name =
    writtenDataset->GetMetaData(static_cast<unsigned int>(1))->Get(vtkMultiBlockDataSet::NAME());

  int returnCode = writtenBlock0Name == block0Name && writtenBlock1Name == block1Name;

  return returnCode ? EXIT_SUCCESS : EXIT_FAILURE;
}
