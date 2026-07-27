// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <vtkCameraPass.h>
#include <vtkColorTransferFunction.h>
#include <vtkFramebufferPass.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkNew.h>
#include <vtkOpenGLGPUVolumeRayCastMapper.h>
#include <vtkPiecewiseFunction.h>
#include <vtkRTAnalyticSource.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTextureObject.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkVolumetricPass.h>

//------------------------------------------------------------------------------
int TestGPURayCastDepthTextureFormat(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkNew<vtkRTAnalyticSource> data;
  data->SetWholeExtent(-100, 100, -100, 100, -100, 100);
  data->Update();

  // depth texture format to use in the render pass.
  // should match with the volume mapper format.
  constexpr int depthTextureFormat = vtkTextureObject::Float32;

  vtkNew<vtkOpenGLGPUVolumeRayCastMapper> mapper;
  mapper->SetInputConnection(data->GetOutputPort());
  mapper->SetDepthTextureFormat(depthTextureFormat);

  vtkNew<vtkColorTransferFunction> colorTransferFunction;
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(150.0, 0.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(255.0, 1.0, 0.0, 0.0);

  vtkNew<vtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(150.0, 0.0);
  scalarOpacity->AddPoint(255.0, 1.0);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->SetInterpolationTypeToLinear();
  volumeProperty->SetColor(colorTransferFunction);
  volumeProperty->SetScalarOpacity(scalarOpacity);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(volumeProperty);

  vtkNew<vtkVolumetricPass> volumePass;

  vtkNew<vtkCameraPass> camPass;
  camPass->SetDelegatePass(volumePass);

  vtkNew<vtkFramebufferPass> framebufferPass;
  framebufferPass->SetColorFormat(vtkTextureObject::Fixed8);
  framebufferPass->SetDepthFormat(depthTextureFormat);
  framebufferPass->SetDelegatePass(camPass);

  vtkNew<vtkRenderer> renderer;
  renderer->SetPass(framebufferPass);
  renderer->AddVolume(volume);
  renderer->SetBackground(0.5, 0.5, 0.5);
  renderer->ResetCamera();

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(800, 600);
  renderWindow->AddRenderer(renderer);

  vtkNew<vtkInteractorStyleTrackballCamera> style;

  vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);
  renderWindowInteractor->SetInteractorStyle(style);

  renderWindow->Render();
  renderWindowInteractor->Start();

  return 0;
}
