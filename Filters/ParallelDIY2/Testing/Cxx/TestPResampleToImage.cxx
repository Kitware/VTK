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
#include "vtkCamera.h"
#include "vtkClipDataSet.h"
#include "vtkCompositeRenderManager.h"
#include "vtkContourFilter.h"
#include "vtkPExtractVOI.h"
#include "vtkMPIController.h"
#include "vtkNew.h"
#include "vtkPieceScalars.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRTAnalyticSource.h"
#include "vtkSmartPointer.h"

#include "vtk_diy2.h"   // must include this before any diy header
VTKDIY2_PRE_INCLUDE
#include VTK_DIY2_HEADER(diy/mpi.hpp)
VTKDIY2_POST_INCLUDE


int TestPResampleToImage(int argc, char *argv[])
{
  diy::mpi::environment mpienv(argc, argv);
  vtkNew<vtkMPIController> controller;
  controller->Initialize(&argc, &argv, true);
  diy::mpi::communicator world;


  // Setup parallel rendering
  vtkNew<vtkCompositeRenderManager> prm;
  vtkSmartPointer<vtkRenderer> renderer =
    vtkSmartPointer<vtkRenderer>::Take(prm->MakeRenderer());
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::Take(prm->MakeRenderWindow());
  renWin->AddRenderer(renderer.GetPointer());
  renWin->DoubleBufferOn();
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  prm->SetRenderWindow(renWin.GetPointer());
  prm->SetController(controller.GetPointer());


  // Create Pipeline
  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(0, 31, 0, 31, 0, 31);
  wavelet->SetCenter(16, 16, 16);

  vtkNew<vtkClipDataSet> clip;
  clip->SetInputConnection(wavelet->GetOutputPort());
  clip->SetValue(157);

  vtkNew<vtkPResampleToImage> resample;
  resample->SetUseInputBounds(true);
  resample->SetSamplingDimensions(64, 64, 64);
  resample->SetInputConnection(clip->GetOutputPort());

  vtkNew<vtkPExtractVOI> voi;
  voi->SetVOI(4, 59, 4, 59, 4, 59);
  voi->SetInputConnection(resample->GetOutputPort());

  vtkNew<vtkContourFilter> contour;
  contour->SetValue(0, 200);
  contour->ComputeNormalsOn();
  contour->SetInputConnection(voi->GetOutputPort());

  vtkNew<vtkPieceScalars> pieceScalars;
  pieceScalars->SetInputConnection(contour->GetOutputPort());
  pieceScalars->SetScalarModeToCellData();


  // Execute pipeline and render
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(pieceScalars->GetOutputPort());
  mapper->SetScalarModeToUseCellFieldData();
  mapper->SelectColorArray("Piece");
  mapper->SetScalarRange(0, world.size() - 1);
  mapper->SetPiece(world.rank());
  mapper->SetNumberOfPieces(world.size());
  mapper->Update();

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper.GetPointer());
  renderer->AddActor(actor.GetPointer());

  int retVal;
  if (world.rank() == 0)
  {
    prm->ResetAllCameras();
    renderer->GetActiveCamera()->Azimuth(90);

    renWin->Render();
    retVal = vtkRegressionTester::Test(argc, argv, renWin.GetPointer(), 10);
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
