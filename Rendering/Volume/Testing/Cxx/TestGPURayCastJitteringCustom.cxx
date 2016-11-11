/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastJitteringCustom.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/** Tests stochastic jittering by rendering a volume exhibiting aliasing due
 * to a big sampling distance (low sampling frequency), a.k.a. wood-grain
 * artifacts. The expected output is 'filtered' due to the noise introduced
 * by a customized noise generator.
 */

#include <iostream>

#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkNew.h>
#include <vtkOpenGLGPUVolumeRayCastMapper.h>
#include <vtkPerlinNoise.h>
#include <vtkPiecewiseFunction.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkStructuredPointsReader.h>
#include <vtkTestUtilities.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>


static const char* TestGPURayCastJitteringCustomLog =
"# StreamVersion 1\n"
"EnterEvent 298 27 0 0 0 0 0\n"
"MouseWheelForwardEvent 200 142 0 0 0 0 0\n"
"LeaveEvent 311 71 0 0 0 0 0\n";

int TestGPURayCastJitteringCustom(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  char* volumeFile = vtkTestUtilities::ExpandDataFileName(
                       argc, argv, "Data/ironProt.vtk");
  vtkNew<vtkStructuredPointsReader> reader;
  reader->SetFileName(volumeFile);
  delete[] volumeFile;

  vtkNew<vtkGPUVolumeRayCastMapper> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());
  mapper->SetAutoAdjustSampleDistances(0);
  mapper->SetSampleDistance(2.0);
  mapper->UseJitteringOn();

  vtkNew<vtkColorTransferFunction> color;
  color->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
  color->AddRGBPoint(64.0, 1.0, 0.0, 0.0);
  color->AddRGBPoint(128.0, 0.0, 0.0, 1.0);
  color->AddRGBPoint(192.0, 0.0, 1.0, 0.0);
  color->AddRGBPoint(255.0, 0.0, 0.2, 0.0);

  vtkNew<vtkPiecewiseFunction> opacity;
  opacity->AddPoint(0.0, 0.0);
  opacity->AddPoint(255.0, 1.0);

  vtkNew<vtkVolumeProperty> property;
  property->SetColor(color.GetPointer());
  property->SetScalarOpacity(opacity.GetPointer());
  property->SetInterpolationTypeToLinear();
  property->ShadeOff();

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper.GetPointer());
  volume->SetProperty(property.GetPointer());

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(400, 400);
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());
  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style.GetPointer());

  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren.GetPointer());


  ren->AddVolume(volume.GetPointer());
  ren->ResetCamera();
  ren->GetActiveCamera()->SetPosition(79.1817, 14.6622, 62.9264);
  ren->GetActiveCamera()->SetFocalPoint(32.0598, 26.5308, 28.0257);

  renWin->Render();
  iren->Initialize();

  // Customize the noise function and texture size
  vtkOpenGLGPUVolumeRayCastMapper* glMapper =
  vtkOpenGLGPUVolumeRayCastMapper::SafeDownCast(mapper.GetPointer());

  int texSize[2] = {600, 600};
  glMapper->SetNoiseTextureSize(texSize);

  vtkPerlinNoise* generator = vtkPerlinNoise::New();
  generator->SetFrequency(1024.0, 1024.0, 1.0);
  generator->SetAmplitude(0.5);
  glMapper->SetNoiseGenerator(generator);
  generator->Delete();

  renWin->Render();

  int rv = vtkTesting::InteractorEventLoop(argc, argv, iren.GetPointer(),
    TestGPURayCastJitteringCustomLog);

  return rv;
}
