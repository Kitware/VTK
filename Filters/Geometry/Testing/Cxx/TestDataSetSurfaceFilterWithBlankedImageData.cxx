// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * Tests blanking in fast and non-fast mode with vtkDataSetSurfaceFilter.
 */

#include "vtkCamera.h"
#include "vtkClipDataSet.h"
#include "vtkClipPolyData.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPlane.h"
#include "vtkPolyDataMapper.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkResampleToImage.h"
#include "vtkSphere.h"
#include "vtkUnstructuredGrid.h"

vtkSmartPointer<vtkDataObject> GenerateDataSet()
{
  vtkLogScopeF(INFO, "GenerateDataSet");
  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-10, 10, -10, 10, -10, 10);

  vtkNew<vtkSphere> outerSphere;
  outerSphere->SetCenter(0, 0, 0);
  outerSphere->SetRadius(8);

  vtkNew<vtkClipDataSet> outerSphereClip;
  outerSphereClip->SetInputConnection(wavelet->GetOutputPort());
  outerSphereClip->SetClipFunction(outerSphere);
  outerSphereClip->InsideOutOn();

  vtkNew<vtkSphere> innerSphere;
  innerSphere->SetCenter(2, 2, 2);
  innerSphere->SetRadius(4);

  vtkNew<vtkClipDataSet> innerSphereClip;
  innerSphereClip->SetInputConnection(outerSphereClip->GetOutputPort());
  innerSphereClip->SetClipFunction(innerSphere);

  vtkNew<vtkResampleToImage> resampler;
  resampler->UseInputBoundsOff();
  resampler->SetSamplingBounds(-10, 10, -10, 10, -10, 10);
  resampler->SetSamplingDimensions(100, 100, 100);
  resampler->SetInputConnection(innerSphereClip->GetOutputPort());

  resampler->Update();
  return resampler->GetOutputDataObject(0);
}

vtkActor* AddActor(vtkRenderer* renderer, vtkAlgorithm* producer)
{
  vtkNew<vtkPlane> plane;

  vtkNew<vtkClipPolyData> clipper;
  clipper->SetInputConnection(producer->GetOutputPort());
  clipper->SetClipFunction(plane);
  clipper->InsideOutOn();
  clipper->Update();

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputData(clipper->GetOutput());
  mapper->SetColorModeToMapScalars();
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SelectColorArray("RTData");
  mapper->SetScalarRange(37, 280);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  renderer->AddActor(actor);
  return actor;
}

int TestDataSetSurfaceFilterWithBlankedImageData(int argc, char* argv[])
{
  // First, generate a dataset.
  auto dataset = ::GenerateDataSet();

  vtkNew<vtkRenderer> ren;

  {
    vtkLogScopeF(INFO, "vtkDataSetSurfaceFilter (fast=false, delegate=false)");
    // This is the best form: we get 2 concentric surfaces
    vtkNew<vtkDataSetSurfaceFilter> dsaFilter1;
    dsaFilter1->SetInputData(dataset);
    dsaFilter1->FastModeOff();
    dsaFilter1->DelegationOff();
    ::AddActor(ren, dsaFilter1);
  }

  {
    vtkLogScopeF(INFO, "vtkDataSetSurfaceFilter (fast=true, delegate=false)");
    // In this mode, the inner surface will be missing.
    vtkNew<vtkDataSetSurfaceFilter> dsaFilter2;
    dsaFilter2->SetInputData(dataset);
    dsaFilter2->FastModeOn();
    dsaFilter2->DelegationOff();
    ::AddActor(ren, dsaFilter2)->AddPosition(22, 0, 0);
  }

  {
    vtkLogScopeF(INFO, "vtkDataSetSurfaceFilter (fast=false, delegate=true)");
    // vtkGeometryFilter is totally busted on this dataset right now!
    // Oh well, we'll still add this test. It test for the current state of it.
    // The baselines should be updated once it's fixed.
    // Issue: vtk/vtk#18279
    vtkNew<vtkDataSetSurfaceFilter> dsaFilter3;
    dsaFilter3->SetInputData(dataset);
    dsaFilter3->FastModeOff();
    dsaFilter3->DelegationOn();
    ::AddActor(ren, dsaFilter3)->AddPosition(0, -22, 0);
  }

  {
    vtkLogScopeF(INFO, "vtkDataSetSurfaceFilter (fast=true, delegate=true)");
    vtkNew<vtkDataSetSurfaceFilter> dsaFilter4;
    dsaFilter4->SetInputData(dataset);
    dsaFilter4->FastModeOn();
    dsaFilter4->DelegationOn();
    ::AddActor(ren, dsaFilter4)->AddPosition(22, -22, 0);
  }

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(600, 600);
  renWin->AddRenderer(ren);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renWin->Render();
  int retValTmp = vtkRegressionTestImageThreshold(renWin, 3.0);
  if (retValTmp == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  if (retValTmp != vtkRegressionTester::PASSED)
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
