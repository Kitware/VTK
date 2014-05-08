/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkActor2D.h"
#include "vtkCellArray.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkCellArray.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkOpenGL2PolyDataMapper2D.h"
#include "vtkNew.h"
#include "vtkProperty2D.h"
#include "vtkTrivialProducer.h"

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

//----------------------------------------------------------------------------
int TestPolyDataMapper2D(int, char *[])
{
  // Initialize everything
  vtkNew<vtkActor2D> actor;
  vtkNew<vtkOpenGL2PolyDataMapper2D> mapper;
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.0, 0.0, 0.0);
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(300, 300);
  renderWindow->AddRenderer(renderer.Get());
  renderer->AddActor(actor.Get());

  // Basic polydata lines, triangles, points...
  vtkNew<vtkPolyData> polydata;
  vtkNew<vtkPoints> points;
  points->SetNumberOfPoints(7);
  points->SetPoint(0, 50, 50, 0);
  points->SetPoint(1, 100, 50, 0);
  points->SetPoint(2, 50, 100, 0);
  points->SetPoint(3, 100, 100, 0);
  points->SetPoint(4, 100, 200, 0);
  points->SetPoint(5, 140, 100, 0);
  points->SetPoint(6, 130, 50, 0);

  vtkNew<vtkCellArray> verts;
  verts->InsertNextCell(1);
  verts->InsertCellPoint(0);
  verts->InsertNextCell(1);
  verts->InsertCellPoint(1);
  verts->InsertNextCell(1);
  verts->InsertCellPoint(5);
  verts->InsertNextCell(1);
  verts->InsertCellPoint(6);

  vtkNew<vtkCellArray> lines;
  lines->InsertNextCell(2);
  lines->InsertCellPoint(2);
  lines->InsertCellPoint(3);
  lines->InsertNextCell(2);
  lines->InsertCellPoint(0);
  lines->InsertCellPoint(4);

  vtkNew<vtkCellArray> tris;
  tris->InsertNextCell(3);
  tris->InsertCellPoint(0);
  tris->InsertCellPoint(2);
  tris->InsertCellPoint(3);

  polydata->SetPoints(points.Get());
  polydata->SetVerts(verts.Get());
  polydata->SetLines(lines.Get());
  polydata->SetPolys(tris.Get());

  vtkNew<vtkTrivialProducer> prod;
  prod->SetOutput(polydata.Get());

  // Set some properties.
  mapper->SetInputConnection(prod->GetOutputPort());
  actor->SetMapper(mapper.Get());
  actor->GetProperty()->SetColor(0.5, 1.0, 0.5);
  actor->GetProperty()->SetOpacity(1.0);

  // Start.
  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow.Get());
  renderWindow->SetMultiSamples(0);
  interactor->Initialize();
  interactor->Start();

  return EXIT_SUCCESS;
}
