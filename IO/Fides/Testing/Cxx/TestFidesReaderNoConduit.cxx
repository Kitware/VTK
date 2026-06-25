// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkCommand.h>
#include <vtkFidesReader.h>
#include <vtkInformation.h>
#include <vtkNew.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkTestUtilities.h>

#include <cstdlib>
#include <iostream>
#include <string>

// Custom observer to catch vtkErrorMacro emissions
class FidesErrorObserver : public vtkCommand
{
public:
  static FidesErrorObserver* New() { return new FidesErrorObserver; }
  vtkTypeMacro(FidesErrorObserver, vtkCommand);

  void Execute(vtkObject*, unsigned long eventId, void* callData) override
  {
    if (eventId == vtkCommand::ErrorEvent)
    {
      std::string errorMsg(static_cast<char*>(callData));
      std::string expectedText = "VTK must be compiled with Conduit support";

      if (errorMsg.find(expectedText) != std::string::npos)
      {
        this->ErrorReceived = true;
      }
    }
  }

  bool ErrorReceived = false;
};

int TestFidesReaderNoConduit(int argc, char* argv[])
{
  vtkNew<vtkFidesReader> reader;

  // Attach custom error observer
  vtkNew<FidesErrorObserver> errorObserver;
  reader->AddObserver(vtkCommand::ErrorEvent, errorObserver);

  conduit_node* dummyNode = nullptr;
  reader->SetDataSourceNode("source", dummyNode);

  // Validate the error macro was triggered
  if (!errorObserver->ErrorReceived)
  {
    std::cerr << "Test failed: Expected vtkFidesReader to emit an error "
              << "when updating with a Conduit node on a Conduit-less build, "
              << "but no error was caught." << std::endl;
    return EXIT_FAILURE;
  }

  // Now make sure attempting to set data source conduit node didn't
  // affect the reader's ability to process a bp file

  const char* jsonFilePath =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "/Data/vtk-uns-grid-2.json");
  const char* bpFilePath =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "/Data/tris-blocks-time.bp");
  reader->SetFileName(jsonFilePath);
  reader->SetDataSourcePath("source", bpFilePath);
  reader->UpdateInformation();

  if (!reader->GetOutputInformation(0)->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    std::cerr << "Test failed: Expected data to have timesteps" << std::endl;
    return EXIT_FAILURE;
  }

  if (reader->GetOutputInformation(0)->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) != 5)
  {
    std::cerr << "Test failed: Expected number of timesteps to equal 5" << std::endl;
    return EXIT_FAILURE;
  }

  reader->Update();

  vtkSmartPointer<vtkPartitionedDataSetCollection> pdsc =
    vtkPartitionedDataSetCollection::SafeDownCast(reader->GetOutputDataObject(0));

  if (!pdsc)
  {
    std::cerr << "Test failed: Expected number of timesteps to equal 5" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
