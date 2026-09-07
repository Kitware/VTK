// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkLineSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkStringScanner.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkElevationFilter.h"
#include "vtkGlyph3DMapper.h"
#include "vtkNew.h"
#include "vtkPlaneSource.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

int TestGlyph3DMapperLines(int argc, char* argv[])
{
  int res = 1;
  vtkNew<vtkPlaneSource> plane;
  plane->SetResolution(res, res);

  vtkNew<vtkElevationFilter> colors;
  colors->SetInputConnection(plane->GetOutputPort());
  colors->SetLowPoint(-1, -1, -1);
  colors->SetHighPoint(0.5, 0.5, 0.5);

  vtkNew<vtkLineSource> line;

  vtkNew<vtkGlyph3DMapper> glypher;
  glypher->SetInputConnection(colors->GetOutputPort());
  glypher->SetScaleFactor(0.5);
  glypher->SetSourceConnection(line->GetOutputPort());

  vtkNew<vtkActor> glyphActor1;
  glyphActor1->SetMapper(glypher);
  int lw = 5;
  for (int i = 0; i < argc; ++i)
  {
    if (!strcmp(argv[i], "-lw"))
    {
      VTK_FROM_CHARS_IF_ERROR_BREAK(argv[i + 1], lw);
    }
  }
  glyphActor1->GetProperty()->SetLineWidth(lw);
  glyphActor1->GetProperty()->SetRepresentationToWireframe();

  // Standard rendering classes
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  renWin->SetMultiSamples(0);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // set up the view
  renderer->SetBackground(0.2, 0.2, 0.2);
  renWin->SetSize(300, 300);

  renderer->AddActor(glyphActor1);

  // run the test
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Roll(45.0);
  renderer->GetActiveCamera()->Azimuth(10.0);

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
