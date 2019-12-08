/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastPositionalLights.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test volume renders a synthetic dataset with four different
// positional lights in the scene.

#include "vtkOSPRayPass.h"
#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkImageData.h>
#include <vtkLight.h>
#include <vtkNew.h>
#include <vtkPiecewiseFunction.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkTestUtilities.h>
#include <vtkVolumeProperty.h>
#include <vtkXMLImageDataReader.h>

#include <vtkActor.h>
#include <vtkContourFilter.h>
#include <vtkLightActor.h>
#include <vtkPolyDataMapper.h>

int TestGPURayCastPositionalLights(int argc, char* argv[])
{
  bool useOSP = true;
  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "-GL"))
    {
      cerr << "GL" << endl;
      useOSP = false;
    }
  }

  double scalarRange[2];

  vtkNew<vtkGPUVolumeRayCastMapper> volumeMapper;
  vtkNew<vtkXMLImageDataReader> reader;
  const char* volumeFile = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vase_1comp.vti");
  reader->SetFileName(volumeFile);
  volumeMapper->SetInputConnection(reader->GetOutputPort());

  volumeMapper->GetInput()->GetScalarRange(scalarRange);
  volumeMapper->SetBlendModeToComposite();
  volumeMapper->SetAutoAdjustSampleDistances(0);
  volumeMapper->SetSampleDistance(0.1);

  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderer> ren;
  ren->SetBackground(0.0, 0.0, 0.4);
  ren->AutomaticLightCreationOff();
  ren->RemoveAllLights();

  vtkNew<vtkLight> light1;
  light1->SetLightTypeToSceneLight();
  light1->SetPositional(true);
  light1->SetDiffuseColor(1, 0, 0);
  light1->SetAmbientColor(0, 0, 0);
  light1->SetSpecularColor(1, 1, 1);
  light1->SetConeAngle(60);
  light1->SetPosition(0.0, 0.0, 100.0);
  light1->SetFocalPoint(0.0, 0.0, 0.0);

  vtkNew<vtkLightActor> lightActor;
  lightActor->SetLight(light1);
  ren->AddViewProp(lightActor);

  renWin->AddRenderer(ren);
  renWin->SetSize(400, 400);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(50, 0.0);
  scalarOpacity->AddPoint(75, 1.0);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOn();
  volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);
  volumeProperty->SetScalarOpacity(scalarOpacity);

  vtkSmartPointer<vtkColorTransferFunction> colorTransferFunction =
    volumeProperty->GetRGBTransferFunction(0);
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(scalarRange[0], 1.0, 1.0, 1.0);
  colorTransferFunction->AddRGBPoint(scalarRange[1], 1.0, 1.0, 1.0);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  ren->AddViewProp(volume);

  vtkNew<vtkPolyDataMapper> pm;
  vtkNew<vtkActor> ac;
  vtkNew<vtkContourFilter> cf;
  ac->SetMapper(pm);
  pm->SetInputConnection(cf->GetOutputPort());
  pm->SetScalarVisibility(0);
  cf->SetValue(0, 60.0);
  cf->SetInputConnection(reader->GetOutputPort());
  ac->SetPosition(-89.0, 0.0, 0.0);
  volume->SetPosition(-30.0, 0.0, 0.0);
  ren->AddActor(ac);
  vtkNew<vtkActor> ac1;
  ac1->SetMapper(pm);
  ac1->SetPosition(0, 0, 0);
  ren->SetTwoSidedLighting(0);

  ren->AddLight(light1);

  // Attach OSPRay render pass
  vtkSmartPointer<vtkOSPRayPass> osprayPass = vtkSmartPointer<vtkOSPRayPass>::New();
  if (useOSP)
  {
    ren->SetPass(osprayPass);
  }

  renWin->Render();

  ren->ResetCamera();
  iren->Initialize();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
