// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This test covers the smart volume mapper and composite method.
// This test volume renders a synthetic dataset with unsigned char values,
// with the composite method.

#include <vtkAnariPass.h>
#include <vtkAnariRendererNode.h>
#include <vtkCamera.h>
#include <vtkClipPolyData.h>
#include <vtkColorTransferFunction.h>
#include <vtkDataArray.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkImageData.h>
#include <vtkImageReader.h>
#include <vtkImageShiftScale.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkNew.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPlane.h>
#include <vtkPointData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkStructuredPointsReader.h>
#include <vtkTestUtilities.h>
#include <vtkTimerLog.h>
#include <vtkVolumeProperty.h>
#include <vtkXMLImageDataReader.h>

int TestAnariSmartVolumeMapper(int argc, char* argv[])
{
  double scalarRange[2];
  vtkNew<vtkActor> dssActor;
  vtkNew<vtkPolyDataMapper> dssMapper;
  vtkNew<vtkSmartVolumeMapper> volumeMapper;
  volumeMapper->SetRequestedRenderModeToAnari();

  vtkNew<vtkXMLImageDataReader> reader;
  const char* volumeFile = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vase_1comp.vti");
  reader->SetFileName(volumeFile);
  reader->Update();
  delete[] volumeFile;

  volumeMapper->SetInputConnection(reader->GetOutputPort());
#ifdef VTK_OPENGL2
  volumeMapper->SetSampleDistance(0.01);
#endif

  // Put inside an open box to evaluate composite order
  vtkNew<vtkDataSetSurfaceFilter> dssFilter;
  dssFilter->SetInputConnection(reader->GetOutputPort());
  vtkNew<vtkClipPolyData> clip;
  vtkNew<vtkPlane> plane;
  plane->SetOrigin(0, 50, 0);
  plane->SetNormal(0, -1, 0);
  clip->SetInputConnection(dssFilter->GetOutputPort());
  clip->SetClipFunction(plane);
  dssMapper->SetInputConnection(clip->GetOutputPort());
  dssMapper->ScalarVisibilityOff();
  dssActor->SetMapper(dssMapper);
  vtkProperty* property = dssActor->GetProperty();
  property->SetDiffuseColor(0.5, 0.5, 0.5);

  volumeMapper->GetInput()->GetScalarRange(scalarRange);
  volumeMapper->SetBlendModeToComposite();
#ifdef VTK_OPENGL2
  volumeMapper->SetAutoAdjustSampleDistances(1);
#endif
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren);
  ren->SetBackground(0.2, 0.2, 0.5);
  renWin->SetSize(400, 400);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(50, 0.0);
  scalarOpacity->AddPoint(75, 0.1);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);

  volumeProperty->SetScalarOpacity(scalarOpacity);

  vtkColorTransferFunction* colorTransferFunction = volumeProperty->GetRGBTransferFunction(0);
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(scalarRange[0], 0.0, 0.8, 0.1);
  colorTransferFunction->AddRGBPoint(30, 0.3, 0.8, 0.4);
  colorTransferFunction->AddRGBPoint(scalarRange[1], 0.0, 0.8, 0.1);

  vtkNew<vtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  vtkNew<vtkAnariPass> anariPass;
  ren->SetPass(anariPass);
  vtkAnariRendererNode::SetLibraryName("environment", ren);
  vtkAnariRendererNode::SetSamplesPerPixel(6, ren);
  vtkAnariRendererNode::SetLightFalloff(.5, ren);
  vtkAnariRendererNode::SetUseDenoiser(1, ren);
  vtkAnariRendererNode::SetCompositeOnGL(1, ren);

  ren->AddViewProp(volume);
  ren->AddActor(dssActor);
  ren->ResetCamera();

  ren->GetActiveCamera()->Pitch(-40);
  ren->ResetCamera();

  iren->Initialize();
  iren->SetDesiredUpdateRate(30.0);

  int retVal = vtkRegressionTestImage(renWin);

  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
