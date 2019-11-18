/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDelaunay2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This test draws a square using 4 triangles defined by 9 points and
// an edge-flag array which allows to hide internal edges.

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkUnsignedCharArray.h"

int TestEdgeFlags(int argc, char* argv[])
{
  vtkNew<vtkPoints> pts;
  pts->SetNumberOfPoints(9);
  // Define 9 points, the 4 first points of the square are repeated
  // twice because 2 edges start from them and we will have to attach an
  // edge flags to each point. The last center point is not duplicated
  // as its edge flag will always be 0 (edge hidden).
  const double pcoords[] = { 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 0.0,

    0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 0.0,

    0.5, 0.5, 0.0 };

  for (int i = 0; i < 9; i++)
  {
    pts->SetPoint(i, pcoords + 3 * i);
  }

  // Define the 4 triangles
  vtkNew<vtkCellArray> cells;
  const vtkIdType tris[] = { 0, 5, 8, 1, 6, 8, 2, 7, 8, 3, 4, 8 };
  for (int i = 0; i < 4; i++)
  {
    cells->InsertNextCell(3, tris + 3 * i);
  }

  // Set the point-edge flags in such a way that only the edges of the
  // boundary of the square are considered as edges.
  vtkNew<vtkUnsignedCharArray> edgeflags;
  edgeflags->SetName("vtkEdgeFlags");
  edgeflags->SetNumberOfComponents(1);
  edgeflags->SetNumberOfTuples(9);
  // Tip: Turn the last flag on to simulate test failure
  const unsigned char flags[] = { 1, 1, 1, 1, 0, 0, 0, 0, 0 };
  for (int i = 0; i < 9; i++)
  {
    edgeflags->SetValue(i, flags[i]);
  }

  vtkNew<vtkPolyData> pd;
  pd->SetPoints(pts);
  pd->SetPolys(cells);
  vtkPointData* pointData = pd->GetPointData();
  pointData->AddArray(edgeflags);
  pointData->SetActiveAttribute(edgeflags->GetName(), vtkDataSetAttributes::EDGEFLAG);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputData(pd);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->SetPosition(-0.75, 0, 0);
  actor->RotateZ(45.0);

  vtkProperty* property = actor->GetProperty();
  property->SetColor(1.0, 0.0, 0.0);
  property->SetRepresentationToWireframe();
  property->SetLineWidth(4.0);

  // Define the 4 triangles
  vtkNew<vtkCellArray> cells2;
  const vtkIdType polys[] = { 0, 1, 6, 8, 3 };
  cells2->InsertNextCell(5, polys);

  vtkNew<vtkPolyData> pd2;
  pd2->SetPoints(pts);
  pd2->SetPolys(cells2);
  pointData = pd2->GetPointData();
  pointData->AddArray(edgeflags);
  pointData->SetActiveAttribute(edgeflags->GetName(), vtkDataSetAttributes::EDGEFLAG);

  vtkNew<vtkPolyDataMapper> mapper2;
  mapper2->SetInputData(pd2);

  vtkNew<vtkActor> actor2;
  actor2->SetMapper(mapper2);
  actor2->SetPosition(0.75, 0, 0);
  vtkProperty* property2 = actor2->GetProperty();
  property2->SetColor(0.0, 1.0, 0.0);
  property2->SetRepresentationToWireframe();
  property2->SetLineWidth(2.0);

  // Render image
  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->AddActor(actor2);
  renderer->SetBackground(1.0, 1.0, 1.0);
  renderer->SetBackground2(0.0, 0.0, 0.0);
  renderer->GradientBackgroundOn();

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(1);
  renWin->SetSize(600, 300);
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renWin->Render();

  // Compare image
  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
