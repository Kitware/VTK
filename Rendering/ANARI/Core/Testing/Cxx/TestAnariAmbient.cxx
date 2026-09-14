// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkLight.h"
#include "vtkPLYReader.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataNormals.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"

#include "vtkAnariRenderWindow.h"
#include "vtkAnariRenderer.h"
#include "vtkAnariTestUtilities.h"

#include <stdlib.h>

/**
 * This test verifies that ambient lights take effect with ANARI.
 */
int TestAnariAmbient(int argc, char* argv[])
{
  bool useDebugDevice = false;

  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "--trace"))
    {
      useDebugDevice = true;
    }
  }

  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderWindow> renWin;
  iren->SetRenderWindow(renWin);
  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  renWin->AddRenderer(renderer);

  vtkAnariTestUtilities::SetParameterDefaults(renWin, useDebugDevice, "TestAnariAmbient");

  // Bunny data
  const char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/bunny.ply");
  vtkSmartPointer<vtkPLYReader> polysource = vtkSmartPointer<vtkPLYReader>::New();
  polysource->SetFileName(fileName);

  vtkSmartPointer<vtkPolyDataNormals> normals = vtkSmartPointer<vtkPolyDataNormals>::New();
  normals->SetInputConnection(polysource->GetOutputPort());

  vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(normals->GetOutputPort());
  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  renderer->AddActor(actor);
  actor->SetMapper(mapper);
  renWin->SetSize(400, 400);

  auto* ar = vtkAnariRenderWindow::SafeDownCast(renWin)->GetAnariRenderer();
  for (double i = 0.; i < 3.14; i += 0.1)
  {
    ar->SetParameterf("ambientRadiance", float(sin(i)));
    renWin->Render();
  }

  ar->SetParameterf("ambientRadiance", 0.2f);
  renWin->Render();

  int retVal = vtkTesting::PASSED;

  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
