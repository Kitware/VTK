/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastMIPToComposite.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This test covers vtkFixedPointVolumeRayCastMapper with a light where
// diffuse and specular components are different.
// This test volume renders a synthetic dataset with unsigned char values,
// with the composite method. The diffuse light component is gray, the
// light specular component is blue.

#include "vtkSphere.h"
#include "vtkSampleFunction.h"

#include "vtkTestUtilities.h"
#include "vtkColorTransferFunction.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkVolumeProperty.h"
#include "vtkCamera.h"
#include "vtkRegressionTestImage.h"
#include "vtkImageShiftScale.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkDataArray.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include <cassert>
#include "vtkFixedPointVolumeRayCastMapper.h"

int TestFixedPointRayCastLightComponents(int argc,
                                         char *argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  // Create a spherical implicit function.
  vtkSphere *shape=vtkSphere::New();
  shape->SetRadius(0.1);
  shape->SetCenter(0.0,0.0,0.0);

  vtkSampleFunction *source=vtkSampleFunction::New();
  source->SetImplicitFunction(shape);
  shape->Delete();
  source->SetOutputScalarTypeToDouble();
  source->SetSampleDimensions(127,127,127); // intentional NPOT dimensions.
  source->SetModelBounds(-100.0,100.0,-100.0,100.0,-100.0,100.0);
  source->SetCapping(false);
  source->SetComputeNormals(false);
  source->SetScalarArrayName("values");

  source->Update();

  vtkDataArray *a=source->GetOutput()->GetPointData()->GetScalars("values");
  double range[2];
  a->GetRange(range);

  vtkImageShiftScale *t=vtkImageShiftScale::New();
  t->SetInputConnection(source->GetOutputPort());
  source->Delete();
  t->SetShift(-range[0]);
  double magnitude=range[1]-range[0];
  if(magnitude==0.0)
    {
    magnitude=1.0;
    }
  t->SetScale(255.0/magnitude);
  t->SetOutputScalarTypeToUnsignedChar();

  t->Update();

  vtkRenderWindow *renWin=vtkRenderWindow::New();
  vtkRenderer *ren1=vtkRenderer::New();
  ren1->SetBackground(0.1,0.4,0.2);

  renWin->AddRenderer(ren1);
  ren1->Delete();
  renWin->SetSize(301,300); // intentional odd and NPOT  width/height

  vtkRenderWindowInteractor *iren=vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  renWin->Delete();

  vtkLightCollection *lights=ren1->GetLights();
  assert("check: lights_empty" && lights->GetNumberOfItems()==0);
  vtkLight *light=vtkLight::New();
  light->SetAmbientColor(0.0,0.0,0.0);
  light->SetDiffuseColor(0.5,0.5,0.5);
  light->SetSpecularColor(0.0,0.0,1.0);
  light->SetIntensity(1.0);
  // positional light is not supported by vtkFixedPointVolumeRayCastMapper
  light->SetLightTypeToHeadlight();
  lights->AddItem(light);
  light->Delete();


  vtkVolumeProperty *volumeProperty;
  vtkVolume *volume;

  vtkFixedPointVolumeRayCastMapper *volumeMapper=vtkFixedPointVolumeRayCastMapper::New();
  volumeMapper->SetSampleDistance(1.0);

  volumeMapper->SetInputConnection(
    t->GetOutputPort());

  volumeProperty=vtkVolumeProperty::New();
  volumeMapper->SetBlendModeToComposite();
  volumeProperty->ShadeOn();
  volumeProperty->SetSpecularPower(128.0);
  volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);

  vtkPiecewiseFunction *compositeOpacity = vtkPiecewiseFunction::New();
  compositeOpacity->AddPoint(0.0,1.0); // 1.0
  compositeOpacity->AddPoint(80.0,1.0); // 1.0
  compositeOpacity->AddPoint(80.1,0.0); // 0.0
  compositeOpacity->AddPoint(255.0,0.0); // 0.0
  volumeProperty->SetScalarOpacity(compositeOpacity);

  vtkColorTransferFunction *color=vtkColorTransferFunction::New();
  color->AddRGBPoint(0.0  ,1.0,1.0,1.0); // blue
  color->AddRGBPoint(40.0  ,1.0,1.0,1.0); // red
  color->AddRGBPoint(255.0,1.0,1.0,1.0); // white
  volumeProperty->SetColor(color);
  color->Delete();

  volume=vtkVolume::New();
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);
  ren1->AddViewProp(volume);

  int retVal;

  ren1->ResetCamera();
  renWin->Render();

  retVal = vtkTesting::Test(argc, argv, renWin, 75);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  volumeMapper->Delete();
  volumeProperty->Delete();
  volume->Delete();
  iren->Delete();
  t->Delete();
  compositeOpacity->Delete();

  return !((retVal == vtkTesting::PASSED) || (retVal == vtkTesting::DO_INTERACTOR));
}
