/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMeanValueCoordinatesInterpolation.cxx

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
#include "vtkLightCollection.h"
#include "vtkLight.h"
#include "vtkProperty.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkSmartPointer.h"

int TestMeanValueCoordinatesInterpolation1( int argc, char *argv[] )
{
  vtkSmartPointer<vtkRenderer> renderer = 
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderer> renderer1 = 
    vtkSmartPointer<vtkRenderer>::New();
  renderer->SetViewport(0, 0, 0.5, 1);
  
  vtkSmartPointer<vtkRenderWindow> renWin = 
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(renderer);
  renWin->AddRenderer(renderer1);
  renderer1->SetViewport(0.5, 0, 1, 1);
  
  vtkSmartPointer<vtkRenderWindowInteractor> iren = 
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  //
  // Case 0: triangle meshes
  //
  // Create a sphere
  vtkSmartPointer<vtkSphereSource> sphere = 
    vtkSmartPointer<vtkSphereSource>::New();
  sphere->SetThetaResolution(51); 
  sphere->SetPhiResolution(17);
  
  // Generate some scalars on the sphere
  vtkSmartPointer<vtkElevationFilter> ele = 
    vtkSmartPointer<vtkElevationFilter>::New();
  ele->SetInputConnection(sphere->GetOutputPort());
  ele->SetLowPoint(-0.5,0,0);
  ele->SetHighPoint(0.5,0,0);
  ele->Update();
  
  // Now clip the sphere in half and display it
  vtkSmartPointer<vtkPlane> plane = vtkSmartPointer<vtkPlane>::New();
  plane->SetOrigin(0,0,0);
  plane->SetNormal(0,0,1);
  
  vtkSmartPointer<vtkClipPolyData> clip = 
    vtkSmartPointer<vtkClipPolyData>::New();
  clip->SetInputConnection(ele->GetOutputPort());
  clip->SetClipFunction(plane);
  
  vtkSmartPointer<vtkPolyDataMapper> sphereMapper = 
    vtkSmartPointer<vtkPolyDataMapper>::New();
  sphereMapper->SetInputConnection(clip->GetOutputPort());
  
  vtkSmartPointer<vtkActor> sphereActor = vtkSmartPointer<vtkActor>::New();
  sphereActor->SetMapper(sphereMapper);

  //Okay now sample the sphere mesh with a plane and see how it interpolates
  vtkSmartPointer<vtkPlaneSource> pSource = 
    vtkSmartPointer<vtkPlaneSource>::New();
  pSource->SetOrigin(-1.0,-1.0,0);
  pSource->SetPoint1(1.0,-1.0,0);
  pSource->SetPoint2(-1.0, 1.0,0);
  pSource->SetXResolution(50);
  pSource->SetYResolution(50);

  // interpolation 0: use the faster MVC algorithm specialized for triangle meshes.
  vtkSmartPointer<vtkProbePolyhedron> interp = 
    vtkSmartPointer<vtkProbePolyhedron>::New();
  interp->SetInputConnection(pSource->GetOutputPort());
  interp->SetSourceConnection(ele->GetOutputPort());

  vtkSmartPointer<vtkPolyDataMapper> interpMapper = 
    vtkSmartPointer<vtkPolyDataMapper>::New();
  interpMapper->SetInputConnection(interp->GetOutputPort());
  
  vtkSmartPointer<vtkActor> interpActor = vtkSmartPointer<vtkActor>::New();
  interpActor->SetMapper(interpMapper);

  //
  // Case 1: general meshes
  //
  // Create a sphere
  vtkSmartPointer<vtkSphereSource> sphere1 = 
    vtkSmartPointer<vtkSphereSource>::New();
  sphere1->SetThetaResolution(51); 
  sphere1->SetPhiResolution(17);
  
  // Generate some scalars on the sphere
  vtkSmartPointer<vtkElevationFilter> ele1 = 
    vtkSmartPointer<vtkElevationFilter>::New();
  ele1->SetInputConnection(sphere1->GetOutputPort());
  ele1->SetLowPoint(-0.5,0,0);
  ele1->SetHighPoint(0.5,0,0);
  ele1->Update();
  
  // create a cell with 4 points
  vtkPolyData* spherePoly = vtkPolyData::SafeDownCast(ele1->GetOutput());
  vtkCellArray *polys = spherePoly->GetPolys();

  // merge the first two cell, this will make vtkProbePolyhedron select the
  // more general MVC algorithm.
  vtkIdType * p = polys->GetPointer();
  vtkIdType pids[4] = {p[1], p[2], p[6], p[3]};
  
  vtkSmartPointer<vtkCellArray> newPolys = vtkSmartPointer<vtkCellArray>::New();
  newPolys->SetNumberOfCells(polys->GetNumberOfCells()-1);
  newPolys->Initialize();
  for (int i = 2; i < polys->GetNumberOfCells(); i++)
    {
    pids[0] = p[4*i+1];
    pids[1] = p[4*i+2];
    pids[2] = p[4*i+3];
    newPolys->InsertNextCell(3,pids);
    }
  spherePoly->SetPolys(newPolys);
  
  // Now clip the sphere in half and display it
  vtkSmartPointer<vtkPlane> plane1 = vtkSmartPointer<vtkPlane>::New();
  plane1->SetOrigin(0,0,0);
  plane1->SetNormal(0,0,1);
  
  vtkSmartPointer<vtkClipPolyData> clip1 = 
    vtkSmartPointer<vtkClipPolyData>::New();
  clip1->SetInput(spherePoly);
  clip1->SetClipFunction(plane1);
  
  vtkSmartPointer<vtkPolyDataMapper> sphereMapper1 = 
    vtkSmartPointer<vtkPolyDataMapper>::New();
  sphereMapper1->SetInputConnection(clip1->GetOutputPort());
  
  vtkSmartPointer<vtkActor> sphereActor1 = vtkSmartPointer<vtkActor>::New();
  sphereActor1->SetMapper(sphereMapper1);

  //Okay now sample the sphere mesh with a plane and see how it interpolates
  vtkSmartPointer<vtkPlaneSource> pSource1 = 
    vtkSmartPointer<vtkPlaneSource>::New();
  pSource1->SetOrigin(-1.0,-1.0,0);
  pSource1->SetPoint1(1.0,-1.0,0);
  pSource1->SetPoint2(-1.0, 1.0,0);
  pSource1->SetXResolution(50);
  pSource1->SetYResolution(50);

  // interpolation 1: use the more general but slower MVC algorithm.
  vtkSmartPointer<vtkProbePolyhedron> interp1 = 
    vtkSmartPointer<vtkProbePolyhedron>::New();
  interp1->SetInputConnection(pSource1->GetOutputPort());
  interp1->SetSourceConnection(ele1->GetOutputPort());

  vtkSmartPointer<vtkPolyDataMapper> interpMapper1 = 
    vtkSmartPointer<vtkPolyDataMapper>::New();
  interpMapper1->SetInputConnection(interp1->GetOutputPort());
  
  vtkSmartPointer<vtkActor> interpActor1 = vtkSmartPointer<vtkActor>::New();
  interpActor1->SetMapper(interpMapper1);

  //
  // add actors to renderer
  //
  vtkSmartPointer<vtkProperty> lightProperty = 
    vtkSmartPointer<vtkProperty>::New();
  lightProperty->LightingOff();
  sphereActor->SetProperty(lightProperty);
  interpActor->SetProperty(lightProperty); 
  interpActor1->SetProperty(lightProperty); 
  
  renderer->AddActor(sphereActor);
  renderer->AddActor(interpActor);
  renderer->ResetCamera();
  renderer->SetBackground(1,1,1);

  renderer1->AddActor(sphereActor);
  renderer1->AddActor(interpActor1);
  renderer1->ResetCamera();
  renderer1->SetBackground(1,1,1);
  
  renWin->SetSize(600,300);

  // interact with data
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );

  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  return !retVal;
}
