// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkActor.h"
#include "vtkDataSetMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkXMLPolyDataReader.h"

int TestPolyDataIncorrectConnectivity(int argc, char* argv[])
{
  // This data has out of range connectivity index
  char* fileName =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/BoxIncorrectConnectivity.vtp");
  vtkNew<vtkXMLPolyDataReader> reader;
  reader->SetFileName(fileName);
  reader->Update();

  vtkNew<vtkDataSetMapper> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);

  // Test that rendering this incorrect data doesn't make window crash
  // This is not a classic render and compare test
  renderer->AddActor(actor);
  renderWindow->Render();

  return EXIT_SUCCESS;
}
