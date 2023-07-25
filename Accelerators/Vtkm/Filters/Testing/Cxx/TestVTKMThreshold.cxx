// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkActor.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkThreshold.h"
#include "vtkTrivialProducer.h"
#include "vtkmThreshold.h"

namespace
{
void fillElevationArray(vtkFloatArray* elven, vtkImageData* grid)
{
  elven->SetName("Elevation");
  const vtkIdType size = grid->GetNumberOfPoints();
  elven->SetNumberOfValues(size);
  double pos[3] = { 0, 0, 0 };
  for (vtkIdType i = 0; i < size; ++i)
  {
    grid->GetPoint(i, pos);
    elven->SetValue(i, sqrt(vtkMath::Dot(pos, pos)));
  }
}

int RunVTKPipeline(vtkImageData* grid, int argc, char* argv[])
{
  vtkNew<vtkRenderer> ren;
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderWindowInteractor> iren;

  renWin->AddRenderer(ren);
  iren->SetRenderWindow(renWin);

  // compute an elevation array
  vtkNew<vtkFloatArray> elevationPoints;
  fillElevationArray(elevationPoints, grid);
  grid->GetPointData()->AddArray(elevationPoints);

  vtkNew<vtkTrivialProducer> producer;
  producer->SetOutput(grid);

  vtkNew<vtkmThreshold> threshold;
  threshold->ForceVTKmOn();
  threshold->SetInputConnection(producer->GetOutputPort());
  threshold->AllScalarsOn();
  threshold->SetThresholdFunction(vtkThreshold::THRESHOLD_BETWEEN);
  threshold->SetLowerThreshold(0.0);
  threshold->SetUpperThreshold(100.0);
  threshold->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Elevation");

  vtkNew<vtkDataSetSurfaceFilter> surface;
  surface->SetInputConnection(threshold->GetOutputPort());

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(surface->GetOutputPort());
  mapper->ScalarVisibilityOn();
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SelectColorArray("Elevation");
  mapper->SetScalarRange(0.0, 100.0);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetAmbient(1.0);
  actor->GetProperty()->SetDiffuse(0.0);

  ren->AddActor(actor);
  ren->ResetCamera();
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
    retVal = vtkRegressionTester::PASSED;
  }
  return (!retVal);
}

} // Anonymous namespace

int TestVTKMThreshold(int argc, char* argv[])
{
  // create the sample grid
  vtkNew<vtkImageData> grid;
  int dim = 128;
  grid->SetOrigin(0.0, 0.0, 0.0);
  grid->SetSpacing(1.0, 1.0, 1.0);
  grid->SetExtent(0, dim - 1, 0, dim - 1, 0, dim - 1);

  // run the pipeline
  return RunVTKPipeline(grid, argc, argv);
}
