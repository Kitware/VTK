// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * Test 2D transfer function support in ANARI Volume Mapper.  The transfer
 * function is created manually using known value/gradient histogram information
 * of the test data (tooth.hdr). A filter to create these histograms will be
 * added in the future.
 *
 * 2D transfer functions are currently not supported in ANARI and should result
 * in switching to essentially the TF_1D mode and using separate 1D functions
 * for color and opacity.
 */

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkFloatArray.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkImageData.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkNrrdReader.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

#include "vtkAnariPass.h"
#include "vtkAnariRendererNode.h"
#include "vtkAnariTestInteractor.h"

typedef vtkSmartPointer<vtkImageData> Transfer2DPtr;
Transfer2DPtr Create2DTransfer()
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
  {
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
  }

  return image;
}

////////////////////////////////////////////////////////////////////////////////
int TestAnariTransfer2D(int argc, char* argv[])
{
  vtkLogger::SetStderrVerbosity(vtkLogger::Verbosity::VERBOSITY_WARNING);
  bool useDebugDevice = false;

  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "-trace"))
    {
      useDebugDevice = true;
      vtkLogger::SetStderrVerbosity(vtkLogger::Verbosity::VERBOSITY_INFO);
    }
  }

  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  // Load data
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/tooth.nhdr");
  vtkNew<vtkNrrdReader> reader;
  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOn();
  volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);

  vtkDataArray* arr = reader->GetOutput()->GetPointData()->GetScalars();
  double range[2];
  arr->GetRange(range);

  // Prepare 1D Transfer Functions

  // Color transfer function
  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(0, 0.0, 0.0, 0.0);
  ctf->AddRGBPoint(510, 0.4, 0.4, 1.0);
  ctf->AddRGBPoint(640, 1.0, 1.0, 1.0);
  ctf->AddRGBPoint(range[1], 0.9, 0.1, 0.1);

  // opacity transfer function
  vtkNew<vtkPiecewiseFunction> otf;
  otf->AddPoint(0, 0.00);
  otf->AddPoint(510, 0.00);
  otf->AddPoint(640, 0.5);
  otf->AddPoint(range[1], 0.4);

  // maps the gradient magnitude of the scalar value to
  // an opacity multiplier
  vtkNew<vtkPiecewiseFunction> gf;
  gf->AddPoint(0, 0.0);
  gf->AddPoint(3, 0.0);
  gf->AddPoint(6, 1.0);
  gf->AddPoint(range[1] / 4.0, 1.0);

  volumeProperty->SetScalarOpacity(otf);
  volumeProperty->SetGradientOpacity(gf);
  volumeProperty->SetColor(ctf);

  // Prepare 2D Transfer Functions
  Transfer2DPtr tf2d = Create2DTransfer();
  volumeProperty->SetTransferFunction2D(tf2d);

  volumeProperty->SetTransferFunctionMode(vtkVolumeProperty::TF_1D);

  // Setup rendering context
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(512, 512);
  renWin->SetMultiSamples(0);
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren);
  ren->SetBackground(0.0, 0.0, 0.0);

  vtkNew<vtkGPUVolumeRayCastMapper> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());

  // If UseJittering is on, each ray traversal direction will be
  // perturbed slightly using a noise-texture to get rid of wood-grain
  // effect.
  mapper->SetUseJittering(1);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(volumeProperty);
  ren->AddVolume(volume);

  vtkNew<vtkAnariPass> anariPass;
  ren->SetPass(anariPass);

  if (useDebugDevice)
  {
    vtkAnariRendererNode::SetUseDebugDevice(1, ren);
    vtkNew<vtkTesting> testing;

    std::string traceDir = testing->GetTempDirectory();
    traceDir += "/anari-trace";
    traceDir += "/TestAnariTransfer2D";
    vtkAnariRendererNode::SetDebugDeviceDirectory(traceDir.c_str(), ren);
  }

  vtkAnariRendererNode::SetLibraryName("environment", ren);
  vtkAnariRendererNode::SetSamplesPerPixel(6, ren);
  vtkAnariRendererNode::SetLightFalloff(.5, ren);
  vtkAnariRendererNode::SetUseDenoiser(1, ren);
  vtkAnariRendererNode::SetCompositeOnGL(1, ren);

  auto cam = ren->GetActiveCamera();
  cam->SetFocalPoint(85.7721, 88.4044, 33.8576);
  cam->SetPosition(-173.392, 611.09, -102.892);
  cam->SetViewUp(0.130638, -0.194997, -0.972065);
  cam->Roll(180);
  cam->Zoom(1.2);
  renWin->Render();

  auto anariRendererNode = anariPass->GetSceneGraph();
  auto extensions = anariRendererNode->GetAnariDeviceExtensions();

  if (extensions.ANARI_KHR_SPATIAL_FIELD_STRUCTURED_REGULAR)
  {
    int retVal = vtkRegressionTestImageThreshold(renWin, 0.05);

    if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
      iren->Initialize();
      iren->SetDesiredUpdateRate(30.0);
      iren->Start();
    }

    return !retVal;
  }

  std::cout << "Required feature KHR_SPATIAL_FIELD_STRUCTURED_REGULAR not supported." << std::endl;
  return VTK_SKIP_RETURN_CODE;
}
