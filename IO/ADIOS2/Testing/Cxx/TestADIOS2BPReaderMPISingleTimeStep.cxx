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
#include "vtkCompositeRenderManager.h"
#include "vtkDataArray.h"
#include "vtkDataSetMapper.h"
#include "vtkImageData.h"
#include "vtkImageDataToPointSet.h"
#include "vtkMPIController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkUnsignedIntArray.h"
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

void TestADIOS2BPReaderMPISingleTimeStep(vtkMultiProcessController* controller, void* _args)
{
  TestArgs* args = reinterpret_cast<TestArgs*>(_args);
  int argc = args->argc;
  char** argv = args->argv;
  *(args->retval) = 1;

  int currentRank = controller->GetLocalProcessId();
  vtkNew<vtkADIOS2CoreImageReader> reader;

  // Read the input data file
  char* filePath =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ADIOS2/HeatMap3D/HeatMap3D.bp");

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
  assert(availVars.size() == 2);
  // Get the dimension
  std::string varName = availVars.begin()->first;

  reader->SetOrigin(0.0, 0.0, 0.0);
  reader->SetSpacing(1.0, 1.0, 1.0);
  reader->SetDimensionArray("temperature");

  reader->Update();

  vtkSmartPointer<vtkMultiBlockDataSet> output =
    vtkMultiBlockDataSet::SafeDownCast(reader->GetOutput());
  assert(output->GetNumberOfBlocks() == 1);
  vtkSmartPointer<vtkMultiPieceDataSet> mpds =
    vtkMultiPieceDataSet::SafeDownCast(output->GetBlock(0));
  assert(mpds->GetNumberOfPieces() == 2);
  vtkSmartPointer<vtkImageData> image0 = vtkImageData::SafeDownCast(mpds->GetPiece(0));
  vtkSmartPointer<vtkImageData> image1 = vtkImageData::SafeDownCast(mpds->GetPiece(1));

  vtkNew<vtkImageDataToPointSet> imageToPointset;
  if (currentRank == 0)
  { // Rank0 should read one block as vtkImageData into index 0
    assert(image0);
    // assert(!image1);
    assert(image0->GetCellData()->GetNumberOfArrays() == 1);
    assert(image0->GetPointData()->GetNumberOfArrays() == 1);
    image0->GetCellData()->SetActiveScalars("temperature");
    image0->GetPointData()->SetActiveScalars("temperaturePerPoint");
    imageToPointset->SetInputData(image0);
  }
  else if (currentRank == 1)
  { // Rank1 should read one block as vtkImageData into index 1
    assert(!image0);
    assert(image1);
    assert(image1->GetCellData()->GetNumberOfArrays() == 1);
    assert(image1->GetPointData()->GetNumberOfArrays() == 1);
    image1->GetCellData()->SetActiveScalars("temperature");
    image1->GetPointData()->SetActiveScalars("temperaturePerPoint");
    imageToPointset->SetInputData(image1);
  }

  imageToPointset->Update();
  // Use vtkXMLPMultiBlockDataWriter if you want to dump the data

  // Since I fail to find a proper mapper to render two vtkImageDatas inside
  // a vtkMultiPieceDataSet in a vtkMultiBlockDataSet, I render the image directly here
  vtkNew<vtkDataSetMapper> mapper;
  mapper->SetInputDataObject(imageToPointset->GetOutput());
  mapper->ScalarVisibilityOn();
  mapper->SetScalarRange(0, 2000);
  mapper->SetScalarModeToUseCellData();
  mapper->ColorByArrayComponent("temperature", 0);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->EdgeVisibilityOn();

  vtkNew<vtkCompositeRenderManager> prm;

  vtkSmartPointer<vtkRenderer> renderer;
  renderer.TakeReference(prm->MakeRenderer());
  renderer->AddActor(actor);
  renderer->SetBackground(0.5, 0.5, 0.5);
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Elevation(2000);

  vtkSmartPointer<vtkRenderWindow> rendWin;
  rendWin.TakeReference(prm->MakeRenderWindow());
  rendWin->SetSize(600, 300);
  rendWin->AddRenderer(renderer);

  prm->SetRenderWindow(rendWin);
  prm->SetController(controller);
  prm->InitializePieces();
  prm->InitializeOffScreen(); // Mesa GL only

  if (controller->GetLocalProcessId() == 0)
  {
    rendWin->Render();

    // Do the test comparsion
    int retval = vtkRegressionTestImage(rendWin);
    if (retval == vtkRegressionTester::DO_INTERACTOR)
    {
      vtkNew<vtkRenderWindowInteractor> iren;
      iren->SetRenderWindow(rendWin);
      iren->Initialize();
      iren->Start();
      retval = vtkRegressionTester::PASSED;
    }
    *(args->retval) = (retval == vtkRegressionTester::PASSED) ? 0 : 1;

    prm->StopServices();
  }
  else // not root node
  {
    prm->StartServices();
  }

  controller->Broadcast(args->retval, 1, 0);
}

int TestADIOS2BPReaderMPISingleTimeStep(int argc, char* argv[])
{
  int retval{ 1 };

  // Note that this will create a vtkMPIController if MPI
  // is configured, vtkThreadedController otherwise.
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv);

  vtkMultiProcessController::SetGlobalController(controller);

  TestArgs args;
  args.retval = &retval;
  args.argc = argc;
  args.argv = argv;

  controller->SetSingleMethod(TestADIOS2BPReaderMPISingleTimeStep, &args);
  controller->SingleMethodExecute();

  controller->Finalize();

  return retval;
}
