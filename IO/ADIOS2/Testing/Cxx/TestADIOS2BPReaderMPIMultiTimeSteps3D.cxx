/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTemporalXdmfReaderWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Description:
// This tests reading of a simple ADIOS2 bp file.

#include "vtkADIOS2CoreImageReader.h"

#include "vtkActor.h"
#include "vtkAlgorithm.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkColorTransferFunction.h"
#include "vtkCompositeRenderManager.h"
#include "vtkDataArray.h"
#include "vtkDataSetMapper.h"
#include "vtkExecutive.h"
#include "vtkImageData.h"
#include "vtkImageDataToPointSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMPIController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiBlockVolumeMapper.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedIntArray.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkXMLPMultiBlockDataWriter.h"

#include "vtkNew.h"
#include "vtkTestUtilities.h"

#include <sstream> // istringstream

struct TestArgs
{
  int* retval;
  int argc;
  char** argv;
};

void TestADIOS2BPReaderMPIMultiTimeSteps3D(vtkMultiProcessController* controller, void* _args)
{
  TestArgs* args = reinterpret_cast<TestArgs*>(_args);
  int argc = args->argc;
  char** argv = args->argv;
  *(args->retval) = 1;

  vtkNew<vtkADIOS2CoreImageReader> reader;

  // Read the input data file
  char* filePath =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ADIOS2/3D_7-point_24-step/gs.bp");

  if (!reader->CanReadFile(filePath))
  {
    std::cerr << "Cannot read file " << reader->GetFileName() << std::endl;
    return;
  }
  reader->SetFileName(filePath);
  delete[] filePath;

  reader->SetController(controller);

  reader->UpdateInformation();
  auto& availVars = reader->GetAvilableVariables();
  assert(availVars.size() == 3);
  // Get the dimension
  std::string varName = availVars.begin()->first;

  // Enable multi time stesp
  reader->SetTimeStepArray("step");
  reader->SetDimensionArray("U");
  reader->SetArrayStatus("step", false);

  reader->SetActiveScalar(std::make_pair("U", vtkADIOS2CoreImageReader::VarType::CellData));
  reader->Update();

  vtkSmartPointer<vtkMultiBlockDataSet> output =
    vtkMultiBlockDataSet::SafeDownCast(reader->GetOutput());
  assert(output->GetNumberOfBlocks() == 1);
  vtkSmartPointer<vtkMultiPieceDataSet> mpds =
    vtkMultiPieceDataSet::SafeDownCast(output->GetBlock(0));
  assert(mpds->GetNumberOfPieces() == 6);
  vtkSmartPointer<vtkImageData> image0 = vtkImageData::SafeDownCast(mpds->GetPiece(0));
  vtkSmartPointer<vtkImageData> image1 = vtkImageData::SafeDownCast(mpds->GetPiece(1));

  // Use vtkXMLPMultiBlockDataWriter + vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()
  // to write out the data if needed

  *(args->retval) = 0;
  controller->Broadcast(args->retval, 1, 0);
}

int TestADIOS2BPReaderMPIMultiTimeSteps3D(int argc, char* argv[])
{
  int retval{ 0 };

  // Note that this will create a vtkMPIController if MPI
  // is configured, vtkThreadedController otherwise.
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv);

  vtkMultiProcessController::SetGlobalController(controller);

  TestArgs args;
  args.retval = &retval;
  args.argc = argc;
  args.argv = argv;

  controller->SetSingleMethod(TestADIOS2BPReaderMPIMultiTimeSteps3D, &args);
  controller->SingleMethodExecute();

  controller->Finalize();

  return retval;
}
