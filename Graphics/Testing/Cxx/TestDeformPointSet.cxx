/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDeformPointSet.cxx

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
#include "vtkElevationFilter.h"
#include "vtkPlane.h"
#include "vtkPlaneSource.h"
#include "vtkProperty.h"
#include "vtkSmartPointer.h"
#include "vtkCellArray.h"
#include "vtkPoints.h"
#include "vtkDeformPointSet.h"
#include "vtkCamera.h"

int TestDeformPointSet( int argc, char *argv[] )
{
  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin = 
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(renderer);
  vtkSmartPointer<vtkRenderWindowInteractor> iren = 
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // Create a sphere to warp
  vtkSmartPointer<vtkSphereSource> sphere = vtkSmartPointer<vtkSphereSource>::New();
    sphere->SetThetaResolution(51); sphere->SetPhiResolution(17);

  // Generate some scalars on the sphere
  vtkSmartPointer<vtkElevationFilter> ele = vtkSmartPointer<vtkElevationFilter>::New();
    ele->SetInputConnection(sphere->GetOutputPort());
    ele->SetLowPoint(0,0,-0.5);
    ele->SetHighPoint(0,0,0.5);

  // Now create a control mesh, in this case a octagon
  vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
  pts->SetNumberOfPoints(6);
  pts->SetPoint(0, -1, 0, 0);
  pts->SetPoint(1,  1, 0, 0);
  pts->SetPoint(2,  0,-1, 0);
  pts->SetPoint(3,  0, 1, 0);
  pts->SetPoint(4,  0, 0,-1);
  pts->SetPoint(5,  0, 0, 1);
  
  vtkSmartPointer<vtkCellArray> tris = vtkSmartPointer<vtkCellArray>::New();
  tris->InsertNextCell(3);
  tris->InsertCellPoint(2);
  tris->InsertCellPoint(0);
  tris->InsertCellPoint(4);
  tris->InsertNextCell(3);
  tris->InsertCellPoint(1);
  tris->InsertCellPoint(2);
  tris->InsertCellPoint(4);
  tris->InsertNextCell(3);
  tris->InsertCellPoint(3);
  tris->InsertCellPoint(1);
  tris->InsertCellPoint(4);
  tris->InsertNextCell(3);
  tris->InsertCellPoint(0);
  tris->InsertCellPoint(3);
  tris->InsertCellPoint(4);
  tris->InsertNextCell(3);
  tris->InsertCellPoint(0);
  tris->InsertCellPoint(2);
  tris->InsertCellPoint(5);
  tris->InsertNextCell(3);
  tris->InsertCellPoint(2);
  tris->InsertCellPoint(1);
  tris->InsertCellPoint(5);
  tris->InsertNextCell(3);
  tris->InsertCellPoint(1);
  tris->InsertCellPoint(3);
  tris->InsertCellPoint(5);
  tris->InsertNextCell(3);
  tris->InsertCellPoint(3);
  tris->InsertCellPoint(0);
  tris->InsertCellPoint(5);
  
  vtkSmartPointer<vtkPolyData> pd = vtkSmartPointer<vtkPolyData>::New();
  pd->SetPoints(pts);
  pd->SetPolys(tris);
  
  // Display the control mesh
  vtkSmartPointer<vtkPolyDataMapper> meshMapper = 
    vtkSmartPointer<vtkPolyDataMapper>::New();
    meshMapper->SetInput(pd);
  vtkSmartPointer<vtkActor> meshActor = vtkSmartPointer<vtkActor>::New();
    meshActor->SetMapper(meshMapper);
    meshActor->GetProperty()->SetRepresentationToWireframe();
    meshActor->GetProperty()->SetColor(0,0,0);

  // Okay now let's do the intitial weight generation
  vtkSmartPointer<vtkDeformPointSet> deform = vtkSmartPointer<vtkDeformPointSet>::New();
  deform->SetInputConnection(ele->GetOutputPort());
  deform->SetControlMesh(pd);
  deform->Update(); //this creates the initial weights
  
  // Now move one point and deform
  pts->SetPoint(5, 0,0,3);
  pts->Modified();
  deform->Update();

  // Display the warped sphere
  vtkSmartPointer<vtkPolyDataMapper> sphereMapper = 
    vtkSmartPointer<vtkPolyDataMapper>::New();
    sphereMapper->SetInputConnection(deform->GetOutputPort());
  vtkSmartPointer<vtkActor> sphereActor = vtkSmartPointer<vtkActor>::New();
    sphereActor->SetMapper(sphereMapper);

  renderer->AddActor(sphereActor);
  renderer->AddActor(meshActor);
  renderer->GetActiveCamera()->SetPosition(1,1,1);
  renderer->ResetCamera();
  
  renderer->SetBackground(1,1,1);
  renWin->SetSize(300,300);

  // interact with data
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );

  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }


  return !retVal;
}
