/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMonitors.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkProperty.h"
#include "vtkCamera.h"
#include "vtkLight.h"
#include "vtkOpenGLLightMonitor.h"
#include "vtkOpenGLModelViewProjectionMonitor.h"
#include "vtkBackgroundColorMonitor.h"
#include "vtkLightingHelper.h"

int TestMonitors(int argc, char* argv[])
{
  (void)argc;
  (void)argv;

  cerr << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl << endl;

  // initialize a scene with a bunch of spheres
  vtkSmartPointer<vtkSphereSource> sphere =
    vtkSmartPointer<vtkSphereSource>::New();
  sphere->SetThetaResolution(100);
  sphere->SetPhiResolution(50);

  vtkSmartPointer<vtkPolyDataMapper> sphereMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  sphereMapper->SetInputConnection(sphere->GetOutputPort());

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

  vtkSmartPointer<vtkRenderer> ren1 =
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(ren1);

  ren1->AddActor(sphere1);
  ren1->AddActor(sphere2);
  ren1->AddActor(sphere3);
  ren1->AddActor(sphere4);
  ren1->AddActor(sphere5);
  ren1->AddActor(sphere6);
  ren1->AddActor(sphere7);
  ren1->AddActor(sphere8);

  vtkSmartPointer<vtkLight> light =
    vtkSmartPointer<vtkLight>::New();
  ren1->AddLight(light);

  vtkSmartPointer<vtkLight> light2 =
    vtkSmartPointer<vtkLight>::New();
  ren1->AddLight(light2);

  renWin->SetSize(400, 200);

  // push all of VTK state to OpenGL.
  renWin->Render();

  // now create the monitors
  // for lighting
  // for gl modelview and projection matrices
  // for the background color
  // and initialize them from current state
  cerr
    << "Lights" << endl
    << "================" << endl;
  const int nLights = vtkLightingHelper::VTK_MAX_LIGHTS;
  vtkSmartPointer<vtkOpenGLLightMonitor> lightMonitor[nLights];
  for (int i=0; i<nLights; ++i)
   {
   lightMonitor[i] = vtkSmartPointer<vtkOpenGLLightMonitor>::New();
   lightMonitor[i]->SetLightId(i);
   lightMonitor[i]->Update();
   lightMonitor[i]->Print(cerr);
   }

  cerr
    << "Matrices" << endl
    << "================" << endl;

  vtkSmartPointer<vtkOpenGLModelViewProjectionMonitor> matrixMonitor
    = vtkSmartPointer<vtkOpenGLModelViewProjectionMonitor>::New();

  matrixMonitor->Update();
  matrixMonitor->Print(cerr);

  cerr
    << "Background Color" << endl
    << "================" << endl;

  vtkSmartPointer<vtkBackgroundColorMonitor> backgroundColorMonitor
    = vtkSmartPointer<vtkBackgroundColorMonitor>::New();

  backgroundColorMonitor->Update(ren1);
  backgroundColorMonitor->Print(cerr);

  // update the scene so that lights, background color
  // and modelview projection matrices are modified
  ren1->SetBackground2(0.1, 0.2, 0.4);
  ren1->SetGradientBackground(1);

  light->SetFocalPoint(1.875,0.6125,0);
  light->SetPosition(0.875,1.6125,1);

  light2->SetFocalPoint(-100,-100,-100);
  light2->SetPosition(100,100,100);

  ren1->GetActiveCamera()->SetFocalPoint(0,0,0);
  ren1->GetActiveCamera()->SetPosition(0,0,1);
  ren1->GetActiveCamera()->SetViewUp(0,1,0);
  ren1->GetActiveCamera()->ParallelProjectionOn();
  ren1->ResetCamera();
  ren1->GetActiveCamera()->SetParallelScale(1.5);

  // push all of VTK state to OpenGL.
  renWin->Render();

  // verify that we can detect the changes
  // in lighting
  // in the modelview and projectionmatrices
  // in the background color
  // all should have been updated
  cerr
    << "Lights" << endl
    << "================" << endl;
  bool lightsChanged = false;
  for (int i=0; i<nLights; ++i)
   {
   if (lightMonitor[i]->StateChanged())
     {
     cerr << "this light was changed..." << endl;
     lightsChanged = true;
     }
   lightMonitor[i]->Print(cerr);
   }

  cerr
    << "Matrices" << endl
    << "================" << endl;

  bool matricesChanged = matrixMonitor->StateChanged();
  matrixMonitor->Print(cerr);

  cerr
    << "Background Color" << endl
    << "================" << endl;

  bool colorChanged = backgroundColorMonitor->StateChanged(ren1);
  backgroundColorMonitor->Print(cerr);

  cerr
    << "Test results" << endl
    << "================" << endl
    << "detected lights changed..." << (lightsChanged?"yes":"no") << endl
    << "detected matrices changed..." << (matricesChanged?"yes":"no") << endl
    << "detected background color changed..." << (colorChanged?"yes":"no") << endl
    << endl;

  if (!(lightsChanged && matricesChanged && colorChanged))
    {
    cerr << "Test fails" << endl;
    return 1;
    }

  cerr << "Test passes" << endl;
  return 0;
}
