/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestExtractSelection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkConeSource.h"
#include "vtkDoubleArray.h"
#include "vtkExecutive.h"
#include "vtkGlyph3D.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTestErrorObserver.h"
#include "vtkTestUtilities.h"

static bool TestGlyph3D_WithBadArray()
{
  vtkSmartPointer<vtkDoubleArray> vectors = vtkSmartPointer<vtkDoubleArray>::New();
  vectors->SetName("Normals");
  vectors->SetNumberOfComponents(4);
  vectors->InsertNextTuple4(1, 1, 1, 1);

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->InsertNextPoint(0, 0, 0);
  vtkSmartPointer<vtkPolyData> polydata = vtkSmartPointer<vtkPolyData>::New();
  polydata->SetPoints(points);

  polydata->GetPointData()->AddArray(vectors);

  vtkSmartPointer<vtkPolyData> glyph = vtkSmartPointer<vtkPolyData>::New();

  vtkSmartPointer<vtkConeSource> glyphSource = vtkSmartPointer<vtkConeSource>::New();

  vtkSmartPointer<vtkGlyph3D> glyph3D = vtkSmartPointer<vtkGlyph3D>::New();
  glyph3D->SetSourceConnection(glyphSource->GetOutputPort());
  glyph3D->SetInputData(polydata);
  glyph3D->SetInputArrayToProcess(1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Normals");
  glyph3D->SetVectorModeToUseVector();
  vtkSmartPointer<vtkTest::ErrorObserver> errorObserver1 =
    vtkSmartPointer<vtkTest::ErrorObserver>::New();
  vtkSmartPointer<vtkTest::ErrorObserver> errorObserver2 =
    vtkSmartPointer<vtkTest::ErrorObserver>::New();
  glyph3D->AddObserver(vtkCommand::ErrorEvent, errorObserver1);
  glyph3D->GetExecutive()->AddObserver(vtkCommand::ErrorEvent, errorObserver2);
  glyph3D->Update();
  int status = errorObserver1->CheckErrorMessage("vtkDataArray Normals has more than 3 components");
  status += errorObserver2->CheckErrorMessage("Algorithm vtkGlyph3D");
  return true;
}

static bool TestGlyph3D_WithoutSource()
{
  vtkNew<vtkPoints> points;
  points->InsertNextPoint(0, 0, 0);
  vtkNew<vtkPolyData> polydata;
  polydata->SetPoints(points);

  vtkNew<vtkGlyph3D> glyph3D;
  glyph3D->SetInputData(polydata);
  glyph3D->Update();

  return true;
}

int TestGlyph3D(int argc, char* argv[])
{
  if (!TestGlyph3D_WithBadArray())
  {
    return EXIT_FAILURE;
  }

  if (!TestGlyph3D_WithoutSource())
  {
    return EXIT_FAILURE;
  }

  vtkSmartPointer<vtkDoubleArray> vectors = vtkSmartPointer<vtkDoubleArray>::New();
  vectors->SetName("Normals");
  vectors->SetNumberOfComponents(2);
  vectors->InsertNextTuple2(1, 1);
  vectors->InsertNextTuple2(1, 0);
  vectors->InsertNextTuple2(0, 1);

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->InsertNextPoint(0, 0, 0);
  points->InsertNextPoint(1, 1, 1);
  points->InsertNextPoint(2, 2, 2);

  vtkSmartPointer<vtkPolyData> polydata = vtkSmartPointer<vtkPolyData>::New();
  polydata->SetPoints(points);

  polydata->GetPointData()->AddArray(vectors);

  vtkSmartPointer<vtkPolyData> glyph = vtkSmartPointer<vtkPolyData>::New();

  vtkSmartPointer<vtkConeSource> glyphSource = vtkSmartPointer<vtkConeSource>::New();

  vtkSmartPointer<vtkGlyph3D> glyph3D = vtkSmartPointer<vtkGlyph3D>::New();
  glyph3D->SetSourceConnection(glyphSource->GetOutputPort());
  glyph3D->SetInputData(polydata);
  glyph3D->SetInputArrayToProcess(1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Normals");
  glyph3D->SetVectorModeToUseVector();
  glyph3D->Update();

  // Visualize

  vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(glyph3D->GetOutputPort());

  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);

  vtkSmartPointer<vtkRenderer> ren = vtkSmartPointer<vtkRenderer>::New();
  ren->SetBackground(0, 0, 0);
  ren->AddActor(actor);
  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(1.5);

  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();

  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  renWin->AddRenderer(ren);
  renWin->SetSize(300, 300);
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
