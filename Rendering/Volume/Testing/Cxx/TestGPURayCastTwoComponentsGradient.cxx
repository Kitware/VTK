/*=========================================================================
Program:   Visualization Toolkit
Module:    TestGPURayCastTwoComponentsGradient.cxx
Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

// Description
// This test creates a vtkImageData with two components.
// The data is volume rendered considering the two components as independent.

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTesting.h"
#include "vtkTestUtilities.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkUnsignedShortArray.h"

int TestGPURayCastTwoComponentsGradient(int argc, char *argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  int dims[3] = { 35, 35, 35 };

  // Create a vtkImageData with two components
  vtkNew<vtkImageData> image;
  image->SetDimensions(dims[0], dims[1], dims[2]);
  image->AllocateScalars(VTK_DOUBLE, 2);

  // Fill the first half rectangular parallelopiped along X with the
  // first component values and the second half with second component values
  double * ptr = static_cast<double *> (image->GetScalarPointer(0, 0, 0));

  for (int z = 0; z < dims[2]; ++z)
  {
    for (int y = 0; y < dims[1]; ++y)
    {
      for (int x = 0; x < dims[0]; ++x)
      {
        if (x < dims[0] / 2)
        {
          *ptr++ = 0.0;
          *ptr++ = 0.0;
        }
        else
        {
          *ptr++ = 1.0;
          *ptr++ = 1.0;
        }
      }
    }
  }

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(301, 300); // Intentional NPOT size
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren.GetPointer());

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  renWin->Render();

  // Volume render the dataset
  vtkNew<vtkGPUVolumeRayCastMapper> mapper;
  mapper->AutoAdjustSampleDistancesOff();
  mapper->SetSampleDistance(0.9);
  mapper->SetInputData(image.GetPointer());

  // Color transfer function
  vtkNew<vtkColorTransferFunction> ctf1;
  ctf1->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
  ctf1->AddRGBPoint(1.0, 1.0, 0.0, 0.0);

  vtkNew<vtkColorTransferFunction> ctf2;
  ctf2->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
  ctf2->AddRGBPoint(1.0, 0.0, 0.0, 1.0);

  // Opacity functions
  vtkNew<vtkPiecewiseFunction> pf1;
  pf1->AddPoint(0.0, 0.0);
  pf1->AddPoint(1.0, 1.0);

  vtkNew<vtkPiecewiseFunction> pf2;
  pf2->AddPoint(0.0, 0.0);
  pf2->AddPoint(1.0, 1.0);

  // Gradient Opacity function
  vtkNew<vtkPiecewiseFunction> pf3;
  pf3->AddPoint(0.0, 0.0);
  pf3->AddPoint(1.0, 1.0);

  vtkNew<vtkPiecewiseFunction> pf4;
  pf4->AddPoint(0.0, 0.0);
  pf4->AddPoint(1.0, 1.0);

  // Volume property with independent components ON
  vtkNew<vtkVolumeProperty> property;
  property->IndependentComponentsOn();

  // Set color and opacity functions
  property->SetColor(0, ctf1.GetPointer());
  property->SetColor(1, ctf2.GetPointer());
  property->SetScalarOpacity(0, pf1.GetPointer());
  property->SetScalarOpacity(1, pf2.GetPointer());
  property->SetGradientOpacity(0, pf3.GetPointer());
  property->SetGradientOpacity(1, pf4.GetPointer());

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper.GetPointer());
  volume->SetProperty(property.GetPointer());
  ren->AddVolume(volume.GetPointer());

  ren->ResetCamera();
  renWin->Render();

  iren->Initialize();

  int retVal = vtkRegressionTestImage( renWin.GetPointer() );
  if( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
