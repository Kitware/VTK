/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMaskPointsModes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

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

  // Uniform: random in space
  vtkNew<vtkMaskPoints> maskPointUniformSpatial;
  maskPointUniformSpatial->SetInputConnection(wavelet->GetOutputPort());
  maskPointUniformSpatial->SetRandomMode(true);
  maskPointUniformSpatial->SetRandomModeType(vtkMaskPoints::UNIFORM_SPATIAL_VOLUME);
  maskPointUniformSpatial->SetRandomSeed(12);
  maskPointUniformSpatial->SetMaximumNumberOfPoints(100);
  maskPointUniformSpatial->GenerateVerticesOn();

  vtkNew<vtkDataSetMapper> mapper1, mapper2;
  mapper1->SetInputConnection(maskPointDefault->GetOutputPort());
  mapper1->ScalarVisibilityOff();
  mapper2->SetInputConnection(maskPointUniformSpatial->GetOutputPort());
  mapper2->ScalarVisibilityOff();

  vtkNew<vtkActor> actor1, actor2, actor3;
  actor1->SetMapper(mapper1);
  actor1->GetProperty()->SetOpacity(0.5);
  actor1->GetProperty()->SetPointSize(3);
  actor1->GetProperty()->SetColor(255, 0, 0);
  actor2->SetMapper(mapper2);
  actor2->GetProperty()->SetOpacity(0.5);
  actor2->GetProperty()->SetPointSize(5);
  actor2->GetProperty()->SetColor(0, 150, 150);

  vtkNew<vtkRenderer> ren;
  ren->AddActor(actor1);
  ren->AddActor(actor2);

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
