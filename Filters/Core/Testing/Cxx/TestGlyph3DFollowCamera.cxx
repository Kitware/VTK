// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
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
#include "vtkTriangle.h"

class vtkMyCameraCallback : public vtkCommand
{
public:
  static vtkMyCameraCallback* New() { return new vtkMyCameraCallback; }
  void Execute(vtkObject* caller, unsigned long, void*) override
  {
    vtkCamera* cam = reinterpret_cast<vtkCamera*>(caller);
    this->Glyph3D->SetFollowedCameraPosition(cam->GetPosition());
    this->Glyph3D->SetFollowedCameraViewUp(cam->GetViewUp());
  }
  vtkGlyph3D* Glyph3D;
};

int TestGlyph3DFollowCamera(int argc, char* argv[])
{
  // Test glyphing with glyphs facing towards the camera

  // Generate input data

  vtkNew<vtkPoints> sourcePoints;
  sourcePoints->InsertNextPoint(-0.3, 0.0, 0.0);
  sourcePoints->InsertNextPoint(0.0, 0.5, 0.0);
  sourcePoints->InsertNextPoint(0.3, 0.0, 0.0);

  vtkNew<vtkTriangle> triangle;
  triangle->GetPointIds()->SetId(0, 0);
  triangle->GetPointIds()->SetId(1, 1);
  triangle->GetPointIds()->SetId(2, 2);

  // Create a cell array to store the triangle in and add the triangle to it
  vtkNew<vtkCellArray> cells;
  cells->InsertNextCell(triangle);

  // Create a polydata to store everything in
  vtkNew<vtkPolyData> sourcePolyData;

  // Add the points to the dataset
  sourcePolyData->SetPoints(sourcePoints);

  // Add the quad to the dataset
  sourcePolyData->SetPolys(cells);

  vtkNew<vtkPoints> points;
  points->InsertNextPoint(0, 0, 0);
  points->InsertNextPoint(1, 1, 1);
  points->InsertNextPoint(2, 2, 2);
  points->InsertNextPoint(1, 2, 1);

  vtkNew<vtkPolyData> polydata;
  polydata->SetPoints(points);

  vtkNew<vtkPolyData> glyph;
  // vtkNew<vtkConeSource> glyphSource;

  // Set up glyphing

  vtkNew<vtkGlyph3D> glyph3D;
  // glyph3D->SetSourceConnection(glyphSource->GetOutputPort());
  glyph3D->SetSourceData(sourcePolyData);
  glyph3D->SetInputData(polydata);
  glyph3D->SetVectorModeToFollowCameraDirection();
  glyph3D->OrientOn();
  glyph3D->Update();

  // Set up visualization

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(glyph3D->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> ren;
  ren->SetBackground(0, 0, 0);
  ren->AddActor(actor);
  ren->ResetCamera();

  vtkNew<vtkMyCameraCallback> cameraCallback;
  cameraCallback->Glyph3D = glyph3D;
  ren->GetActiveCamera()->AddObserver(vtkCommand::ModifiedEvent, cameraCallback);
  cameraCallback->Execute(ren->GetActiveCamera(), 0, nullptr); // initial update

  vtkNew<vtkRenderWindow> renWin;

  vtkNew<vtkRenderWindowInteractor> iren;
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
