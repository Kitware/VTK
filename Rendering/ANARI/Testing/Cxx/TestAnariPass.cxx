// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This test verifies that we can hot swap ANARI and GL backends.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
//              In interactive mode it responds to the keys listed
//              vtkAnariTestInteractor.h

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPLYReader.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataNormals.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"

#include "vtkAnariPass.h"
#include "vtkAnariSceneGraph.h"
#include "vtkAnariTestInteractor.h"
#include "vtkAnariTestUtilities.h"

int TestAnariPass(int argc, char* argv[])
{
  vtkLogger::SetStderrVerbosity(vtkLogger::Verbosity::VERBOSITY_WARNING);
  bool useDebugDevice = false;

  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "-trace"))
    {
      useDebugDevice = true;
      vtkLogger::SetStderrVerbosity(vtkLogger::Verbosity::VERBOSITY_INFO);
    }
  }

  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderWindow> renWin;
  iren->SetRenderWindow(renWin);
  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer);

  const char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/bunny.ply");
  vtkNew<vtkPLYReader> polysource;
  polysource->SetFileName(fileName);

  vtkNew<vtkPolyDataNormals> normals;
  normals->SetInputConnection(polysource->GetOutputPort());
  // normals->ComputePointNormalsOn();
  // normals->ComputeCellNormalsOff();

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(normals->GetOutputPort());
  vtkNew<vtkActor> actor;
  renderer->AddActor(actor);
  actor->SetMapper(mapper);
  // actor->GetProperty()->SetEdgeColor(1,0,0);
  // actor->GetProperty()->EdgeVisibilityOn();
  auto prop = actor->GetProperty();
  prop->SetMaterialName("matte");
  prop->SetDiffuseColor(1.0, 1.0, 1.0);
  renderer->SetBackground(0.0, 0.0, 0.5);
  renWin->SetSize(400, 400);
  renWin->Render();

  vtkNew<vtkAnariPass> anariPass;
  SetParameterDefaults(anariPass, renderer, useDebugDevice, "TestAnariPass");

  for (int i = 1; i < 10; i++)
  {
    if (i % 2)
    {
      cerr << "Render via ANARI" << endl;
      renderer->SetPass(anariPass);
    }
    else
    {
      cerr << "Render via GL" << endl;
      renderer->SetPass(nullptr);
    }

    renWin->Render();
  }

  int retVal = vtkRegressionTestImage(renWin);

  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    vtkNew<vtkAnariTestInteractor> style;
    style->SetPipelineControlPoints(renderer, anariPass, nullptr);
    iren->SetInteractorStyle(style);
    style->SetCurrentRenderer(renderer);

    iren->Start();
  }

  return !retVal;
}
