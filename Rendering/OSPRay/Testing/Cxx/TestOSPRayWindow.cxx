/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSPRayScalarBar.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSmartPointer.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkLight.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty2D.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTextProperty.h"
#include "vtkSphereSource.h"
#include "vtkNew.h"
#include "vtkElevationFilter.h"
#include "vtkOSPRayWindowNode.h"
#include "vtkImageActor.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkImageMapper3D.h"

#include "vtkTestUtilities.h"

int TestOSPRayWindow( int argc, char *argv[] )
{
  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(8);
  sphere->SetPhiResolution(8);
  vtkNew<vtkElevationFilter> elev;
  elev->SetInputConnection(sphere->GetOutputPort(0));

  vtkNew<vtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(elev->GetOutputPort(0));
  vtkNew<vtkActor> sphereActor;
  sphereActor->SetMapper(sphereMapper.Get());

  vtkSmartPointer<vtkLight> light1 = vtkSmartPointer<vtkLight>::New();

  // Create the RenderWindow, Renderer and all Actors
  vtkSmartPointer<vtkRenderer> ren1 =
    vtkSmartPointer<vtkRenderer>::New();
  ren1->AddLight(light1);

  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer( ren1 );

  // Add the actors to the renderer, set the background and size
  //
  ren1->AddActor( sphereActor.Get() );
  ren1->SetBackground( .2,.3,.4 );

  // render the image
  renWin->SetWindowName( "VTK - Scalar Bar options" );
  renWin->SetSize( 600, 500 );

  vtkNew<vtkOSPRayWindowNode> owindow;
  owindow->SetRenderable(renWin);
  owindow->TraverseAllPasses();

  // now get the result and display it
  int *size = owindow->GetSize();
  vtkNew<vtkImageData> image;
  image->SetDimensions(size[0], size[1], 1);
  image->GetPointData()->SetScalars(owindow->GetColorBuffer());

  // Create a new image actor and remove the geometry one
  vtkNew<vtkImageActor> imageActor;
  imageActor->GetMapper()->SetInputData(image.Get());
  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(imageActor.Get());

  // Background color white to distinguish image boundary
  renderer->SetBackground(1, 1, 1);
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer.Get());

  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow.Get());
  renderWindow->Render();
  renderer->ResetCamera();
  renderWindow->Render();
  int retVal = vtkRegressionTestImage(renderWindow.Get());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return !retVal;
}
