// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Description
// Test for a custom cursor shape

#include "vtkNew.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"

int TestCustomCursorShape(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetSize(301, 300); // Intentional NPOT size

  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren.GetPointer());

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  const char* fileName = nullptr;
#if defined(_WIN32)
  fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/pen_1.cur");
#elif defined(__linux)
  fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/pen_1.xcursor");
#endif
  renWin->Render();
  renWin->SetCursorFileName(fileName);
  renWin->SetCurrentCursor(VTK_CURSOR_CUSTOM);
  renWin->Render();

  delete[] fileName;

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
