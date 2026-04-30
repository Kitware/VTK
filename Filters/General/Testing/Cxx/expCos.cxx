// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
//
// Brute force computation of Bessel functions. Might be better to create a
// filter (or source) object. Might also consider vtkSampleFunction.

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkDataSetMapper.h"
#include "vtkDebugLeaks.h"
#include "vtkFloatArray.h"
#include "vtkNew.h"
#include "vtkPlaneSource.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"
#include "vtkWarpScalar.h"

int expCos(int, char*[])
{
  int i, numPts;
  double x[3];
  double r, deriv;

  vtkNew<vtkRenderer> ren;
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(ren);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // create plane to warp
  vtkNew<vtkPlaneSource> plane;
  plane->SetResolution(300, 300);

  vtkNew<vtkTransform> transform;
  transform->Scale(10.0, 10.0, 1.0);

  vtkNew<vtkTransformFilter> transF;
  transF->SetInputConnection(plane->GetOutputPort());
  transF->SetTransform(transform);
  transF->Update();

  // compute Bessel function and derivatives. This portion could be
  // encapsulated into source or filter object.
  //
  vtkPolyData* input = transF->GetPolyDataOutput();
  numPts = input->GetNumberOfPoints();

  vtkNew<vtkPoints> newPts;
  newPts->SetNumberOfPoints(numPts);

  vtkNew<vtkFloatArray> derivs;
  derivs->SetNumberOfTuples(numPts);

  vtkNew<vtkPolyData> bessel;
  bessel->CopyStructure(input);
  bessel->SetPoints(newPts);
  bessel->GetPointData()->SetScalars(derivs);

  for (i = 0; i < numPts; i++)
  {
    input->GetPoint(i, x);
    r = sqrt(x[0] * x[0] + x[1] * x[1]);
    x[2] = exp(-r) * cos(10.0 * r);
    newPts->SetPoint(i, x);
    deriv = -exp(-r) * (cos(10.0 * r) + 10.0 * sin(10.0 * r));
    derivs->SetValue(i, deriv);
  }

  // warp plane
  vtkNew<vtkWarpScalar> warp;
  warp->SetInputData(bessel);
  warp->XYPlaneOn();
  warp->SetScaleFactor(0.5);

  // mapper and actor
  vtkNew<vtkDataSetMapper> mapper;
  mapper->SetInputConnection(warp->GetOutputPort());
  double tmp[2];
  bessel->GetScalarRange(tmp);
  mapper->SetScalarRange(tmp[0], tmp[1]);

  vtkNew<vtkActor> carpet;
  carpet->SetMapper(mapper);

  // assign our actor to the renderer
  ren->AddActor(carpet);
  ren->SetBackground(1, 1, 1);
  renWin->SetSize(300, 300);

  // draw the resulting scene
  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(1.4);
  ren->GetActiveCamera()->Elevation(-55);
  ren->GetActiveCamera()->Azimuth(25);
  ren->ResetCameraClippingRange();
  renWin->Render();

  iren->Start();

  return EXIT_SUCCESS;
}
