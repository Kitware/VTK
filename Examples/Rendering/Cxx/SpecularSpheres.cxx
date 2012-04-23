/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SpecularSpheres.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This examples demonstrates the effect of specular lighting.
//
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkProperty.h"
#include "vtkCamera.h"
#include "vtkLight.h"

int main()
{
  // The following lines create a sphere represented by polygons.
  //
  vtkSmartPointer<vtkSphereSource> sphere =
    vtkSmartPointer<vtkSphereSource>::New();
  sphere->SetThetaResolution(100);
  sphere->SetPhiResolution(50);

  // The mapper is responsible for pushing the geometry into the graphics
  // library. It may also do color mapping, if scalars or other attributes
  // are defined.
  //
  vtkSmartPointer<vtkPolyDataMapper> sphereMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  sphereMapper->SetInputConnection(sphere->GetOutputPort());

  // The actor is a grouping mechanism: besides the geometry (mapper), it
  // also has a property, transformation matrix, and/or texture map.
  // In this example we create eight different spheres (two rows of four
  // spheres) and set the specular lighting coefficients. A little ambient
  // is turned on so the sphere is not completely black on the back side.
  //
  vtkSmartPointer<vtkActor> sphere1 =
    vtkSmartPointer<vtkActor>::New();
  sphere1->SetMapper(sphereMapper);
  sphere1->GetProperty()->SetColor(1,0,0);
  sphere1->GetProperty()->SetAmbient(0.3);
  sphere1->GetProperty()->SetDiffuse(0.0);
  sphere1->GetProperty()->SetSpecular(1.0);
  sphere1->GetProperty()->SetSpecularPower(5.0);

  vtkSmartPointer<vtkActor> sphere2 =
    vtkSmartPointer<vtkActor>::New();
  sphere2->SetMapper(sphereMapper);
  sphere2->GetProperty()->SetColor(1,0,0);
  sphere2->GetProperty()->SetAmbient(0.3);
  sphere2->GetProperty()->SetDiffuse(0.0);
  sphere2->GetProperty()->SetSpecular(1.0);
  sphere2->GetProperty()->SetSpecularPower(10.0);
  sphere2->AddPosition(1.25,0,0);

  vtkSmartPointer<vtkActor> sphere3 =
    vtkSmartPointer<vtkActor>::New();
  sphere3->SetMapper(sphereMapper);
  sphere3->GetProperty()->SetColor(1,0,0);
  sphere3->GetProperty()->SetAmbient(0.3);
  sphere3->GetProperty()->SetDiffuse(0.0);
  sphere3->GetProperty()->SetSpecular(1.0);
  sphere3->GetProperty()->SetSpecularPower(20.0);
  sphere3->AddPosition(2.5,0,0);

  vtkSmartPointer<vtkActor> sphere4 =
    vtkSmartPointer<vtkActor>::New();
  sphere4->SetMapper(sphereMapper);
  sphere4->GetProperty()->SetColor(1,0,0);
  sphere4->GetProperty()->SetAmbient(0.3);
  sphere4->GetProperty()->SetDiffuse(0.0);
  sphere4->GetProperty()->SetSpecular(1.0);
  sphere4->GetProperty()->SetSpecularPower(40.0);
  sphere4->AddPosition(3.75,0,0);

  vtkSmartPointer<vtkActor> sphere5 =
    vtkSmartPointer<vtkActor>::New();
  sphere5->SetMapper(sphereMapper);
  sphere5->GetProperty()->SetColor(1,0,0);
  sphere5->GetProperty()->SetAmbient(0.3);
  sphere5->GetProperty()->SetDiffuse(0.0);
  sphere5->GetProperty()->SetSpecular(0.5);
  sphere5->GetProperty()->SetSpecularPower(5.0);
  sphere5->AddPosition(0.0,1.25,0);

  vtkSmartPointer<vtkActor> sphere6 =
    vtkSmartPointer<vtkActor>::New();
  sphere6->SetMapper(sphereMapper);
  sphere6->GetProperty()->SetColor(1,0,0);
  sphere6->GetProperty()->SetAmbient(0.3);
  sphere6->GetProperty()->SetDiffuse(0.0);
  sphere6->GetProperty()->SetSpecular(0.5);
  sphere6->GetProperty()->SetSpecularPower(10.0);
  sphere6->AddPosition(1.25,1.25,0);

  vtkSmartPointer<vtkActor> sphere7 =
    vtkSmartPointer<vtkActor>::New();
  sphere7->SetMapper(sphereMapper);
  sphere7->GetProperty()->SetColor(1,0,0);
  sphere7->GetProperty()->SetAmbient(0.3);
  sphere7->GetProperty()->SetDiffuse(0.0);
  sphere7->GetProperty()->SetSpecular(0.5);
  sphere7->GetProperty()->SetSpecularPower(20.0);
  sphere7->AddPosition(2.5,1.25,0);

  vtkSmartPointer<vtkActor> sphere8 =
    vtkSmartPointer<vtkActor>::New();
  sphere8->SetMapper(sphereMapper);
  sphere8->GetProperty()->SetColor(1,0,0);
  sphere8->GetProperty()->SetAmbient(0.3);
  sphere8->GetProperty()->SetDiffuse(0.0);
  sphere8->GetProperty()->SetSpecular(0.5);
  sphere8->GetProperty()->SetSpecularPower(40.0);
  sphere8->AddPosition(3.75,1.25,0);

  // Create the graphics structure. The renderer renders into the
  // render window. The render window interactor captures mouse events
  // and will perform appropriate camera or actor manipulation
  // depending on the nature of the events.
  //
  vtkSmartPointer<vtkRenderer> ren1 =
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(ren1);
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // Add the actors to the renderer, set the background and size.
  //
  ren1->AddActor(sphere1);
  ren1->AddActor(sphere2);
  ren1->AddActor(sphere3);
  ren1->AddActor(sphere4);
  ren1->AddActor(sphere5);
  ren1->AddActor(sphere6);
  ren1->AddActor(sphere7);
  ren1->AddActor(sphere8);
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(400, 200);

  // Set up the lighting.
  //
  vtkSmartPointer<vtkLight> light =
    vtkSmartPointer<vtkLight>::New();
  light->SetFocalPoint(1.875,0.6125,0);
  light->SetPosition(0.875,1.6125,1);
  ren1->AddLight(light);

  // We want to eliminate perspective effects on the apparent lighting.
  // Parallel camera projection will be used. To zoom in parallel projection
  // mode, the ParallelScale is set.
  //
  ren1->GetActiveCamera()->SetFocalPoint(0,0,0);
  ren1->GetActiveCamera()->SetPosition(0,0,1);
  ren1->GetActiveCamera()->SetViewUp(0,1,0);
  ren1->GetActiveCamera()->ParallelProjectionOn();
  ren1->ResetCamera();
  ren1->GetActiveCamera()->SetParallelScale(1.5);

  // This starts the event loop and invokes an initial render.
  //
  iren->Initialize();
  iren->Start();

  return EXIT_SUCCESS;
}




