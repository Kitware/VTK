/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastRenderDepthToImage2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Description
// Test the GPU volume mapper low level API to render depth buffer to texture

#include "vtkActor.h"
#include "vtkRTAnalyticSource.h"
#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkImageActor.h"
#include "vtkImageData.h"
#include "vtkImageMapper3D.h"
#include "vtkNew.h"
#include "vtkOpenGLGPUVolumeRayCastMapper.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTesting.h"
#include "vtkTestUtilities.h"
#include "vtkVolume16Reader.h"
#include "vtkVolumeProperty.h"

int TestGPURayCastRenderDepthToImage2(int argc, char *argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  vtkNew<vtkGPUVolumeRayCastMapper> volumeMapper;
  vtkNew<vtkRTAnalyticSource> waveletSource;
  volumeMapper->SetInputConnection(waveletSource->GetOutputPort());
  volumeMapper->RenderToImageOn();

  vtkNew<vtkColorTransferFunction> colorFunction;
  colorFunction->AddRGBPoint(900.0, 198/255.0, 134/255.0, 66/255.0);
  colorFunction->AddRGBPoint(37.35310363769531, 0.231373, 0.298039, 0.752941);
  colorFunction->AddRGBPoint(157.0909652709961, 0.865003, 0.865003, 0.865003);
  colorFunction->AddRGBPoint(276.8288269042969, 0.705882, 0.0156863, 0.14902);

  float dataRange[2];
  dataRange[0] = 37.3;
  dataRange[1] = 276.8;
  float halfSpread = (dataRange[1] - dataRange[0]) / 2.0;
  float center = dataRange[0] + halfSpread;

  vtkNew<vtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->RemoveAllPoints();
  scalarOpacity->AddPoint(center, 0.01);
  scalarOpacity->AddPoint(dataRange[1], 0.4);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOn();
  volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);
  volumeProperty->SetColor(colorFunction.GetPointer());
  volumeProperty->SetScalarOpacity(scalarOpacity.GetPointer());

  // Setup volume actor
  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper.GetPointer());
  volume->SetProperty(volumeProperty.GetPointer());

  // Testing prefers image comparison with small images
  vtkNew<vtkRenderWindow> renWin;

  // Intentional odd and NPOT  width/height
  renWin->SetSize(401, 399);

  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren.GetPointer());

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  ren->AddVolume(volume.GetPointer());
  ren->ResetCamera();
  renWin->Render();

  vtkNew<vtkImageData> im;

  // Get color texture as image
  volumeMapper->GetColorImage(im.GetPointer());

  // Get depth texture as image
  volumeMapper->GetDepthImage(im.GetPointer());

  vtkNew<vtkImageActor> ia;
  ia->GetMapper()->SetInputData(im.GetPointer());
  ren->AddActor(ia.GetPointer());
  ren->RemoveVolume(volume.GetPointer());
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
