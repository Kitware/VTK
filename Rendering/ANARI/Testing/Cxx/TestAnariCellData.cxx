// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This tests whether ANARI properly handles cell data
#include "vtkAnariPass.h"
#include "vtkAnariSceneGraph.h"
#include "vtkAnariTestInteractor.h"
#include "vtkAnariTestUtilities.h"
#include <vtkColorTransferFunction.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkImageData.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkLogger.h>
#include <vtkNew.h>
#include <vtkOutlineFilter.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPointDataToCellData.h>
#include <vtkPolyDataMapper.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTestUtilities.h>
#include <vtkTesting.h>
#include <vtkVolumeProperty.h>
#include <vtkXMLImageDataReader.h>

int TestAnariCellData(int argc, char* argv[])
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

  double scalarRange[2];

  vtkNew<vtkActor> outlineActor;
  vtkNew<vtkPolyDataMapper> outlineMapper;
  vtkNew<vtkGPUVolumeRayCastMapper> volumeMapper;

  vtkNew<vtkXMLImageDataReader> reader;
  char* volumeFile = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vase_1comp.vti");
  reader->SetFileName(volumeFile);
  reader->Update();
  delete[] volumeFile;

  vtkNew<vtkPointDataToCellData> pointToCell;
  pointToCell->SetInputConnection(reader->GetOutputPort());
  pointToCell->Update();
  volumeMapper->SetInputConnection(pointToCell->GetOutputPort());

  // Add outline filter
  vtkNew<vtkOutlineFilter> outlineFilter;
  outlineFilter->SetInputConnection(pointToCell->GetOutputPort());
  outlineMapper->SetInputConnection(outlineFilter->GetOutputPort());
  outlineActor->SetMapper(outlineMapper);

  volumeMapper->GetInput()->GetScalarRange(scalarRange);
  volumeMapper->SetSampleDistance(0.1); // TODO: ignored
  volumeMapper->SetAutoAdjustSampleDistances(0);
  volumeMapper->SetBlendModeToComposite(); // TODO: ignored

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetSize(400, 400);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);

  vtkNew<vtkRenderer> ren;
  ren->SetBackground(0.2, 0.2, 0.5);
  renWin->AddRenderer(ren);

  vtkNew<vtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(50, 0.0);
  scalarOpacity->AddPoint(75, 1.0);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOn(); // TODO: ignored
  volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);
  volumeProperty->SetScalarOpacity(scalarOpacity);

  vtkNew<vtkColorTransferFunction> colorTransferFunction;
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(scalarRange[0], 0.6, 0.4, 0.1);
  colorTransferFunction->AddRGBPoint(scalarRange[1], 0.6, 0.4, 0.1);
  volumeProperty->SetColor(colorTransferFunction);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  ren->AddActor(outlineActor);
  ren->AddVolume(volume);

  vtkNew<vtkAnariPass> anariPass;
  ren->SetPass(anariPass);

  SetParameterDefaults(anariPass, ren, useDebugDevice, "TestAnariCellData");
  auto* ar = anariPass->GetAnariRenderer();
  ar->SetParameterf("ambientRadiance", 0.5f);

  renWin->Render();
  ren->ResetCamera();

  auto anariRendererNode = anariPass->GetSceneGraph();
  auto extensions = anariRendererNode->GetAnariDeviceExtensions();

  if (extensions.ANARI_KHR_SPATIAL_FIELD_STRUCTURED_REGULAR)
  {
    int retVal = vtkRegressionTestImage(renWin);

    if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
      vtkNew<vtkAnariTestInteractor> anariStyle;
      anariStyle->SetPipelineControlPoints(ren, anariPass, nullptr);
      anariStyle->SetCurrentRenderer(ren);

      iren->SetInteractorStyle(anariStyle);
      iren->SetDesiredUpdateRate(30.0);
      iren->Start();
    }

    return !retVal;
  }

  std::cout << "Required feature KHR_SPATIAL_FIELD_STRUCTURED_REGULAR not supported." << std::endl;
  return VTK_SKIP_RETURN_CODE;
}
