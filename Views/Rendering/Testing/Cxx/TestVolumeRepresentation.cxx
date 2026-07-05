// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <iostream>

#include "vtkColorTransferFunction.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRenderWindow.h"
#include "vtkStandardRenderView.h"
#include "vtkVolumeRepresentation.h"

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

int TestVolumeRepresentation(int argc, char* argv[])
{
  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-10, 10, -10, 10, -10, 10);

  vtkNew<vtkVolumeRepresentation> rep;
  rep->SetInputConnection(wavelet->GetOutputPort());

  // Test transfer functions
  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(37.0, 0.0, 0.0, 1.0);
  ctf->AddRGBPoint(157.0, 0.0, 1.0, 0.0);
  ctf->AddRGBPoint(276.0, 1.0, 0.0, 0.0);
  rep->SetColorTransferFunction(ctf);

  vtkNew<vtkPiecewiseFunction> pf;
  pf->AddPoint(37.0, 0.0);
  pf->AddPoint(276.0, 0.5);
  rep->SetScalarOpacity(pf);

  // Test volume properties
  rep->SetShade(true);
  rep->SetAmbient(0.1);
  rep->SetDiffuse(0.9);
  rep->SetSpecular(0.2);
  rep->SetSpecularPower(10.0);

  // Test visibility and transforms
  rep->SetVisibility(true);
  rep->SetPosition(0.0, 0.0, 0.0);
  rep->SetOrientation(0.0, 0.0, 0.0);
  rep->SetScale(1.0, 1.0, 1.0);

  // Test scalar bar
  rep->SetScalarBarVisibility(true);
  if (!rep->GetScalarBarActor())
  {
    std::cerr << "GetScalarBarActor() returned null after enabling." << std::endl;
    return EXIT_FAILURE;
  }

  // Test GetVolume
  if (!rep->GetVolume())
  {
    std::cerr << "GetVolume() returned null." << std::endl;
    return EXIT_FAILURE;
  }

  // Add to a view and render
  vtkNew<vtkStandardRenderView> view;
  view->GetRenderWindow()->SetOffScreenRendering(true);
  view->SetWindowSize(300, 300);
  view->AddRepresentation(rep);
  view->ResetCamera();
  view->Render();

  int retVal = vtkRegressionTestImage(view->GetRenderWindow());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    view->Start();
  }
  return !retVal;
}
