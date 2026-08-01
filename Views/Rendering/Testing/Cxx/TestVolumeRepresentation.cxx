// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <iostream>

#include "vtkColorTransferFunction.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRenderWindow.h"
#include "vtkStandardRenderView.h"
#include "vtkVolumeRepresentation.h"

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

namespace
{

// Resetting a transfer function hands it back to the representation, which
// rebuilds it from the scalar range of the input.  Uses its own representation
// so that the rendered state below is untouched.
int TestTransferFunctionReset()
{
  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-10, 10, -10, 10, -10, 10);
  wavelet->Update();
  double dataRange[2];
  wavelet->GetOutput()->GetPointData()->GetScalars()->GetRange(dataRange);

  vtkNew<vtkVolumeRepresentation> rep;
  rep->SetInputConnection(wavelet->GetOutputPort());

  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(0.0, 1.0, 0.0, 0.0);
  ctf->AddRGBPoint(1.0, 0.0, 0.0, 1.0);
  rep->SetColorTransferFunction(ctf);

  vtkNew<vtkPiecewiseFunction> pf;
  pf->AddPoint(0.0, 0.0);
  pf->AddPoint(1.0, 1.0);
  rep->SetScalarOpacity(pf);
  rep->Update();

  if (rep->GetColorTransferFunction() != ctf || rep->GetScalarOpacity() != pf)
  {
    std::cerr << "The user's transfer functions should be the ones in use." << std::endl;
    return EXIT_FAILURE;
  }

  // Resetting the color leaves the user's opacity alone.
  rep->ResetColorTransferFunction();
  if (rep->GetColorTransferFunction() == ctf)
  {
    std::cerr << "ResetColorTransferFunction did not discard the user's function." << std::endl;
    return EXIT_FAILURE;
  }
  if (rep->GetScalarOpacity() != pf)
  {
    std::cerr << "ResetColorTransferFunction should not touch the opacity." << std::endl;
    return EXIT_FAILURE;
  }

  // The rebuilt function covers the scalar range of the input, not the range
  // of the function it replaced.
  double* range = rep->GetColorTransferFunction()->GetRange();
  if (range[0] != dataRange[0] || range[1] != dataRange[1])
  {
    std::cerr << "The regenerated color transfer function should span the data range ["
              << dataRange[0] << ", " << dataRange[1] << "], got [" << range[0] << ", " << range[1]
              << "]." << std::endl;
    return EXIT_FAILURE;
  }

  rep->ResetScalarOpacity();
  if (rep->GetScalarOpacity() == pf)
  {
    std::cerr << "ResetScalarOpacity did not discard the user's function." << std::endl;
    return EXIT_FAILURE;
  }

  // Both at once, from a state where both are the user's again.
  rep->SetColorTransferFunction(ctf);
  rep->SetScalarOpacity(pf);
  rep->ResetTransferFunctions();
  if (rep->GetColorTransferFunction() == ctf || rep->GetScalarOpacity() == pf)
  {
    std::cerr << "ResetTransferFunctions did not discard both functions." << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

}

int TestVolumeRepresentation(int argc, char* argv[])
{
  if (TestTransferFunctionReset() != EXIT_SUCCESS)
  {
    return EXIT_FAILURE;
  }

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
