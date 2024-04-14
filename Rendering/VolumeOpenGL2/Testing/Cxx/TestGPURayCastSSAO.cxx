// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkOpenGLGPUVolumeRayCastMapper.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderStepsPass.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSSAOPass.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkVolume.h"
#include "vtkVolume16Reader.h"
#include "vtkVolumeProperty.h"

//------------------------------------------------------------------------------
vtkSmartPointer<vtkImageData> loadImage(int argc, char* argv[])
{
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");

  vtkNew<vtkVolume16Reader> volumeReader;
  volumeReader->SetDataDimensions(64, 64);
  volumeReader->SetDataByteOrderToLittleEndian();
  volumeReader->SetImageRange(1, 93);
  volumeReader->SetDataSpacing(3.2, 3.2, 1.5);
  volumeReader->SetFilePrefix(fname);
  volumeReader->Update();
  delete[] fname;

  return volumeReader->GetOutput();
}

//------------------------------------------------------------------------------
int TestGPURayCastSSAO(int argc, char* argv[])
{
  vtkSmartPointer<vtkImageData> volumeData = loadImage(argc, argv);

  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.3, 0.4, 0.6);
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(600, 600);
  renderWindow->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  const double* bounds = volumeData->GetBounds();

  vtkNew<vtkOpenGLGPUVolumeRayCastMapper> volumeMapper;
  volumeMapper->SetInputData(volumeData);
  volumeMapper->AutoAdjustSampleDistancesOff();
  volumeMapper->LockSampleDistanceToInputSpacingOn();

  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper);
  renderer->AddActor(volume);

  // Shading must be turned on to enable SSAO on volumes
  volume->GetProperty()->ShadeOn();
  // Linear interpolation produces smoother results
  volume->GetProperty()->SetInterpolationTypeToLinear();

  const double* range = volumeData->GetScalarRange();

  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(range[0], 0, 0, 0);
  ctf->AddRGBPoint(0.5 * (range[0] + range[1]), 1, 1, 1);
  ctf->AddRGBPoint(range[1], 1, 1, 1);
  volume->GetProperty()->SetColor(ctf);

  vtkNew<vtkPiecewiseFunction> pwf;
  pwf->AddPoint(range[0], 0);
  pwf->AddPoint(0.3 * (range[0] + range[1]), 0);
  pwf->AddPoint(0.4 * (range[0] + range[1]), 1);
  volume->GetProperty()->SetScalarOpacity(pwf);

  vtkNew<vtkSphereSource> source;
  source->SetRadius(0.25 * (bounds[1] - bounds[0]));
  source->SetCenter(bounds[0], 0.25 * (bounds[2] + bounds[3]), 0.25 * (bounds[4] + bounds[5]));

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(source->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  renderWindow->SetMultiSamples(0);

  // Render passes setup
  vtkNew<vtkRenderStepsPass> basicPasses;
  vtkNew<vtkSSAOPass> ssao;
  ssao->SetRadius(40);
  ssao->SetKernelSize(128);
  ssao->SetBias(0.01);
  ssao->BlurOn();
  // The depth format must be Fixed32 for the volume mapper to successfully copy the depth texture
  ssao->SetDepthFormat(vtkTextureObject::Fixed32);
  ssao->SetVolumeOpacityThreshold(0.95);
  ssao->SetDelegatePass(basicPasses);

  renderer->SetPass(ssao);

  // Camera setup
  renderer->GetActiveCamera()->SetViewUp(0, 0, -1);
  renderer->GetActiveCamera()->SetPosition(0, 10, 0);
  renderer->GetActiveCamera()->OrthogonalizeViewUp();
  renderer->GetActiveCamera()->Yaw(-40);
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Zoom(1.5);
  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
