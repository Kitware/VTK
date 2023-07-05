// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Description
// This is a basic test that creates and volume renders the wavelet dataset.

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkFloatArray.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkImageData.h"
#include "vtkLight.h"
#include "vtkNew.h"
#include "vtkNrrdReader.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestErrorObserver.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkTimerLog.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

typedef vtkSmartPointer<vtkImageData> Transfer2DPtr;
Transfer2DPtr Create2DTransferTooth()
{
  int bins[2] = { 256, 256 };
  Transfer2DPtr image = Transfer2DPtr::New();
  image->SetDimensions(bins[0], bins[1], 1);
  image->AllocateScalars(VTK_FLOAT, 4);
  vtkFloatArray* arr = vtkFloatArray::SafeDownCast(image->GetPointData()->GetScalars());

  // Initialize to zero
  void* dataPtr = arr->GetVoidPointer(0);
  memset(dataPtr, 0, bins[0] * bins[1] * 4 * sizeof(float));

  // Setting RGBA [1.0, 0,0, 0.05] for a square in the histogram (known)
  // containing some of the interesting edges (e.g. tooth root).
  for (int j = 0; j < bins[1]; j++)
    for (int i = 0; i < bins[0]; i++)
    {
      if (i > 130 && i < 190 && j < 50)
      {
        double const jFactor = 256.0 / 50;

        vtkIdType const index = bins[0] * j + i;
        double const red = static_cast<double>(i) / bins[0];
        double const green = jFactor * static_cast<double>(j) / bins[1];
        double const blue = jFactor * static_cast<double>(j) / bins[1];
        double const alpha = 0.25 * jFactor * static_cast<double>(j) / bins[0];

        double color[4] = { red, green, blue, alpha };
        arr->SetTuple(index, color);
      }
    }

  return image;
}

//-----------------------------------------------------
int TestGPUVolumeRayCastMapperManyLights(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  // Load data
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/tooth.nhdr");
  vtkNew<vtkNrrdReader> reader;
  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

  vtkNew<vtkTest::ErrorObserver> errorObserver;

  vtkNew<vtkGPUVolumeRayCastMapper> volumeMapper;
  volumeMapper->SetAutoAdjustSampleDistances(0);
  volumeMapper->SetSampleDistance(0.5);
  volumeMapper->SetInputConnection(reader->GetOutputPort());
  volumeMapper->AddObserver(vtkCommand::ErrorEvent, errorObserver);

  vtkNew<vtkVolumeProperty> volumeProperty;

  Transfer2DPtr tf2d = Create2DTransferTooth();
  volumeProperty->SetShade(1);
  volumeProperty->SetTransferFunctionModeTo2D();
  volumeProperty->SetTransferFunction2D(tf2d);
  volumeProperty->SetScalarOpacityUnitDistance(1.732);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  // Create the renderwindow, interactor and renderer
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetMultiSamples(0);
  renderWindow->SetSize(401, 399); // NPOT size
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);
  vtkNew<vtkRenderer> renderer;
  renderer->SetTwoSidedLighting(false);
  renderWindow->AddRenderer(renderer);

  std::vector<std::array<float, 6>> lights = { { { 15.0, -46.0, -22.0, 0.0, 0.0, 0.0 },
    { 15.0, -46.0, -22.0, 0.0, 0.0, 0.0 }, { 107, 10, 235, 42, 52, -9 },
    { 107, 10, 235, 42, 52, -9 }, { 100, 218, 215, 74, 85, 120 }, { -19, 44, -99, 12, 46, 8 },
    { 249, -8, 157, 252, -266, -120 }, { 149, 104, -50, 85, 69, 67 } } };

  renderer->RemoveAllLights();

  int idx = 0;
  for (auto& lightInfo : lights)
  {
    vtkNew<vtkLight> light;
    // everything so the volume isn't lighted
    light->SetLightTypeToSceneLight();
    light->SetPositional(idx % 2);
    light->SetPosition(lightInfo.data());
    light->SetFocalPoint(lightInfo.data() + 3);
    light->SetConeAngle(60.0);
    light->SetIntensity(1.0 / (idx + 1));
    renderer->AddLight(light);

    idx++;
  }

  renderer->ResetCamera();
  renderer->GetActiveCamera()->SetPosition(179, -372, -18);
  renderer->GetActiveCamera()->SetFocalPoint(38, 88, 89);
  renderer->GetActiveCamera()->SetViewUp(-0.22, -0.29, 0.93);

  renderer->AddVolume(volume);
  renderer->ResetCamera();
  renderWindow->Render();

  int retVal = vtkTesting::Test(argc, argv, renderWindow, 90);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !((retVal == vtkTesting::PASSED) || (retVal == vtkTesting::DO_INTERACTOR));
}
