/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPResampleToImage.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPResampleToImage.h"

#include "vtkActor.h"
#include "vtkAssignAttribute.h"
#include "vtkCamera.h"
#include "vtkCompositeRenderManager.h"
#include "vtkContourFilter.h"
#include "vtkExtentTranslator.h"
#include "vtkImageData.h"
#include "vtkMPIController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPointDataToCellData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"

// clang-format off
#include "vtk_diy2.h" // must include this before any diy header
#include VTK_DIY2(diy/mpi.hpp)
// clang-format on

int TestPResampleToImageCompositeDataSet(int argc, char* argv[])
{
  diy::mpi::environment mpienv(argc, argv);
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv, true);
  diy::mpi::communicator world;

  // Setup parallel rendering
  vtkNew<vtkCompositeRenderManager> prm;
  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::Take(prm->MakeRenderer());
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::Take(prm->MakeRenderWindow());
  renWin->AddRenderer(renderer);
  renWin->DoubleBufferOn();
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  prm->SetRenderWindow(renWin);
  prm->SetController(controller);

  // Create input dataset
  const int piecesPerRank = 2;
  int numberOfPieces = world.size() * piecesPerRank;

  vtkNew<vtkMultiBlockDataSet> input;
  input->SetNumberOfBlocks(numberOfPieces);

  vtkNew<vtkExtentTranslator> extentTranslator;
  extentTranslator->SetWholeExtent(0, 31, 0, 31, 0, 31);
  extentTranslator->SetNumberOfPieces(numberOfPieces);
  extentTranslator->SetSplitModeToBlock();

  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(0, 31, 0, 31, 0, 31);
  wavelet->SetCenter(16, 16, 16);
  vtkNew<vtkPointDataToCellData> pointToCell;
  pointToCell->SetInputConnection(wavelet->GetOutputPort());

  for (int i = 0; i < piecesPerRank; ++i)
  {
    int piece = (world.rank() * piecesPerRank) + i;
    int pieceExtent[6];
    extentTranslator->SetPiece(piece);
    extentTranslator->PieceToExtent();
    extentTranslator->GetExtent(pieceExtent);
    pointToCell->UpdateExtent(pieceExtent);
    vtkNew<vtkImageData> img;
    img->DeepCopy(vtkImageData::SafeDownCast(pointToCell->GetOutput()));
    input->SetBlock(piece, img);
  }

  // create pipeline
  vtkNew<vtkPResampleToImage> resample;
  resample->SetInputDataObject(input);
  resample->SetController(controller);
  resample->SetUseInputBounds(true);
  resample->SetSamplingDimensions(64, 64, 64);

  vtkNew<vtkAssignAttribute> assignAttrib;
  assignAttrib->SetInputConnection(resample->GetOutputPort());
  assignAttrib->Assign("RTData", vtkDataSetAttributes::SCALARS, vtkAssignAttribute::POINT_DATA);

  vtkNew<vtkContourFilter> contour;
  contour->SetInputConnection(assignAttrib->GetOutputPort());
  contour->SetValue(0, 157);
  contour->ComputeNormalsOn();

  // Execute pipeline and render
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(contour->GetOutputPort());
  mapper->Update();

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  int retVal;
  if (world.rank() == 0)
  {
    prm->ResetAllCameras();
    renWin->Render();
    retVal = vtkRegressionTester::Test(argc, argv, renWin, 10);
    if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
      prm->StartInteractor();
    }
    controller->TriggerBreakRMIs();
  }
  else
  {
    prm->StartServices();
  }
  world.barrier();

  diy::mpi::broadcast(world, retVal, 0);

  controller->Finalize(true);

  return !retVal;
}
