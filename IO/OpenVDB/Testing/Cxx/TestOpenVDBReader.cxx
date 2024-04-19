// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenVDBReader.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"

int TestOpenVDBReader(int argc, char* argv[])
{
  char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/sphere_points.vdb");
  if (fileName == nullptr)
  {
    cerr << "Could not get file names.";
    return EXIT_FAILURE;
  }

  vtkNew<vtkOpenVDBReader> reader;
  if (!reader->CanReadFile(fileName))
  {
    cerr << "Reader reports " << fileName << " cannot be read.";
    return EXIT_FAILURE;
  }

  reader->SetFileName(fileName);
  reader->Update();

  vtkPartitionedDataSetCollection* output =
    vtkPartitionedDataSetCollection::SafeDownCast(reader->GetOutputDataObject(0));
  vtkPolyData* outputblock = vtkPolyData::SafeDownCast(output->GetPartitionAsDataObject(0, 0));

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputData(outputblock);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.2, 0.2, 0.2);

  vtkCamera* camera = renderer->GetActiveCamera();
  camera->SetPosition(0.0, 0.0, 5.0);
  camera->SetFocalPoint(0.0, 0.0, 0.0);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);
  renderWindow->SetSize(400, 400);

  renderer->AddActor(actor);

  renderWindow->Render();

  delete[] fileName;

  return EXIT_SUCCESS;
}
