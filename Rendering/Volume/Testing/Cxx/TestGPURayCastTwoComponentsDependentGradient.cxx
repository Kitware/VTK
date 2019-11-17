/*=========================================================================
Program:   Visualization Toolkit
Module:    TestGPURayCastTwoComponentsDependentGradient.cxx
Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

// Description
// This test creates a vtkImageData with two components.
// The data is volume rendered considering the two components as dependent
// and gradient based modulation of the opacity is applied

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkUnsignedShortArray.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

int TestGPURayCastTwoComponentsDependentGradient(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  int dims[3] = { 30, 30, 30 };

  // Create a vtkImageData with two components
  vtkNew<vtkImageData> image;
  image->SetDimensions(dims[0], dims[1], dims[2]);
  image->AllocateScalars(VTK_DOUBLE, 2);

  // Fill the first half rectangular parallelopiped along X with the
  // first component values and the second half with second component values
  double* ptr = static_cast<double*>(image->GetScalarPointer(0, 0, 0));

  for (int z = 0; z < dims[2]; ++z)
  {
    for (int y = 0; y < dims[1]; ++y)
    {
      for (int x = 0; x < dims[0]; ++x)
      {
        if (x < dims[0] / 2)
        {
          if (y < dims[1] / 2)
          {
            *ptr++ = 0.0;
            *ptr++ = 0.0;
          }
          else
          {
            *ptr++ = 0.25;
            *ptr++ = 25.0;
          }
        }
        else
        {
          if (y < dims[1] / 2)
          {
            *ptr++ = 0.5;
            *ptr++ = 50.0;
          }
          else
          {
            *ptr++ = 1.0;
            *ptr++ = 100.0;
          }
        }
      }
    }
  }

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(301, 300); // Intentional NPOT size
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renWin->Render();

  // Volume render the dataset
  vtkNew<vtkGPUVolumeRayCastMapper> mapper;
  mapper->AutoAdjustSampleDistancesOff();
  mapper->SetSampleDistance(0.5);
  mapper->SetInputData(image);

  // Color transfer function
  vtkNew<vtkColorTransferFunction> ctf1;
  ctf1->AddRGBPoint(0.0, 0.0, 0.0, 1.0);
  ctf1->AddRGBPoint(0.5, 0.0, 1.0, 0.0);
  ctf1->AddRGBPoint(1.0, 1.0, 0.0, 0.0);

  vtkNew<vtkColorTransferFunction> ctf2;
  ctf2->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
  ctf2->AddRGBPoint(1.0, 0.0, 0.0, 1.0);

  // Opacity functions
  vtkNew<vtkPiecewiseFunction> pf1;
  pf1->AddPoint(0.0, 0.1);
  pf1->AddPoint(100.0, 0.1);

  // Gradient Opacity function
  // Whenever the gradient
  vtkNew<vtkPiecewiseFunction> pf2;
  pf2->AddPoint(0.0, 0.2);
  pf2->AddPoint(30.0, 1.0);

  // Volume property with independent components OFF
  vtkNew<vtkVolumeProperty> property;
  property->IndependentComponentsOff();

  // Set color and opacity functions
  property->SetColor(0, ctf1);
  // Setting the transfer function for second component would be a no-op as only
  // the first component functions are used.
  property->SetColor(1, ctf2);
  property->SetScalarOpacity(0, pf1);
  property->SetGradientOpacity(0, pf2);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(property);
  ren->AddVolume(volume);

  ren->ResetCamera();
  renWin->Render();

  iren->Initialize();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
