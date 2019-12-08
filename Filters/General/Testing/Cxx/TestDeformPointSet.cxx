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
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkDeformPointSet.h"
#include "vtkElevationFilter.h"
#include "vtkNew.h"
#include "vtkPlane.h"
#include "vtkPlaneSource.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"

int TestDeformPointSet(int argc, char* argv[])
{
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // Create a sphere to warp
  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(51);
  sphere->SetPhiResolution(17);

  // Generate some scalars on the sphere
  vtkNew<vtkElevationFilter> ele;
  ele->SetInputConnection(sphere->GetOutputPort());
  ele->SetLowPoint(0, 0, -0.5);
  ele->SetHighPoint(0, 0, 0.5);

  // Now create a control mesh, in this case a octagon
  vtkNew<vtkPoints> pts;
  pts->SetNumberOfPoints(6);
  pts->SetPoint(0, -1, 0, 0);
  pts->SetPoint(1, 1, 0, 0);
  pts->SetPoint(2, 0, -1, 0);
  pts->SetPoint(3, 0, 1, 0);
  pts->SetPoint(4, 0, 0, -1);
  pts->SetPoint(5, 0, 0, 1);

  vtkNew<vtkCellArray> tris;
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

  vtkNew<vtkPolyData> pd;
  pd->SetPoints(pts);
  pd->SetPolys(tris);

  // Display the control mesh
  vtkNew<vtkPolyDataMapper> meshMapper;
  meshMapper->SetInputData(pd);
  vtkNew<vtkActor> meshActor;
  meshActor->SetMapper(meshMapper);
  meshActor->GetProperty()->SetRepresentationToWireframe();
  meshActor->GetProperty()->SetColor(0, 0, 0);

  // Okay now let's do the initial weight generation
  vtkNew<vtkDeformPointSet> deform;
  deform->SetInputConnection(ele->GetOutputPort());
  deform->SetControlMeshData(pd);
  deform->Update(); // this creates the initial weights

  // Now move one point and deform
  pts->SetPoint(5, 0, 0, 3);
  pts->Modified();
  deform->Update();

  // Display the warped sphere
  vtkNew<vtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(deform->GetOutputPort());
  vtkNew<vtkActor> sphereActor;
  sphereActor->SetMapper(sphereMapper);

  renderer->AddActor(sphereActor);
  renderer->AddActor(meshActor);
  renderer->GetActiveCamera()->SetPosition(1, 1, 1);
  renderer->ResetCamera();

  renderer->SetBackground(1, 1, 1);
  renWin->SetSize(300, 300);
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
