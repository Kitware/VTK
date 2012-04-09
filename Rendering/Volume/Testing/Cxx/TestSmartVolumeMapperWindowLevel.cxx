/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSmartVolumeMapperWindowLevel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers the smart volume mapper and composite method with a
// customed window/level values (brightness/contrast).
// This test volume renders a synthetic dataset with unsigned char values,
// with the composite method.

#include "vtkSphere.h"
#include "vtkSampleFunction.h"

#include "vtkSmartVolumeMapper.h"
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

int TestSmartVolumeMapperWindowLevel(int argc,
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
  source->SetModelBounds(-1.0,1.0,-1.0,1.0,-1.0,1.0);
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
  ren1->SetBackground(0.0,0.0,0.5);
  ren1->SetViewport(0.0,0.0,0.33,1.0);
  renWin->AddRenderer(ren1);
  ren1->Delete();

  vtkRenderer *ren2=vtkRenderer::New();
  ren2->SetBackground(0.5,0.5,0.5);
  ren2->SetViewport(0.33,0.0,0.66,1.0);
  renWin->AddRenderer(ren2);
  ren2->Delete();

  vtkRenderer *ren3=vtkRenderer::New();
  ren3->SetBackground(0.0,0.5,0.0);
  ren3->SetViewport(0.66,0.0,1.0,1.0);
  renWin->AddRenderer(ren3);
  ren3->Delete();

  renWin->SetSize(301,300); // intentional odd and NPOT  width/height

  vtkRenderWindowInteractor *iren=vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  renWin->Delete();

  renWin->Render(); // make sure we have an OpenGL context.

  vtkSmartVolumeMapper *volumeMapper;
  vtkVolumeProperty *volumeProperty;
  vtkVolume *volume;

  volumeMapper=vtkSmartVolumeMapper::New();
  volumeMapper->SetBlendModeToComposite();
  volumeMapper->SetInputConnection(
    t->GetOutputPort());

  volumeProperty=vtkVolumeProperty::New();
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);

  vtkPiecewiseFunction *compositeOpacity = vtkPiecewiseFunction::New();
  compositeOpacity->AddPoint(0.0,0.0);
  compositeOpacity->AddPoint(80.0,1.0);
  compositeOpacity->AddPoint(80.1,0.0);
  compositeOpacity->AddPoint(255.0,0.0);
  volumeProperty->SetScalarOpacity(compositeOpacity); // composite first.

  vtkColorTransferFunction *color=vtkColorTransferFunction::New();
  color->AddRGBPoint(0.0  ,0.0,0.0,1.0);
  color->AddRGBPoint(40.0  ,1.0,0.0,0.0);
  color->AddRGBPoint(255.0,1.0,1.0,1.0);
  volumeProperty->SetColor(color);
  color->Delete();

  volume=vtkVolume::New();
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);
  ren1->AddViewProp(volume);

  vtkSmartVolumeMapper *volumeMapper2;
  vtkVolume *volume2;

  volumeMapper2=vtkSmartVolumeMapper::New();
  volumeMapper2->SetBlendModeToComposite();
  volumeMapper2->SetInputConnection(
    t->GetOutputPort());
  // 3D texture mode.
  volumeMapper2->SetRequestedRenderModeToRayCastAndTexture();

  volume2=vtkVolume::New();
  volume2->SetMapper(volumeMapper2);
  volume2->SetProperty(volumeProperty);
  ren2->AddViewProp(volume2);

  vtkSmartVolumeMapper *volumeMapper3;
  vtkVolume *volume3;

  volumeMapper3=vtkSmartVolumeMapper::New();
  volumeMapper3->SetBlendModeToComposite();
  volumeMapper3->SetInputConnection(
    t->GetOutputPort());
  // Software mode
  volumeMapper3->SetRequestedRenderModeToRayCast();

  volume3=vtkVolume::New();
  volume3->SetMapper(volumeMapper3);
  volume3->SetProperty(volumeProperty);
  ren3->AddViewProp(volume3);


  int retVal;

  ren1->ResetCamera();
  ren2->ResetCamera();
  ren3->ResetCamera();

  volumeMapper->SetFinalColorLevel(0.25);
  volumeMapper->SetFinalColorWindow(0.5);

  volumeMapper2->SetFinalColorLevel(0.25);
  volumeMapper2->SetFinalColorWindow(0.5);

  volumeMapper3->SetFinalColorLevel(0.25);
  volumeMapper3->SetFinalColorWindow(0.5);

  renWin->Render();

  retVal = vtkTesting::Test(argc, argv, renWin, 90);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  volumeMapper3->Delete();
  volume3->Delete();

  volumeMapper2->Delete();
  volume2->Delete();

  volumeMapper->Delete();
  volumeProperty->Delete();
  volume->Delete();

  iren->Delete();
  t->Delete();
  compositeOpacity->Delete();

  return !((retVal == vtkTesting::PASSED) || (retVal == vtkTesting::DO_INTERACTOR));
}
