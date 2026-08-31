// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkGaussianBlurPass.h"
#include "vtkHDRReader.h"
#include "vtkPLYReader.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderPassCollection.h"
#include "vtkRenderStepsPass.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSSAOPass.h"
#include "vtkSkybox.h"
#include "vtkSobelGradientMagnitudePass.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkTexture.h"
#include "vtkToneMappingPass.h"

namespace
{

/**
 * This function renders the data coming from source with the renderPass object attached to the
 * renderer. This function uses a default HDRI file as a background to test the skybox blur with
 * custom render passes after it.
 */
bool TestRenderPass(int argc, char* argv[], vtkSmartPointer<vtkAlgorithm> source,
  vtkSmartPointer<vtkRenderPass> renderPass, const std::string& baselineImageFileName)
{
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(source->GetOutputPort());

  std::string hdrFilePath =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/spiaggia_di_mondello_1k.hdr");
  vtkNew<vtkHDRReader> hdrReader;
  hdrReader->SetFileName(hdrFilePath.c_str());

  vtkNew<vtkTexture> hdrTexture;
  hdrTexture->SetColorModeToDirectScalars();
  hdrTexture->MipmapOn();
  hdrTexture->InterpolateOn();
  hdrTexture->SetInputConnection(hdrReader->GetOutputPort());

  vtkNew<vtkRenderer> renderer;
  renderer->UseImageBasedLightingOn();
  renderer->SetEnvironmentTexture(hdrTexture);
  renderer->SkyboxBlurEnabledOn();
  renderer->SetSkyboxBlurRadius(40.0f);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetInterpolationToPBR();
  actor->GetProperty()->SetColor(1.0, 0.0, 0.0);
  actor->GetProperty()->SetRoughness(0.0);
  renderer->AddActor(actor);

  vtkNew<vtkSkybox> skybox;
  skybox->SetTexture(hdrTexture);
  skybox->SetProjectionToSphere();
  renderer->AddActor(skybox);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);
  renderer->SetPass(renderPass);

  renderWindow->Render();

  // vtkTesting wants the baseline as a path on the host: under emscripten it preloads that
  // file into the sandbox itself, so the sandbox path returned by ExpandDataFileName cannot
  // be used here.
  char* dataRoot = vtkTestUtilities::GetDataRoot(argc, argv);
  std::string baselineImageFilePath = std::string(dataRoot) + "/Data/" + baselineImageFileName;
  delete[] dataRoot;
  vtkNew<vtkTesting> tester;
  tester->AddArgument("-V");
  tester->AddArgument(baselineImageFilePath.c_str());
  tester->SetRenderWindow(renderWindow);

  int regressionTestRes = tester->RegressionTest(0.05);
  return regressionTestRes == vtkTesting::PASSED;
}

}

/**
 * Testing vtkImageProcessingPass based render passes with skybox blur to check if the result image
 * is valid.
 */
int TestSkyboxBlurWithRenderPasses(int argc, char* argv[])
{
  vtkNew<vtkSphereSource> sphereSource;
  sphereSource->SetThetaResolution(16);
  sphereSource->SetPhiResolution(16);

  bool result = true;

  // Basic pass
  {
    vtkNew<vtkRenderStepsPass> basicPasses;

    result &= ::TestRenderPass(argc, argv, sphereSource, basicPasses,
      "TestSkyboxBlurWithRenderPasses_Result_GaussianBlur.png");
  }

  // Gaussian blur pass
  {
    vtkNew<vtkRenderStepsPass> basicPasses;
    vtkNew<vtkGaussianBlurPass> gaussianBlurPass;
    gaussianBlurPass->SetDelegatePass(basicPasses);

    result &= ::TestRenderPass(argc, argv, sphereSource, gaussianBlurPass,
      "TestSkyboxBlurWithRenderPasses_Result_GaussianBlur.png");
  }

  // Sobel gradient magnitude pass
  {
    vtkNew<vtkRenderStepsPass> basicPasses;
    vtkNew<vtkSobelGradientMagnitudePass> sobelGradientMagnitudePass;
    sobelGradientMagnitudePass->SetDelegatePass(basicPasses);

    result &= ::TestRenderPass(argc, argv, sphereSource, sobelGradientMagnitudePass,
      "TestSkyboxBlurWithRenderPasses_Result_SobelGradientMagnitude.png");
  }

  // SSAO pass
  {
    vtkNew<vtkRenderStepsPass> basicPasses;
    vtkNew<vtkSSAOPass> ssaoPass;
    ssaoPass->SetDelegatePass(basicPasses);
    ssaoPass->SetRadius(0.05);
    ssaoPass->SetKernelSize(128);

    std::string sourceFilePath =
      vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dragon.ply");
    vtkNew<vtkPLYReader> reader;
    reader->SetFileName(sourceFilePath.c_str());

    result &= ::TestRenderPass(
      argc, argv, reader, ssaoPass, "TestSkyboxBlurWithRenderPasses_Result_SSAO.png");
  }

  // Tone mapping pass
  {
    vtkNew<vtkRenderStepsPass> basicPasses;
    vtkNew<vtkToneMappingPass> toneMappingPass;
    toneMappingPass->SetDelegatePass(basicPasses);

    result &= ::TestRenderPass(argc, argv, sphereSource, toneMappingPass,
      "TestSkyboxBlurWithRenderPasses_Result_ToneMapping.png");
  }

  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
