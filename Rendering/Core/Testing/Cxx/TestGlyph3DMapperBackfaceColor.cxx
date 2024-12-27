// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkGlyph3DMapper.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

int TestGlyph3DMapperBackfaceColor(int argc, char* argv[])
{
  // The points to glyph:
  vtkNew<vtkPolyData> input;
  vtkNew<vtkPoints> points;
  vtkNew<vtkIntArray> indexArray;

  for (int row = 0; row < 2; ++row)
  {
    for (int col = 0; col < 3; ++col)
    {
      points->InsertNextPoint((row ? col : (2 - col)) * 5, row * 5, 0.);
    }
  }

  input->SetPoints(points);

  // The glyph sources:
  vtkNew<vtkSphereSource> source;
  source->SetStartTheta(20);
  source->SetEndTheta(330);
  source->SetRadius(2);

  vtkNew<vtkGlyph3DMapper> mapper;
  mapper->SetInputData(input);
  mapper->SetSourceConnection(source->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(1., 1., 0.);
  vtkNew<vtkProperty> backfaceProperty;
  backfaceProperty->SetColor(1.0, 0.0, 1.0);
  actor->SetBackfaceProperty(backfaceProperty);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->SetBackground(0., 0., 0.);
  renderer->GetActiveCamera()->Azimuth(40.0);
  renderer->ResetCamera();
  renderer->ResetCameraClippingRange();

  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderWindow> renWin;
  iren->SetRenderWindow(renWin);
  renWin->AddRenderer(renderer);
  renWin->SetMultiSamples(0);
  renWin->SetSize(300, 300);

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
