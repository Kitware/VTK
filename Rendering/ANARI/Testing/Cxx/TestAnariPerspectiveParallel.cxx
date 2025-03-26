// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This test covers switch from perspective to parallel projection.
// This test volume renders a synthetic dataset with unsigned char values,
// with the composite method.

#include "vtkNew.h"
#include "vtkSampleFunction.h"
#include "vtkSphere.h"

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkDataArray.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkImageData.h"
#include "vtkImageShiftScale.h"
#include "vtkLogger.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkVolumeProperty.h"

#include "vtkAnariPass.h"
#include "vtkAnariRendererNode.h"

int TestAnariPerspectiveParallel(int argc, char* argv[])
{
  vtkLogger::SetStderrVerbosity(vtkLogger::Verbosity::VERBOSITY_WARNING);
  std::cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << std::endl;
  bool useDebugDevice = false;

  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "-trace"))
    {
      useDebugDevice = true;
      vtkLogger::SetStderrVerbosity(vtkLogger::Verbosity::VERBOSITY_INFO);
    }
  }

  // Create a spherical implicit function.
  vtkNew<vtkSphere> shape;
  shape->SetRadius(0.1);
  shape->SetCenter(0.0, 0.0, 0.0);

  vtkNew<vtkSampleFunction> source;
  source->SetImplicitFunction(shape);
  source->SetOutputScalarTypeToDouble();
  source->SetSampleDimensions(127, 127, 127); // intentional NPOT dimensions.
  source->SetModelBounds(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
  source->SetCapping(false);
  source->SetComputeNormals(false);
  source->SetScalarArrayName("values");
  source->Update();

  vtkDataArray* a = source->GetOutput()->GetPointData()->GetScalars("values");
  double range[2];
  a->GetRange(range);

  vtkNew<vtkImageShiftScale> imageShiftScale;
  imageShiftScale->SetInputConnection(source->GetOutputPort());
  imageShiftScale->SetShift(-range[0]);
  double magnitude = range[1] - range[0];

  if (magnitude == 0.0)
  {
    magnitude = 1.0;
  }

  imageShiftScale->SetScale(255.0 / magnitude);
  imageShiftScale->SetOutputScalarTypeToUnsignedChar();
  imageShiftScale->Update();

  vtkNew<vtkRenderer> ren1;
  ren1->SetBackground(0.1, 0.4, 0.2);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(ren1);
  renWin->SetSize(301, 300); // intentional odd and NPOT  width/height

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkGPUVolumeRayCastMapper> volumeMapper;
  volumeMapper->SetBlendModeToComposite();
  volumeMapper->SetInputConnection(imageShiftScale->GetOutputPort());

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);

  vtkNew<vtkPiecewiseFunction> compositeOpacity;
  compositeOpacity->AddPoint(0.0, 0.0);
  compositeOpacity->AddPoint(80.0, 1.0);
  compositeOpacity->AddPoint(80.1, 0.0);
  compositeOpacity->AddPoint(255.0, 0.0);
  volumeProperty->SetScalarOpacity(compositeOpacity);

  vtkNew<vtkColorTransferFunction> color;
  color->AddRGBPoint(0.0, 0.0, 0.0, 1.0);
  color->AddRGBPoint(40.0, 1.0, 0.0, 0.0);
  color->AddRGBPoint(255.0, 1.0, 1.0, 1.0);
  volumeProperty->SetColor(color);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);
  ren1->AddViewProp(volume);

  // Attach ANARI render pass
  vtkNew<vtkAnariPass> anariPass;
  ren1->SetPass(anariPass);

  if (useDebugDevice)
  {
    vtkAnariRendererNode::SetUseDebugDevice(1, ren1);
    vtkNew<vtkTesting> testing;

    std::string traceDir = testing->GetTempDirectory();
    traceDir += "/anari-trace";
    traceDir += "/TestAnariPerspectiveParallel";
    vtkAnariRendererNode::SetDebugDeviceDirectory(traceDir.c_str(), ren1);
  }

  vtkAnariRendererNode::SetLibraryName("environment", ren1);
  vtkAnariRendererNode::SetSamplesPerPixel(5, ren1);
  vtkAnariRendererNode::SetLightFalloff(.3, ren1);
  vtkAnariRendererNode::SetUseDenoiser(1, ren1);
  vtkAnariRendererNode::SetCompositeOnGL(1, ren1);

  ren1->ResetCamera();
  // Render composite. Default camera is perpective.
  renWin->Render();

  // Switch to parallel
  vtkCamera* camera = ren1->GetActiveCamera();
  camera->SetParallelProjection(true);
  renWin->Render();

  auto anariRendererNode = anariPass->GetSceneGraph();
  auto extensions = anariRendererNode->GetAnariDeviceExtensions();

  if (extensions.ANARI_KHR_SPATIAL_FIELD_STRUCTURED_REGULAR)
  {
    int retVal = vtkRegressionTestImageThreshold(renWin, 0.05);

    if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
      iren->Start();
    }

    return !retVal;
  }

  std::cout << "Required feature KHR_VOLUME_SCIVIS not supported." << std::endl;
  return VTK_SKIP_RETURN_CODE;
}
