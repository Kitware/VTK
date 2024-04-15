// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * Designed to test paraview/paraview#19012: when the array to volume render
 * with is changed, the volume mapper must update correctly.
 */

#include <vtkArrayCalculator.h>
#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkImageData.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkNew.h>
#include <vtkOpenGLRenderer.h>
#include <vtkPiecewiseFunction.h>
#include <vtkProperty.h>
#include <vtkRTAnalyticSource.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTestUtilities.h>
#include <vtkTesting.h>
#include <vtkTestingObjectFactory.h>
#include <vtkTransform.h>
#include <vtkVolumeProperty.h>

int TestGPURayCastModelTransformMatrix(int argc, char* argv[])
{
  vtkNew<vtkRTAnalyticSource> rtSource;
  rtSource->SetWholeExtent(-10, 10, -10, 10, -10, 10);

  vtkNew<vtkGPUVolumeRayCastMapper> mapper;
  mapper->SetInputConnection(rtSource->GetOutputPort());
  mapper->AutoAdjustSampleDistancesOn();
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SelectScalarArray("RTData");

  vtkNew<vtkColorTransferFunction> colorTransferFunction;
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(0, 0.23137254902, 0.298039215686, 0.752941176471);
  colorTransferFunction->AddRGBPoint(125, 0.865, 0.865, 0.865);
  colorTransferFunction->AddRGBPoint(250.0, 0.705882352941, 0.0156862745098, 0.149019607843);

  vtkNew<vtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(0, 0.0);
  scalarOpacity->AddPoint(250, 0.5);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->SetInterpolationTypeToLinear();
  volumeProperty->SetColor(colorTransferFunction);
  volumeProperty->SetScalarOpacity(scalarOpacity);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(volumeProperty);

  vtkNew<vtkRenderer> renderer;
  renderer->AddVolume(volume);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(800, 600);
  renderWindow->AddRenderer(renderer);

  vtkNew<vtkInteractorStyleTrackballCamera> style;
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);
  iren->SetInteractorStyle(style);

  // Use camera's ModelTransformMatrix to rotate 60 degrees
  vtkCamera* cam = renderer->GetActiveCamera();
  vtkNew<vtkTransform> transform;
  transform->Identity();
  transform->RotateY(60);
  cam->SetModelTransformMatrix(transform->GetMatrix());

  renderWindow->Render();
  renderer->ResetCamera();

  iren->Initialize();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return EXIT_SUCCESS;
}
