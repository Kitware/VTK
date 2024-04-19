// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkActor.h>
#include <vtkCompositeDataGeometryFilter.h>
#include <vtkIOSSReader.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTemporalDataSetCache.h>
#include <vtkTestUtilities.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>

int TestTemporalCacheUndefinedTimeStep(int argc, char* argv[])
{
  vtkNew<vtkIOSSReader> reader;
  std::string fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/can.ex2");
  reader->SetFileName(fileName.c_str());

  vtkNew<vtkTemporalDataSetCache> temporalCache;
  temporalCache->SetInputConnection(reader->GetOutputPort());
  temporalCache->SetCacheSize(43);

  vtkNew<vtkTransform> transform;
  transform->RotateX(90);

  vtkNew<vtkTransformFilter> transformFilter;
  transformFilter->SetInputConnection(temporalCache->GetOutputPort());
  transformFilter->SetTransform(transform);

  vtkNew<vtkCompositeDataGeometryFilter> geometryFilter;
  geometryFilter->SetInputConnection(transformFilter->GetOutputPort());
  geometryFilter->UpdateTimeStep(0.00165); // Doesn't exist

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputDataObject(geometryFilter->GetOutputDataObject(0));

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(300, 300);
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
