// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkDataSet.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkImageResize.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkVolume16Reader.h"
#include "vtkVolumeProperty.h"

#include "vtkAnariSceneGraph.h"
#include "vtkAnariTestUtilities.h"

/**
 * This test verifies that we can hot swap ANARI Volume rendering and
 * GL volume rendering for float volume data.
 */
int TestAnariGPUVolumeRayCastMapper(int argc, char* argv[])
{
  bool useDebugDevice = false;

  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "--trace"))
    {
      useDebugDevice = true;
    }
  }

  vtkNew<vtkVolume16Reader> headReader;
  headReader->SetDataDimensions(64, 64);
  headReader->SetImageRange(1, 93);
  headReader->SetDataByteOrderToLittleEndian();
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");
  headReader->SetFilePrefix(fname);
  headReader->SetDataSpacing(3.2, 3.2, 1.5);
  delete[] fname;

  // Upsample data
  vtkNew<vtkImageResize> resample;
  resample->SetInputConnection(headReader->GetOutputPort());
  resample->SetResizeMethodToOutputDimensions();
  resample->SetOutputDimensions(512, 512, 512);
  resample->Update();

  vtkNew<vtkGPUVolumeRayCastMapper> volumeMapper;
  volumeMapper->SetInputConnection(resample->GetOutputPort());

  double scalarRange[2];
  volumeMapper->GetInput()->GetScalarRange(scalarRange);
  volumeMapper->SetBlendModeToComposite();

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderer> ren;
  ren->SetBackground(0.2, 0.2, 0.5);

  renWin->AddRenderer(ren);
  renWin->SetSize(400, 400);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkPiecewiseFunction> pf;
  pf->AddPoint(0, 0.00);
  pf->AddPoint(500, 0.02);
  pf->AddPoint(1000, 0.02);
  pf->AddPoint(1150, 0.85);

  vtkNew<vtkPiecewiseFunction> gf;
  gf->AddPoint(0, 0.0);
  gf->AddPoint(90, 0.5);
  gf->AddPoint(100, 0.7);

  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(0, 0.0, 0.0, 0.0);
  ctf->AddRGBPoint(500, 1.0, 0.5, 0.3);
  ctf->AddRGBPoint(1000, 1.0, 0.5, 0.3);
  ctf->AddRGBPoint(1150, 1.0, 1.0, 0.9);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);
  volumeProperty->SetScalarOpacity(pf);
  volumeProperty->SetGradientOpacity(gf);
  volumeProperty->SetColor(ctf);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);
  ren->AddVolume(volume);

  vtkAnariTestUtilities::SetParameterDefaults(renWin, useDebugDevice, "TestAnariVolumeRenderer");

  auto cam = ren->GetActiveCamera();
  cam->SetFocalPoint(85.7721, 88.4044, 33.8576);
  cam->SetPosition(-173.392, 611.09, -102.892);
  cam->SetViewUp(0.130638, -0.194997, -0.972065);

  renWin->Render();

  const auto& extensions = vtkAnariTestUtilities::GetDeviceExtensions(renWin);
  if (extensions.ANARI_KHR_SPATIAL_FIELD_STRUCTURED_REGULAR)
  {
    int retVal = vtkRegressionTestImage(renWin);

    if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
      iren->Initialize();
      iren->SetDesiredUpdateRate(30.0);
      iren->Start();
    }

    return !retVal;
  }

  vtkLogF(WARNING, "Required feature KHR_VOLUME_TRANSFER_FUNCTION1D not supported.");
  return VTK_SKIP_RETURN_CODE;
}
