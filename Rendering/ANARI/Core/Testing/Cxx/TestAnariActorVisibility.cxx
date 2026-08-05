// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPLYReader.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataNormals.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"

#include "vtkAnariSceneGraph.h"
#include "vtkAnariTestUtilities.h"

/**
 * This test verifies that we can hot swap ANARI and GL backends.
 */
int TestAnariActorVisibility(int argc, char* argv[])
{
  bool useDebugDevice = false;

  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "--trace"))
    {
      useDebugDevice = true;
    }
  }

  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> iren;
  renWin->SetInteractor(iren);

  const char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/bunny.ply");
  vtkNew<vtkPLYReader> polysource;
  polysource->SetFileName(fileName);

  vtkNew<vtkPolyDataNormals> normals;
  normals->SetInputConnection(polysource->GetOutputPort());

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(normals->GetOutputPort());
  vtkNew<vtkActor> actor;
  renderer->AddActor(actor);
  actor->SetMapper(mapper);
  auto prop = actor->GetProperty();
  prop->SetMaterialName("matte");
  prop->SetDiffuseColor(1.0, 1.0, 1.0);
  renderer->SetBackground(0.0, 0.0, 0.5);
  renWin->SetSize(400, 400);

  vtkAnariTestUtilities::SetParameterDefaults(renWin, useDebugDevice, "TestAnariPassVisibility");

  for (int i = 1; i < 3; i++)
  {
    if (i % 2)
    {
      vtkLogF(INFO, "Render visible");
      actor->SetVisibility(true);
    }
    else
    {
      vtkLogF(INFO, "Render invisible");
      actor->SetVisibility(false);
    }
    renWin->Render();
  }

  int retVal = vtkRegressionTestImage(renWin);

  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
