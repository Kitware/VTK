// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * Sets two inputs in vtkGPUVolumeRayCastMapper and uses a vtkMultiVolume instance to render the
 * two inputs simultaneously. Each input is a 4-component RGBA volume.
 */
#include "vtkCamera.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkImageData.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkLight.h"
#include "vtkMultiVolume.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkVolumeProperty.h"

namespace
{

vtkSmartPointer<vtkImageData> CreateQuarterCylinderImageData(
  const int radius, const int height, const unsigned int rgba[4])
{
  vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
  image->SetDimensions(radius, radius, height);
  image->AllocateScalars(VTK_UNSIGNED_CHAR, 4);

  for (vtkIdType k = 0; k < height; k++)
  {
    for (vtkIdType i = 0; i < radius; i++)
    {
      for (vtkIdType j = 0; j < radius; j++)
      {
        for (vtkIdType c = 0; c < 4; c++)
        {
          if (i * i + j * j < radius * radius)
          {
            image->SetScalarComponentFromDouble(i, j, k, c, rgba[c]);
          }
          else
          {
            image->SetScalarComponentFromDouble(i, j, k, c, 0);
          }
        }
      }
    }
  }
  return image;
}

}

//------------------------------------------------------------------------------
int TestGPURayCastMultiVolumeRGBA(int argc, char* argv[])
{
  // Create data
  constexpr unsigned int red[4] = { 255, 0, 0, 255 };
  vtkSmartPointer<vtkImageData> image = CreateQuarterCylinderImageData(50, 100, red);

  constexpr unsigned int green[4] = { 0, 255, 0, 255 };
  vtkSmartPointer<vtkImageData> image1 = CreateQuarterCylinderImageData(100, 50, green);

  // Volume 0 (thin cylinder)
  // ------------------------
  vtkNew<vtkPiecewiseFunction> pwf;
  pwf->AddPoint(0, 0.0);
  pwf->AddPoint(255, 1.0);

  vtkNew<vtkVolume> volume;
  volume->GetProperty()->SetScalarOpacity(pwf);
  volume->GetProperty()->SetInterpolationTypeToLinear();
  volume->GetProperty()->ShadeOn();
  volume->GetProperty()->SetDiffuse(1.0);
  volume->GetProperty()->SetAmbient(1.0);
  volume->GetProperty()->SetSpecular(1.0);
  // The first three components directly represent RGB (no lookup table).
  // The 4th component will be passed through the scalar opacity function.
  volume->GetProperty()->IndependentComponentsOff();

  // Volume 1 (thick cylinder)
  // -------------------------
  vtkNew<vtkPiecewiseFunction> pwf1;
  pwf1->AddPoint(0, 0.0);
  pwf1->AddPoint(255, 0.05);

  vtkNew<vtkVolume> volume1;
  volume1->GetProperty()->SetScalarOpacity(pwf1);
  volume1->GetProperty()->SetInterpolationTypeToLinear();
  volume1->GetProperty()->ShadeOn();
  volume1->GetProperty()->SetDiffuse(1.0);
  volume1->GetProperty()->SetAmbient(1.0);
  volume1->GetProperty()->SetSpecular(1.0);
  // The first three components directly represent RGB (no lookup table).
  // The 4th component will be passed through the scalar opacity function.
  volume1->GetProperty()->IndependentComponentsOff();

  // Multi volume instance
  // ---------------------
  vtkNew<vtkMultiVolume> multiVolume;
  vtkNew<vtkGPUVolumeRayCastMapper> mapper;
  multiVolume->SetMapper(mapper);

  mapper->SetInputDataObject(0, image);
  multiVolume->SetVolume(volume, 0);

  mapper->SetInputDataObject(2, image1);
  multiVolume->SetVolume(volume1, 2);

  // Rendering
  // ---------
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(300, 300);
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren);
  ren->SetBackground(1.0, 1.0, 1.0);

  ren->AddVolume(multiVolume);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);

  vtkNew<vtkLight> light;
  light->SetLightTypeToHeadlight();
  light->SetAmbientColor(1, 1, 1);
  ren->RemoveAllLights();
  ren->AutomaticLightCreationOff();
  ren->AddLight(light);

  auto cam = ren->GetActiveCamera();
  cam->SetFocalPoint(0.0, 0.0, 50.0);
  cam->SetPosition(275.0, 275.0, 75.0);
  cam->SetViewUp(0.0, 0.0, 1.0);

  int retVal = vtkTesting::Test(argc, argv, renWin, 90);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !((retVal == vtkTesting::PASSED) || (retVal == vtkTesting::DO_INTERACTOR));
}
