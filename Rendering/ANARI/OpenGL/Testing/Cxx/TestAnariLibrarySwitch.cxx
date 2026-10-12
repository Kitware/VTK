// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkAnariPass.h"
#include "vtkAnariSceneGraph.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPLYReader.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataNormals.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"

#include <cstdlib>

/**
 * Replace the ANARI pass between renders so the renderer and actor outlive the
 * scene graph nodes that observe them.
 */
int TestAnariLibrarySwitch(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer);
  renWin->SetSize(400, 400);
  renderer->SetBackground(0.0, 0.0, 0.5);

  char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/bunny.ply");
  vtkNew<vtkPLYReader> reader;
  reader->SetFileName(fileName);
  delete[] fileName;

  vtkNew<vtkPolyDataNormals> normals;
  normals->SetInputConnection(reader->GetOutputPort());
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(normals->GetOutputPort());
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetMaterialName("matte");
  renderer->AddActor(actor);

  // "environment" resolves through ANARI_LIBRARY, so each pass loads its own device.
  for (const char* library : { "environment", "helide", "environment" })
  {
    vtkNew<vtkAnariPass> anariPass;
    if (!anariPass->GetAnariDevice()->SetupAnariDeviceFromLibrary(library, "default", false))
    {
      vtkLogF(ERROR, "Could not load ANARI library '%s'", library);
      return EXIT_FAILURE;
    }
    vtkAnariSceneGraph::SetCompositeOnGL(renderer, 1);
    renderer->SetPass(anariPass);
    renWin->Render();
  }

  return EXIT_SUCCESS;
}
