// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAnariRenderWindow.h"
#include "vtkAnariRenderer.h"
#include "vtkAnariSceneGraph.h"
#include "vtkAnariTestUtilities.h"
#include "vtkColorTransferFunction.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkOutlineFilter.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointDataToCellData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkVolumeProperty.h"
#include "vtkXMLImageDataReader.h"

/**
 * This tests whether ANARI properly handles cell data
 */
int TestAnariCellData(int argc, char* argv[])
{
  bool useDebugDevice = false;

  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "--trace"))
    {
      useDebugDevice = true;
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
  ren->ResetCamera();

  vtkAnariTestUtilities::SetParameterDefaults(renWin, useDebugDevice, "TestAnariCellData");

  vtkAnariRenderer* anariRenderer = vtkAnariRenderWindow::SafeDownCast(renWin)->GetAnariRenderer();
  anariRenderer->SetParameterf("ambientRadiance", 0.5f);

  renWin->Render();

  const auto& extensions = vtkAnariTestUtilities::GetDeviceExtensions(renWin);
  if (extensions.ANARI_KHR_SPATIAL_FIELD_STRUCTURED_REGULAR)
  {
    int retVal = vtkRegressionTestImage(renWin);

    if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
      iren->Start();
    }

    return !retVal;
  }

  vtkLogF(WARNING, "Required feature KHR_SPATIAL_FIELD_STRUCTURED_REGULAR not supported.");
  return VTK_SKIP_RETURN_CODE;
}
