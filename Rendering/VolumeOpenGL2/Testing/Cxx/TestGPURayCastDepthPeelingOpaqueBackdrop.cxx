// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkDataArray.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPlaneSource.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

int TestGPURayCastDepthPeelingOpaqueBackdrop(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> window;
  window->SetSize(300, 300);
  window->SetMultiSamples(0);
  window->SetAlphaBitPlanes(1);
  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(window);
  vtkNew<vtkOpenGLRenderer> renderer;
  window->AddRenderer(renderer);
  window->Render();
  if (!renderer->IsDualDepthPeelingSupported())
  {
    return VTK_SKIP_RETURN_CODE;
  }

  vtkNew<vtkImageData> image;
  image->SetDimensions(21, 21, 21);
  image->SetOrigin(-1.0, -1.0, -1.0);
  image->SetSpacing(0.1, 0.1, 0.1);
  image->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
  image->GetPointData()->GetScalars()->FillComponent(0, 1);
  vtkNew<vtkGPUVolumeRayCastMapper> volumeMapper;
  volumeMapper->SetInputData(image);
  volumeMapper->AutoAdjustSampleDistancesOff();
  volumeMapper->SetSampleDistance(0.05);
  vtkNew<vtkColorTransferFunction> color;
  color->AddRGBPoint(1, 0.1, 0.3, 1.0);
  vtkNew<vtkPiecewiseFunction> opacity;
  opacity->AddPoint(1, 0.5);
  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->SetColor(color);
  volumeProperty->SetScalarOpacity(opacity);
  volumeProperty->SetInterpolationTypeToLinear();
  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);
  renderer->AddVolume(volume);

  vtkNew<vtkPlaneSource> backdrop;
  backdrop->SetOrigin(-1.2, -1.2, -1.1);
  backdrop->SetPoint1(1.2, -1.2, -1.1);
  backdrop->SetPoint2(-1.2, 1.2, -1.1);
  vtkNew<vtkPolyDataMapper> backdropMapper;
  backdropMapper->SetInputConnection(backdrop->GetOutputPort());
  vtkNew<vtkActor> backdropActor;
  backdropActor->SetMapper(backdropMapper);
  backdropActor->GetProperty()->SetColor(0.1, 0.1, 0.1);
  backdropActor->GetProperty()->LightingOff();
  renderer->AddActor(backdropActor);

  // The left half has only volume and opaque geometry; the right also has a translucent layer.
  vtkNew<vtkPlaneSource> translucent;
  translucent->SetOrigin(0.0, -1.0, 0.2);
  translucent->SetPoint1(1.0, -1.0, 0.2);
  translucent->SetPoint2(0.0, 1.0, 0.2);
  vtkNew<vtkPolyDataMapper> translucentMapper;
  translucentMapper->SetInputConnection(translucent->GetOutputPort());
  vtkNew<vtkActor> translucentActor;
  translucentActor->SetMapper(translucentMapper);
  translucentActor->GetProperty()->SetColor(1.0, 0.0, 0.0);
  translucentActor->GetProperty()->SetOpacity(0.5);
  translucentActor->GetProperty()->LightingOff();
  renderer->AddActor(translucentActor);

  renderer->UseDepthPeelingOn();
  renderer->UseDepthPeelingForVolumesOn();
  renderer->SetMaximumNumberOfPeels(4);
  renderer->SetOcclusionRatio(0.0);
  renderer->GetActiveCamera()->ParallelProjectionOn();
  renderer->GetActiveCamera()->SetPosition(0.0, 0.0, 5.0);
  renderer->GetActiveCamera()->SetFocalPoint(0.0, 0.0, 0.0);
  renderer->GetActiveCamera()->SetParallelScale(1.3);
  renderer->ResetCameraClippingRange();
  window->Render();

  int result = vtkRegressionTestImage(window);
  if (result == vtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }
  return !result;
}
