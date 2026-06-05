// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkContourFilter.h"
#include "vtkDiscretizableColorTransferFunction.h"
#include "vtkElevationFilter.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

namespace
{
vtkSmartPointer<vtkPolyData> CreatePolyDataWithLines(float y)
{
  vtkNew<vtkSphereSource> sphere;
  sphere->SetCenter(0.0, y, 0.0);
  sphere->SetRadius(5.0);
  vtkNew<vtkElevationFilter> elevationFilter;
  elevationFilter->SetInputConnection(sphere->GetOutputPort());
  elevationFilter->SetLowPoint(0.0, y - 5.0, 0.0);
  elevationFilter->SetHighPoint(0.0, y + 5.0, 0.0);
  elevationFilter->SetScalarRange(0.0, 4.0);
  vtkNew<vtkContourFilter> contour;
  contour->SetInputArray("Elevation");
  contour->GenerateValues(10, 0.0, 4.0);
  contour->SetInputConnection(elevationFilter->GetOutputPort());
  contour->Update();
  return contour->GetOutput();
}
}

int TestLineRendering(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetWindowName(__func__);
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer);

  vtkNew<vtkPartitionedDataSetCollection> pdsc;
  pdsc->SetPartition(0, 0, CreatePolyDataWithLines(0.0));
  pdsc->SetPartition(1, 0, CreatePolyDataWithLines(10.0));

  vtkNew<vtkCompositePolyDataMapper> mapper;
  mapper->SetInputDataObject(pdsc);

  vtkNew<vtkDiscretizableColorTransferFunction> ctf;
  ctf->SetDiscretize(true);
  ctf->SetNumberOfValues(128);
  ctf->AddRGBPoint(0.0, 1.0, 0.0, 0.0);
  ctf->AddRGBPoint(1.0, 0.0, 1.0, 0.0);
  ctf->AddRGBPoint(2.0, 1.0, 1.0, 1.0);
  ctf->AddRGBPoint(3.0, 0.0, 0.0, 1.0);
  ctf->AddRGBPoint(4.0, 1.0, 0.0, 1.0);
  mapper->SetLookupTable(ctf);
  mapper->InterpolateScalarsBeforeMappingOn();

  vtkNew<vtkActor> actor;
  actor->GetProperty()->SetLineWidth(4);
  bool translucent = false;
  for (int i = 0; i < argc; ++i)
  {
    if (strcmp(argv[i], "--translucent") == 0)
    {
      translucent = true;
      break;
    }
  }
  if (translucent)
  {
    actor->GetProperty()->SetOpacity(0.4);
    actor->GetProperty()->SetLineJoin(vtkProperty::LineJoinType::MiterJoin);
  }
  else
  {
    actor->GetProperty()->SetLineJoin(vtkProperty::LineJoinType::RoundCapRoundJoin);
  }
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  renderer->ResetCamera();
  renderer->SetBackground(0.2, 0.3, 0.4);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);
  style->SetDefaultRenderer(renderer);

  renWin->Render();

  const int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
