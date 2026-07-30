// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// A volume between two opaque spheres. The near sphere must survive untouched and the far one
// must end up behind the volume.

#include "vtkWebGPUVolumeTestHelpers.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkVolume.h"

int TestWebGPUVolumeMixedGeometry(int argc, char* argv[])
{
  vtkNew<vtkRTAnalyticSource> source;
  source->SetWholeExtent(-31, 31, -31, 31, -31, 31);
  source->Update();

  double range[2];
  source->GetOutput()->GetPointData()->GetScalars()->GetRange(range);

  vtkNew<vtkGPUVolumeRayCastMapper> mapper;
  if (!vtkWebGPUVolumeTest::CheckClass(mapper, "vtkWebGPUVolumeRayCastMapper"))
  {
    return EXIT_FAILURE;
  }
  mapper->SetInputConnection(source->GetOutputPort());

  auto property = vtkWebGPUVolumeTest::MakeProperty(range);
  property->SetScalarOpacityUnitDistance(4.0);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(property);

  const double background[3] = { 0.1, 0.1, 0.15 };
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(background);
  renderer->AddVolume(volume);

  const double nearColor[3] = { 1.0, 0.9, 0.2 };
  const double farColor[3] = { 1.0, 0.2, 0.9 };
  for (const auto& sphere : { std::make_pair(70.0, nearColor), std::make_pair(-70.0, farColor) })
  {
    vtkNew<vtkSphereSource> geometry;
    geometry->SetCenter(0.0, 0.0, sphere.first);
    geometry->SetRadius(20.0);
    geometry->SetThetaResolution(48);
    geometry->SetPhiResolution(48);

    vtkNew<vtkPolyDataMapper> geometryMapper;
    geometryMapper->SetInputConnection(geometry->GetOutputPort());

    vtkNew<vtkActor> actor;
    actor->SetMapper(geometryMapper);
    actor->GetProperty()->SetColor(sphere.second[0], sphere.second[1], sphere.second[2]);
    actor->GetProperty()->SetAmbient(1.0);
    actor->GetProperty()->SetDiffuse(0.0);
    renderer->AddActor(actor);
  }

  vtkNew<vtkRenderWindow> window;
  window->SetSize(500, 400);
  window->AddRenderer(renderer);
  if (!vtkWebGPUVolumeTest::CheckClass(window, "vtkWebGPURenderWindow"))
  {
    return EXIT_FAILURE;
  }
  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(window);

  vtkCamera* camera = renderer->GetActiveCamera();
  camera->SetPosition(0.0, 0.0, 260.0);
  camera->SetFocalPoint(0.0, 0.0, 0.0);
  camera->SetViewUp(0.0, 1.0, 0.0);
  camera->ParallelProjectionOff();
  renderer->ResetCameraClippingRange();
  window->Render();

  const auto frame = vtkWebGPUVolumeTest::Capture(window);

  // Straight down the axis the near sphere sits in front of everything, so the centre pixel has
  // to be its flat colour. Anything else means the volume painted over opaque geometry.
  const unsigned char* centre = frame.At(frame.Width / 2, frame.Height / 2);
  for (int c = 0; c < 3; ++c)
  {
    const int expected = static_cast<int>(nearColor[c] * 255.0 + 0.5);
    if (std::abs(static_cast<int>(centre[c]) - expected) > 20)
    {
      std::cerr << "The centre pixel is " << static_cast<int>(centre[0]) << ", "
                << static_cast<int>(centre[1]) << ", " << static_cast<int>(centre[2])
                << " rather than the colour of the sphere in front of the volume.\n";
      return EXIT_FAILURE;
    }
  }

  // The far sphere is buried in the volume, so nothing on screen should still be its flat colour.
  std::size_t untouched = 0;
  for (std::size_t i = 0; i + 2 < frame.Pixels.size(); i += 3)
  {
    bool matches = true;
    for (int c = 0; c < 3; ++c)
    {
      matches = matches &&
        std::abs(
          static_cast<int>(frame.Pixels[i + c]) - static_cast<int>(farColor[c] * 255.0 + 0.5)) < 8;
    }
    untouched += matches ? 1 : 0;
  }
  if (untouched > 32)
  {
    std::cerr << untouched
              << " pixels kept the colour of the sphere behind the volume, so the volume was not "
                 "composited over it.\n";
    return EXIT_FAILURE;
  }

  const int result = vtkRegressionTestImage(window);
  if (result == vtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }
  return !result;
}
