/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastJittering.cxx

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
 * by jittering the entry point of the rays.
 *
 * Added renderer to expand coverage for vtkDualDepthPeelingPass.
 */

#include <iostream>

#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkNew.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSphereSource.h>
#include <vtkStructuredPointsReader.h>
#include <vtkTestUtilities.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>


static const char* TestGPURayCastJitteringLog =
"# StreamVersion 1\n"
"EnterEvent 298 27 0 0 0 0 0\n"
"MouseWheelForwardEvent 200 142 0 0 0 0 0\n"
"LeaveEvent 311 71 0 0 0 0 0\n";

int TestGPURayCastJittering(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  char* volumeFile = vtkTestUtilities::ExpandDataFileName(
                       argc, argv, "Data/ironProt.vtk");
  vtkNew<vtkStructuredPointsReader> reader;
  reader->SetFileName(volumeFile);
  delete[] volumeFile;

  // Setup actors and mappers
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

  // Translucent Spheres
  vtkNew<vtkSphereSource> sphereSource;
  sphereSource->SetCenter(45.f, 45.f, 45.f);
  sphereSource->SetRadius(25.0);

  vtkNew<vtkActor> sphereActor;
  vtkProperty* sphereProperty = sphereActor->GetProperty();
  sphereProperty->SetColor(0.0, 1.0, 0.0);
  sphereProperty->SetOpacity(0.3);

  vtkNew<vtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(sphereSource->GetOutputPort());
  sphereActor->SetMapper(sphereMapper.GetPointer());

  vtkNew<vtkSphereSource> sphereSource2;
  sphereSource2->SetCenter(30.f, 30.f, 30.f);
  sphereSource2->SetRadius(25.0);

  vtkNew<vtkActor> sphereActor2;
  vtkProperty* sphereProperty2 = sphereActor2->GetProperty();
  sphereProperty2->SetColor(0.9, 0.9, 0.9);
  sphereProperty2->SetOpacity(0.3);

  vtkNew<vtkPolyDataMapper> sphereMapper2;
  sphereMapper2->SetInputConnection(sphereSource2->GetOutputPort());
  sphereActor2->SetMapper(sphereMapper2.GetPointer());

  // Render window
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(800, 400);
  renWin->SetMultiSamples(0);

  // Renderer 1
  vtkNew<vtkRenderer> ren;
  ren->SetViewport(0.0, 0.0, 0.5, 1.0);
  renWin->AddRenderer(ren.GetPointer());

  ren->AddVolume(volume.GetPointer());
  ren->ResetCamera();
  ren->GetActiveCamera()->SetPosition(115.539, 5.50485, 89.8544);
  ren->GetActiveCamera()->SetFocalPoint(32.0598, 26.5308, 28.0257);

  // Renderer 2 (translucent geometry)
  vtkNew<vtkRenderer> ren2;
  ren2->SetViewport(0.5, 0.0, 1.0, 1.0);
  renWin->AddRenderer(ren2.GetPointer());

  ren2->SetUseDepthPeeling(1);
  ren2->SetOcclusionRatio(0.0);
  ren2->SetMaximumNumberOfPeels(5);
  ren2->SetUseDepthPeelingForVolumes(true);

  ren2->AddVolume(volume.GetPointer());
  ren2->AddActor(sphereActor.GetPointer());
  ren2->AddActor(sphereActor2.GetPointer());
  ren2->SetActiveCamera(ren->GetActiveCamera());

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());
  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style.GetPointer());

  renWin->Render();
  iren->Initialize();

  int rv = vtkTesting::InteractorEventLoop(argc, argv, iren.GetPointer(),
    TestGPURayCastJitteringLog);

  return rv;
}
