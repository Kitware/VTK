// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * Regression test for depth-peeled volume rendering across multiple viewports
 * that share a single render window.
 *
 * Several renderers share one vtkGPUVolumeRayCastMapper volume with depth
 * peeling for volumes enabled. One renderer, anchored at the window's
 * lower-left corner, holds a large opaque sphere and *no* volume.
 *
 * Regression: the volume ray cast captured its opaque depth from the wrong
 * window tile (the tile at the window origin) whenever a renderer's viewport
 * was not anchored at the window origin. As a result the shared volume was
 * carved by the lower-left renderer's opaque sphere in every other viewport.
 * With the fix each renderer reads the opaque depth of its own viewport, so
 * the volumes render intact.
 */

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkImageData.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkNew.h>
#include <vtkOpenGLRenderer.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>
#include <vtkTestUtilities.h>
#include <vtkTestingObjectFactory.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>

#include <algorithm>
#include <cmath>
#include <iostream>

namespace
{

vtkSmartPointer<vtkImageData> CreateSyntheticVolume()
{
  const int dim = 96;
  vtkNew<vtkImageData> img;
  img->SetDimensions(dim, dim, dim);
  img->SetSpacing(1.0, 1.0, 1.0);
  img->SetOrigin(0.0, 0.0, 0.0);
  img->AllocateScalars(VTK_UNSIGNED_CHAR, 1);

  const double cx = (dim - 1) * 0.5;
  const double maxR = std::sqrt(3.0 * cx * cx);
  auto* ptr = static_cast<unsigned char*>(img->GetScalarPointer());
  for (int z = 0; z < dim; ++z)
  {
    for (int y = 0; y < dim; ++y)
    {
      for (int x = 0; x < dim; ++x)
      {
        const double dx = x - cx, dy = y - cx, dz = z - cx;
        const double r = std::sqrt(dx * dx + dy * dy + dz * dz);
        const double t = std::max(0.0, 1.0 - (r / maxR));
        ptr[(static_cast<size_t>(z) * dim + y) * dim + x] =
          static_cast<unsigned char>(std::lround(t * 255.0));
      }
    }
  }
  return img;
}

vtkSmartPointer<vtkVolume> CreateSharedVolume(vtkImageData* volumeData)
{
  vtkNew<vtkGPUVolumeRayCastMapper> mapper;
  mapper->SetInputData(volumeData);

  vtkNew<vtkPiecewiseFunction> opacity;
  opacity->AddPoint(0.0, 0.00);
  opacity->AddPoint(50.0, 0.05);
  opacity->AddPoint(255.0, 0.30);

  vtkNew<vtkColorTransferFunction> color;
  color->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
  color->AddRGBPoint(128.0, 0.8, 0.4, 0.2);
  color->AddRGBPoint(255.0, 1.0, 1.0, 1.0);

  vtkNew<vtkVolumeProperty> property;
  property->SetScalarOpacity(opacity);
  property->SetColor(color);
  property->SetInterpolationTypeToLinear();
  property->ShadeOff();

  vtkSmartPointer<vtkVolume> volume = vtkSmartPointer<vtkVolume>::New();
  volume->SetMapper(mapper);
  volume->SetProperty(property);
  return volume;
}

void ConfigureRenderer(
  vtkRenderer* ren, vtkImageData* volumeData, vtkVolume* sharedVolume, int viewportIndex)
{
  // Viewport 2 is the lower-left renderer: a large opaque sphere and no volume.
  if (viewportIndex != 2)
  {
    ren->AddVolume(sharedVolume);
  }

  int dims[3];
  volumeData->GetDimensions(dims);

  const double offsetX = (viewportIndex == 0 || viewportIndex == 2) ? -15.0 : +15.0;
  const double offsetY = (viewportIndex == 0 || viewportIndex == 1) ? +15.0 : -15.0;

  vtkNew<vtkSphereSource> sphereSrc;
  sphereSrc->SetCenter(dims[0] * 0.5 + offsetX, dims[1] * 0.5 + offsetY, dims[2] * 0.5);
  sphereSrc->SetRadius(viewportIndex == 2 ? 50.0 : 20.0 + 5.0 * viewportIndex);
  sphereSrc->SetThetaResolution(32);
  sphereSrc->SetPhiResolution(32);

  vtkNew<vtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(sphereSrc->GetOutputPort());

  static const double kSphereColors[4][3] = {
    { 1.0, 0.0, 0.0 },
    { 0.0, 1.0, 0.0 },
    { 0.0, 0.0, 1.0 },
    { 1.0, 1.0, 0.0 },
  };
  vtkNew<vtkActor> sphereActor;
  sphereActor->SetMapper(sphereMapper);
  sphereActor->GetProperty()->SetOpacity((viewportIndex == 1 || viewportIndex == 2) ? 1.0 : 0.1);
  sphereActor->GetProperty()->SetColor(const_cast<double*>(kSphereColors[viewportIndex]));
  ren->AddActor(sphereActor);

  ren->SetUseDepthPeeling(true);
  ren->SetMaximumNumberOfPeels(4);
  ren->SetOcclusionRatio(0.0);
  ren->SetUseDepthPeelingForVolumes(true);

  ren->ResetCamera();
  ren->GetActiveCamera()->Azimuth(viewportIndex == 0 ? 45.0 : viewportIndex * 45.0);
  ren->ResetCameraClippingRange();
}

} // end anonymous namespace

int TestGPURayCastDepthPeelingMultiViewport(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(600, 600);
  renWin->SetMultiSamples(0);
  renWin->SetAlphaBitPlanes(1);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);

  // Volume peeling is only supported through the dual depth peeling algorithm.
  vtkNew<vtkRenderer> probe;
  renWin->AddRenderer(probe);
  renWin->Render(); // create the context
  vtkOpenGLRenderer* oglRen = vtkOpenGLRenderer::SafeDownCast(probe);
  const bool supported = oglRen && oglRen->IsDualDepthPeelingSupported();
  renWin->RemoveRenderer(probe);
  if (!supported)
  {
    std::cerr << "Skipping test; volume depth peeling not supported.\n";
    return VTK_SKIP_RETURN_CODE;
  }

  vtkSmartPointer<vtkImageData> volumeData = CreateSyntheticVolume();
  vtkSmartPointer<vtkVolume> sharedVolume = CreateSharedVolume(volumeData);

  const double viewports[4][4] = {
    { 0.0, 0.5, 0.5, 1.0 }, // top-left
    { 0.5, 0.5, 1.0, 1.0 }, // top-right
    { 0.0, 0.0, 0.5, 0.5 }, // bottom-left (opaque sphere, no volume)
    { 0.5, 0.0, 1.0, 0.5 }, // bottom-right
  };
  const double backgrounds[4][3] = {
    { 0.0, 0.0, 0.2 },
    { 0.0, 0.2, 0.0 },
    { 0.1, 0.1, 0.1 },
    { 0.2, 0.0, 0.0 },
  };

  for (int i = 0; i < 4; ++i)
  {
    vtkNew<vtkRenderer> ren;
    ren->SetViewport(const_cast<double*>(viewports[i]));
    ren->SetBackground(const_cast<double*>(backgrounds[i]));
    ConfigureRenderer(ren, volumeData, sharedVolume, i);
    renWin->AddRenderer(ren);
  }

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
