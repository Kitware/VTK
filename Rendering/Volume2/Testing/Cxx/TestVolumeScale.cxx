/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastAdditive.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers additive method.
// This test volume renders a synthetic dataset with unsigned char values,
// with the additive method.

#include <vtkSphere.h>
#include <vtkSampleFunction.h>

//#include <vtkTestUtilities.h>

#include <vtkImageChangeInformation.h>
#include <vtkColorTransferFunction.h>
#include <vtkCommand.h>
#include <vtkFixedPointVolumeRayCastMapper.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkPiecewiseFunction.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkVolumeProperty.h>
#include <vtkCamera.h>
//#include <vtkRegressionTestImage.h>
#include <vtkImageShiftScale.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>
#include <vtkRTAnalyticSource.h>
#include <vtkNew.h>
#include <vtkSphereSource.h>
#include <vtkPolyDataMapper.h>

#include <vtkSmartPointer.h>
#include <vtksys/SystemTools.hxx>

#include <vtkImageReader.h>
#include <vtkStructuredPointsReader.h>

#include <vtkSinglePassVolumeMapper.h>

#include <vtkOutlineFilter.h>
#include <vtkTimerLog.h>
#include <vtkXMLImageDataReader.h>

#include <cstdlib>

int TestVolumeScale(int argc, char *argv[])
{
  double scalarRange[2];

  vtkNew<vtkActor> outlineActor;
  vtkNew<vtkPolyDataMapper> outlineMapper;
  vtkNew<vtkSinglePassVolumeMapper> volumeMapper;

  vtkNew<vtkXMLImageDataReader> reader;
  reader->SetFileName(arg.c_str());
  reader->Update();

  vtkSmartPointer<vtkImageChangeInformation> changeInformation =
    vtkSmartPointer<vtkImageChangeInformation>::New();
  changeInformation->SetInputConnection(reader->GetOutputPort());
  changeInformation->SetOutputSpacing(1, 2, 3);
  changeInformation->SetOutputOrigin(10, 20, 30);
  changeInformation->Update();
  volumeMapper->SetInputConnection(changeInformation->GetOutputPort());

  // Add outline filter
  vtkSmartPointer<vtkOutlineFilter> outlineFilter =
    vtkSmartPointer<vtkOutlineFilter>::New();
  outlineFilter->SetInputConnection(changeInformation->GetOutputPort());
  outlineMapper->SetInputConnection(outlineFilter->GetOutputPort());
  outlineActor->SetMapper(outlineMapper);

  volumeMapper->GetInput()->GetScalarRange(scalarRange);
  volumeMapper->SetBlendModeToComposite();

  vtkNew<vtkRenderWindow> renWin;
  ren->SetBackground(0.2, 0.2, 0.5);

  // Intentional odd and NPOT  width/height
  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren.GetPointer());
  renWin->SetSize(400, 400);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  // Make sure we have an OpenGL context.
  renWin->Render();

  vtkNew<vtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(scalarRange[0], 0.0);
  scalarOpacity->AddPoint(scalarRange[1], 1.0);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);

  volumeProperty->SetScalarOpacity(scalarOpacity);

  vtkColorTransferFunction* colorTransferFunction =
    volumeProperty->GetRGBTransferFunction(0);
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(scalarRange[0], 0.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(scalarRange[1], 1.0, 1.0, 1.0);

  vtkSmartPointer<vtkVolume> volume = vtkSmartPointer<vtkVolume>::New();
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  /// Rotate the volume for testing purposes
  volume->RotateY(45.0);
  outlineActor->RotateY(45.0);

  /// Add sphere for testing
  vtkSmartPointer<vtkSphereSource> sphereSource = vtkSmartPointer<vtkSphereSource>::New();
  sphereSource->SetCenter(100, 10, 10);
  sphereSource->SetRadius(100.0);
  vtkNew<vtkPolyDataMapper> sphereMapper = vtkNew<vtkPolyDataMapper>::New();
  vtkNew<vtkActor> sphereActor = vtkNew<vtkActor>::New();
  sphereMapper->SetInputConnection(sphereSource->GetOutputPort());
  sphereActor->SetMapper(sphereMapper);

  ren->AddViewProp(volume);
  ren->AddActor(outlineActor);
  ren->AddActor(sphereActor);
  ren->ResetCamera();

  renWin->Render();
  ren->ResetCamera();

  /// Testing code
  if (testing)
    {
    vtkNew<vtkTimerCallback> cb;
    cb->Set(renWin, ren);
    iren->AddObserver(vtkCommand::TimerEvent, cb.GetPointer());
    iren->CreateRepeatingTimer(10);
    }

  iren->Initialize();
  iren->Start();
}

