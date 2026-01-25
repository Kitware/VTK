// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkMinimalStandardRandomSequence.h"
#include "vtkNew.h"
#include "vtkPointSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStringScanner.h"
#include "vtkTesting.h"

int TestPointRendering(int argc, char* argv[])
{
  double pointSize = 1.0;
  bool drawRoundPoints = false;
  for (int i = 0; i < argc; i++)
  {
    if (std::string(argv[i]) == "--point-size")
    {
      VTK_FROM_CHARS_IF_ERROR_RETURN(argv[++i], pointSize, EXIT_FAILURE);
    }
    if (std::string(argv[i]) == "--round")
    {
      drawRoundPoints = true;
    }
  }
  vtkNew<vtkRenderWindow> renWin;
  if (renWin->IsA("vtkOpenGLRenderWindow") && drawRoundPoints)
  {
    // Round points are not supported by the OpenGL mapper
    return VTK_SKIP_RETURN_CODE;
  }
  renWin->SetWindowName(__func__);
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer);

  vtkNew<vtkPointSource> points;
  vtkNew<vtkMinimalStandardRandomSequence> randomSequence;
  randomSequence->SetSeed(1);
  points->SetRandomSequence(randomSequence);
  points->SetRadius(1);
  points->SetNumberOfPoints(100);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(points->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->GetProperty()->SetPointSize(pointSize);
  if (drawRoundPoints)
  {
    actor->GetProperty()->SetPoint2DShape(vtkProperty::Point2DShapeType::Round);
  }
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  renderer->ResetCamera();

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);
  style->SetDefaultRenderer(renderer);

  renWin->Render();

  const int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
