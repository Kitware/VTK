// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Description
// This test creates a wavelet and apply several mask points with different
// parameters

#include "vtkActor.h"
#include "vtkDataSetMapper.h"
#include "vtkMaskPoints.h"
#include "vtkNew.h"
#include "vtkProperty.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTesting.h"

int TestMaskPointsModes(int argc, char* argv[])
{
  // Create the sample dataset
  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-10, 10, -10, 10, -10, 10);
  wavelet->SetCenter(0.0, 0.0, 0.0);
  wavelet->Update();

  // Default mode: contiguous points
  vtkNew<vtkMaskPoints> maskPointDefault;
  maskPointDefault->SetInputConnection(wavelet->GetOutputPort());
  maskPointDefault->SetRandomMode(false);
  maskPointDefault->SetMaximumNumberOfPoints(100);
  maskPointDefault->GenerateVerticesOn();

  // Uniform: random in space (volume based)
  vtkNew<vtkMaskPoints> maskPointUniformVolume;
  maskPointUniformVolume->SetInputConnection(wavelet->GetOutputPort());
  maskPointUniformVolume->SetRandomMode(true);
  maskPointUniformVolume->SetRandomModeType(vtkMaskPoints::UNIFORM_SPATIAL_VOLUME);
  maskPointUniformVolume->SetRandomSeed(12);
  maskPointUniformVolume->SetMaximumNumberOfPoints(100);
  maskPointUniformVolume->GenerateVerticesOn();

  // Uniform: random in space (bound based)
  vtkNew<vtkMaskPoints> maskPointUniformBounds;
  maskPointUniformBounds->SetInputConnection(wavelet->GetOutputPort());
  maskPointUniformBounds->SetRandomMode(true);
  maskPointUniformBounds->SetRandomModeType(vtkMaskPoints::UNIFORM_SPATIAL_BOUNDS);
  maskPointUniformBounds->SetRandomSeed(12);
  maskPointUniformBounds->SetMaximumNumberOfPoints(100);
  maskPointUniformBounds->GenerateVerticesOn();

  vtkNew<vtkDataSetMapper> mapper1, mapper2, mapper3;
  mapper1->SetInputConnection(maskPointDefault->GetOutputPort());
  mapper1->ScalarVisibilityOff();
  mapper2->SetInputConnection(maskPointUniformVolume->GetOutputPort());
  mapper2->ScalarVisibilityOff();
  mapper3->SetInputConnection(maskPointUniformBounds->GetOutputPort());
  mapper3->ScalarVisibilityOff();

  vtkNew<vtkActor> actor1, actor2, actor3;
  actor1->SetMapper(mapper1);
  actor1->GetProperty()->SetOpacity(0.5);
  actor1->GetProperty()->SetPointSize(3);
  actor1->GetProperty()->SetColor(255, 0, 0);
  actor2->SetMapper(mapper2);
  actor2->GetProperty()->SetOpacity(0.5);
  actor2->GetProperty()->SetPointSize(5);
  actor2->GetProperty()->SetColor(0, 255, 0);
  actor3->SetMapper(mapper3);
  actor3->GetProperty()->SetOpacity(0.5);
  actor3->GetProperty()->SetPointSize(7);
  actor3->GetProperty()->SetColor(0, 0, 255);

  vtkNew<vtkRenderer> ren;
  ren->AddActor(actor1);
  ren->AddActor(actor2);
  ren->AddActor(actor3);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(300, 300);
  renWin->AddRenderer(ren);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  ren->ResetCamera();
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
