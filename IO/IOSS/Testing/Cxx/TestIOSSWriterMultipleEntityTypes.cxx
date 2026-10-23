// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * Ensures that vtkIOSSWriter fails, without crashing, when a block is selected
 * as more than one entity type, and that it stops after the first time step.
 */

#include <vtkDataAssembly.h>
#include <vtkExecutive.h>
#include <vtkIOSSReader.h>
#include <vtkIOSSWriter.h>
#include <vtkInformation.h>
#include <vtkLogger.h>
#include <vtkNew.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkTestErrorObserver.h>
#include <vtkTestUtilities.h>
#include <vtkTesting.h>

#include <string>

namespace
{
std::string GetFileName(int argc, char* argv[], const std::string& fnameC)
{
  char* fileNameC = vtkTestUtilities::ExpandDataFileName(argc, argv, fnameC.c_str());
  std::string fname(fileNameC);
  delete[] fileNameC;
  return fname;
}

std::string GetOutputFileName(int argc, char* argv[], const std::string& suffix)
{
  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);
  auto* tempDir = testing->GetTempDirectory();
  if (!tempDir)
  {
    vtkLogF(ERROR, "No output directory specified!");
    return {};
  }

  return std::string(tempDir) + "/" + suffix;
}
}

int TestIOSSWriterMultipleEntityTypes(int argc, char* argv[])
{
  auto ofname = GetOutputFileName(argc, argv, "test_ioss_writer_multiple_entity_types.ex2");
  if (ofname.empty())
  {
    return EXIT_FAILURE;
  }

  vtkNew<vtkIOSSReader> reader;
  auto fname = GetFileName(argc, argv, std::string("Data/Exodus/can.e.4/can.e.4.0"));
  reader->SetFileName(fname.c_str());
  reader->UpdateInformation();
  auto* outInfo = reader->GetOutputInformation(0);
  const int numberOfTimeSteps = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  if (numberOfTimeSteps < 2)
  {
    vtkLogF(ERROR, "Expected multiple time steps, got %d.", numberOfTimeSteps);
    return EXIT_FAILURE;
  }
  reader->Update();
  auto* pdc = vtkPartitionedDataSetCollection::SafeDownCast(reader->GetOutputDataObject(0));
  auto* assembly = pdc ? pdc->GetDataAssembly() : nullptr;
  if (!assembly)
  {
    vtkLogF(ERROR, "Expected a vtkPartitionedDataSetCollection with an assembly.");
    return EXIT_FAILURE;
  }
  const auto numberOfElementBlocks = static_cast<int>(
    assembly->GetDataSetIndices(assembly->GetFirstNodeByPath("/IOSS/element_blocks")).size());

  // select the element blocks as both element blocks and node sets.
  vtkNew<vtkIOSSWriter> writer;
  writer->SetFileName(ofname.c_str());
  writer->SetInputConnection(reader->GetOutputPort());
  writer->SetAssemblyName("Assembly");
  writer->AddElementBlockSelector("/IOSS/element_blocks");
  writer->AddNodeSetSelector("/IOSS/element_blocks");

  vtkNew<vtkTest::ErrorObserver> errorObserver;
  vtkNew<vtkTest::ErrorObserver> errorObserverExec;
  writer->AddObserver(vtkCommand::ErrorEvent, errorObserver);
  writer->GetExecutive()->AddObserver(vtkCommand::ErrorEvent, errorObserverExec);

  if (writer->Write())
  {
    vtkLogF(ERROR, "Expected the write to fail.");
    return EXIT_FAILURE;
  }
  if (!errorObserver->HasErrorMessage("is selected as more than one entity type"))
  {
    return EXIT_FAILURE;
  }
  // one error per element block means that only the first time step was attempted.
  if (errorObserver->GetNumberOfErrors() != numberOfElementBlocks)
  {
    vtkLogF(ERROR, "Expected %d errors, got %d.", numberOfElementBlocks,
      errorObserver->GetNumberOfErrors());
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
