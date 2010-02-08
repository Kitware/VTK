/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkActor.h"
#include "vtkDebugLeaks.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkClipPolyData.h"
#include "vtkElevationFilter.h"
#include "vtkPlane.h"
#include "vtkPlaneSource.h"
#include "vtkProbePolyhedron.h"

int TestMeanValueCoordinatesInterpolation( int argc, char *argv[] )
{
  vtkRenderer *renderer = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(renderer);
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

  // Create a sphere
  vtkSphereSource *sphere = vtkSphereSource::New();
    sphere->SetThetaResolution(51); sphere->SetPhiResolution(17);

  // Generate some scalars on the sphere
  vtkElevationFilter *ele = vtkElevationFilter::New();
    ele->SetInputConnection(sphere->GetOutputPort());
    ele->SetLowPoint(0,0,-0.5);
    ele->SetHighPoint(0,0,0.5);

  // Now clip the sphere in half and display it
  vtkPlane *plane = vtkPlane::New();
    plane->SetOrigin(0,0,0);
    plane->SetNormal(1,0,0);
  vtkClipPolyData *clip = vtkClipPolyData::New();
    clip->SetInputConnection(ele->GetOutputPort());
    clip->SetClipFunction(plane);
  vtkPolyDataMapper *sphereMapper = vtkPolyDataMapper::New();
    sphereMapper->SetInputConnection(clip->GetOutputPort());
  vtkActor *sphereActor = vtkActor::New();
    sphereActor->SetMapper(sphereMapper);

  //Okay now sample the sphere mesh with a plane and see how it interpolates
  vtkPlaneSource *pSource = vtkPlaneSource::New();
    pSource->SetOrigin(0,-1.0,-1.0);
    pSource->SetPoint1(0, 1.0,-1.0);
    pSource->SetPoint2(0,-1.0, 1.0);
    pSource->SetXResolution(50);
    pSource->SetYResolution(50);
  vtkProbePolyhedron *interp = 
      vtkProbePolyhedron::New();
    interp->SetInputConnection(pSource->GetOutputPort());
    interp->SetSourceConnection(ele->GetOutputPort());
  vtkPolyDataMapper *interpMapper = vtkPolyDataMapper::New();
    interpMapper->SetInputConnection(interp->GetOutputPort());
  vtkActor *interpActor = vtkActor::New();
    interpActor->SetMapper(interpMapper);

  renderer->AddActor(sphereActor);
  renderer->AddActor(interpActor);

  renderer->SetBackground(1,1,1);
  renWin->SetSize(300,300);

  // interact with data
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );

  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  // Clean up
  renderer->Delete();
  renWin->Delete();
  iren->Delete();
  plane->Delete();
  ele->Delete();
  clip->Delete();
  sphere->Delete();
  sphereMapper->Delete();
  sphereActor->Delete();
  pSource->Delete();
  interp->Delete();
  interpMapper->Delete();
  interpActor->Delete();

  return !retVal;
}
