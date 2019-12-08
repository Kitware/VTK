/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastClipping.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers cropping on volume datasets.

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkImageData.h>
#include <vtkNew.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPlane.h>
#include <vtkPlaneCollection.h>
#include <vtkRTAnalyticSource.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkTestUtilities.h>
#include <vtkVolumeProperty.h>

int TestGPURayCastClipping(int argc, char* argv[])
{
  double scalarRange[2];

  vtkNew<vtkGPUVolumeRayCastMapper> volumeMapper;

  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->Update();
  volumeMapper->SetInputConnection(wavelet->GetOutputPort());

  volumeMapper->GetInput()->GetScalarRange(scalarRange);
  volumeMapper->SetBlendModeToComposite();

  // Testing prefers image comparison with small images
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(400, 400);

  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(scalarRange[0], 0.0);
  scalarOpacity->AddPoint(scalarRange[1], 1.0);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);
  volumeProperty->SetScalarOpacity(scalarOpacity);

  vtkSmartPointer<vtkColorTransferFunction> colorTransferFunction =
    volumeProperty->GetRGBTransferFunction(0);
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(scalarRange[0], 0.1, 0.5, 1.0);
  colorTransferFunction->AddRGBPoint(scalarRange[1], 1.0, 0.5, 0.1);

  // Test cropping now
  const double* bounds = wavelet->GetOutput()->GetBounds();
  vtkNew<vtkPlane> clipPlane1;
  clipPlane1->SetOrigin(0.45 * (bounds[0] + bounds[1]), 0.0, 0.0);
  clipPlane1->SetNormal(0.8, 0.0, 0.0);

  vtkNew<vtkPlane> clipPlane2;
  clipPlane2->SetOrigin(0.45 * (bounds[0] + bounds[1]), 0.35 * (bounds[2] + bounds[3]), 0.0);
  clipPlane2->SetNormal(0.2, -0.2, 0.0);

  vtkNew<vtkPlaneCollection> clipPlaneCollection;
  clipPlaneCollection->AddItem(clipPlane1);
  clipPlaneCollection->AddItem(clipPlane2);
  volumeMapper->SetClippingPlanes(clipPlaneCollection);

  // Setup volume actor
  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  ren->AddViewProp(volume);
  ren->GetActiveCamera()->Azimuth(-40);
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
