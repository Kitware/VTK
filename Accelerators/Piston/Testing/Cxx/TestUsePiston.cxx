/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTwo.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//This test exercises basic use Piston for accelerated processing in VTK.
//
//A simple pipeline is created in which data is sent to the GPU, processed
//there and then rendered.

#include "vtkActor.h"
#include "vtkDataSetToPiston.h"
#include "vtkDebugLeaks.h"
#include "vtkImageMandelbrotSource.h"
#include "vtkPistonContour.h"
#include "vtkPistonMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

int TestUsePiston( int argc, char *argv[] )
{
  vtkRenderer *renderer = vtkRenderer::New();
  vtkRenderWindow *renwin = vtkRenderWindow::New();
  renwin->AddRenderer(renderer);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renwin);

  // Force creation of a context that we can work with
  renwin->Render();

  // Initialize interop rendering using that context
  // TODO: Add an argument to decide if use interop or not
  vtkPistonMapper::InitCudaGL(renwin);

  vtkImageMandelbrotSource *src = vtkImageMandelbrotSource::New();
  src->SetWholeExtent(0,20,0,20,0,20);

  vtkDataSetToPiston *d2p = vtkDataSetToPiston::New();
  d2p->SetInputConnection(src->GetOutputPort());

  vtkPistonContour *contour = vtkPistonContour::New();
  contour->SetInputConnection(d2p->GetOutputPort());
  contour->SetIsoValue(50.0);

  vtkPistonMapper *mapper = vtkPistonMapper::New();
  mapper->SetInputConnection(contour->GetOutputPort());
  mapper->Update(); //TODO Why is this needed?

  vtkActor *actor = vtkActor::New();
  actor->SetMapper(mapper);

  renderer->AddActor(actor);

  // interact with data
  renderer->ResetCamera();
  renwin->Render();

  int retVal = vtkRegressionTestImage(renwin);

  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  // Clean up
  renderer->Delete();
  renwin->Delete();
  iren->Delete();
  src->Delete();
  d2p->Delete();
  contour->Delete();
  mapper->Delete();
  actor->Delete();

  return !retVal;
}
