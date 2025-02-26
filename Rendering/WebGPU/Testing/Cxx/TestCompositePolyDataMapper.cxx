// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkConeSource.h"
#include "vtkGroupDataSetsFilter.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"

namespace
{
void GenerateGlyphs(vtkGroupDataSetsFilter* groupedMeshes, vtkAlgorithm* source)
{
  source->Update();
  auto* dataset = vtkDataSet::SafeDownCast(source->GetOutputDataObject(0));
  double bounds[6];
  dataset->GetBounds(bounds);
  double lengths[3] = { bounds[1] - bounds[0], bounds[3] - bounds[2], bounds[5] - bounds[4] };
  double scales[3] = { 1 / lengths[0], 1 / lengths[1], 1 / lengths[2] };
  for (std::size_t i = 0; i < 2; ++i)
  {
    for (std::size_t j = 0; j < 2; ++j)
    {
      for (std::size_t k = 0; k < 2; ++k)
      {
        vtkNew<vtkTransformFilter> transformFilter;
        transformFilter->SetInputConnection(source->GetOutputPort());
        vtkNew<vtkTransform> transform;
        transform->Translate(i, j, k);
        transform->Scale(scales);
        transformFilter->SetTransform(transform);
        groupedMeshes->AddInputConnection(transformFilter->GetOutputPort());
      }
    }
  }
}

}

int TestCompositePolyDataMapper(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetWindowName(__func__);
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer);

  vtkNew<vtkGroupDataSetsFilter> groupMeshes;
  vtkNew<vtkConeSource> cone;
  ::GenerateGlyphs(groupMeshes, cone);

  vtkNew<vtkCompositePolyDataMapper> mapper;
  mapper->SetInputConnection(groupMeshes->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  renderer->ResetCamera();
  renderer->SetBackground(0.2, 0.3, 0.4);
  renWin->Render();

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);
  style->SetDefaultRenderer(renderer);

  const int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return 0;
}
