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

#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkImageData.h>
#include <vtkLight.h>
#include <vtkNew.h>
#include <vtkPiecewiseFunction.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSmartPointer.h>
#include <vtkTestUtilities.h>
#include <vtkVolumeProperty.h>
#include <vtkXMLImageDataReader.h>

#include <vtkLightActor.h>
#include <vtkContourFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>

int TestGPURayCastPositionalLights(int argc, char *argv[])
{
  double scalarRange[2];

  vtkNew<vtkGPUVolumeRayCastMapper> volumeMapper;
  vtkNew<vtkXMLImageDataReader> reader;
  const char* volumeFile = vtkTestUtilities::ExpandDataFileName(
                            argc, argv, "Data/vase_1comp.vti");
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
  light1->SetDiffuseColor(1,0,0);
  light1->SetAmbientColor(0,0,0);
  light1->SetSpecularColor(1,1,1);
  light1->SetConeAngle(60);
  light1->SetPosition(0.0, 0.0, 100.0);
  light1->SetFocalPoint(0.0, 0.0, 0.0);
//  light1->SetColor(1,0,0);
//  light1->SetPosition(40,40,301);
//  light1->SetPosition(-57, -50, -360);

  vtkNew<vtkLightActor> lightActor;
  lightActor->SetLight(light1.GetPointer());
  ren->AddViewProp(lightActor.GetPointer());
  vtkNew<vtkLight> light2;
  vtkNew<vtkLight> light3;
  vtkNew<vtkLight> light4;

  renWin->AddRenderer(ren.GetPointer());
  renWin->SetSize(400, 400);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  vtkNew<vtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(50, 0.0);
  scalarOpacity->AddPoint(75, 1.0);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOn();
  volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);
  volumeProperty->SetScalarOpacity(scalarOpacity.GetPointer());

  vtkSmartPointer<vtkColorTransferFunction> colorTransferFunction =
    volumeProperty->GetRGBTransferFunction(0);
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(scalarRange[0], 1.0, 1.0, 1.0);
  colorTransferFunction->AddRGBPoint(scalarRange[1], 1.0, 1.0, 1.0);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper.GetPointer());
  volume->SetProperty(volumeProperty.GetPointer());

  ren->AddViewProp(volume.GetPointer());

  vtkNew<vtkPolyDataMapper> pm;
  vtkNew<vtkActor> ac;
  vtkNew<vtkContourFilter> cf;
  ac->SetMapper(pm.GetPointer());
  pm->SetInputConnection(cf->GetOutputPort());
  pm->SetScalarVisibility(0);
  cf->SetValue(0, 60.0);
  cf->SetInputConnection(reader->GetOutputPort());
  ac->SetPosition(-89.0, 0.0, 0.0);
  volume->SetPosition(-30.0, 0.0, 0.0);
  ren->AddActor(ac.GetPointer());
  vtkNew<vtkActor> ac1;
  ac1->SetMapper(pm.GetPointer());
  ac1->SetPosition(0,0,0);
  ren->SetTwoSidedLighting(0);

  ren->AddLight(light1.GetPointer());
  renWin->Render();

  ren->ResetCamera();
  iren->Initialize();

  int retVal = vtkRegressionTestImage( renWin.GetPointer() );
  if( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  return !retVal;
}
