/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestActorLightingFlag.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers the lighting flag on a vtkProperty object of an vtkActor.
// It draws a cone with lighting next to a cone with no lighting, next to a
// third cone with lighting again.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#include "vtkRenderWindowInteractor.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkActor.h"

#include "vtkPolyDataMapper.h"
#include "vtkCamera.h"

#include "vtkConeSource.h"
#include "vtkProperty.h"


// For each spotlight, add a light frustum wireframe representation and a cone
// wireframe representation, colored with the light color.
void AddLightActors(vtkRenderer *r);

int TestActorLightingFlag(int argc, char* argv[])
{
  vtkRenderWindowInteractor *iren=vtkRenderWindowInteractor::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->SetMultiSamples(0);

  renWin->SetAlphaBitPlanes(1);
  iren->SetRenderWindow(renWin);
  renWin->Delete();

  vtkRenderer *renderer = vtkRenderer::New();
  renWin->AddRenderer(renderer);
  renderer->Delete();

  vtkConeSource *coneSource1=vtkConeSource::New();
  vtkPolyDataMapper *coneMapper1=vtkPolyDataMapper::New();
  coneMapper1->SetInputConnection(coneSource1->GetOutputPort());
  coneSource1->Delete();
  vtkActor *coneActor1=vtkActor::New();
  coneActor1->SetMapper(coneMapper1);
  coneMapper1->Delete();
  coneActor1->SetPosition(-2.0,0.0,0.0);
  renderer->AddActor(coneActor1);
  coneActor1->Delete();

  vtkConeSource *coneSource2=vtkConeSource::New();
  vtkPolyDataMapper *coneMapper2=vtkPolyDataMapper::New();
  coneMapper2->SetInputConnection(coneSource2->GetOutputPort());
  coneSource2->Delete();
  vtkActor *coneActor2=vtkActor::New();
  coneActor2->SetMapper(coneMapper2);
  coneMapper2->Delete();
  coneActor2->SetPosition(0.0,0.0,0.0);
  coneActor2->GetProperty()->SetLighting(false);
  renderer->AddActor(coneActor2);
  coneActor2->Delete();

  vtkConeSource *coneSource3=vtkConeSource::New();
  vtkPolyDataMapper *coneMapper3=vtkPolyDataMapper::New();
  coneMapper3->SetInputConnection(coneSource3->GetOutputPort());
  coneSource3->Delete();
  vtkActor *coneActor3=vtkActor::New();
  coneActor3->SetMapper(coneMapper3);
  coneMapper3->Delete();
  coneActor3->SetPosition(2.0,0.0,0.0);
  renderer->AddActor(coneActor3);
  coneActor3->Delete();

  renderer->SetBackground(0.1,0.3,0.0);
  renWin->SetSize(200,200);

  renWin->Render();

  vtkCamera *camera=renderer->GetActiveCamera();
  camera->Azimuth(-40.0);
  camera->Elevation(20.0);
  renderer->ResetCamera();
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }
  iren->Delete();

  return !retVal;
}
