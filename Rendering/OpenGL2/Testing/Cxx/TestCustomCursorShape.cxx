/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCustomCursorShape.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

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
#ifdef _WIN32
  fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/pen_1.cur");
#elif __linux
  fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/pen_1.xcursor");
#endif
  renWin->Render();
  renWin->SetCursorFileName(fileName);
  renWin->SetCurrentCursor(VTK_CURSOR_CUSTOM);
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
