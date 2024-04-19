// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkXMLMultiBlockDataWriter.h"

#include "vtkTesting.h"
#include <string>

int TestXMLMultiBlockDataWriterWithEmptyLeaf(int argc, char* argv[])
{
  vtkNew<vtkMultiBlockDataSet> dataset;
  dataset->SetBlock(0, vtkNew<vtkMultiBlockDataSet>());

  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);
  std::string tempFile = testing->GetTempDirectory();
  tempFile += "/test.vtm";

  vtkNew<vtkXMLMultiBlockDataWriter> writer;
  writer->SetFileName(tempFile.c_str());
  writer->SetInputData(dataset);
  int returnCode = writer->Write();

  return returnCode ? EXIT_SUCCESS : EXIT_FAILURE;
}
