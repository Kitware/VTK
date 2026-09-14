// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCameraPass.h"
#include "vtkElevationFilter.h"
#include "vtkNew.h"
#include "vtkOverlayPass.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderPassCollection.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkScalarBarActor.h"
#include "vtkScalarsToColors.h"
#include "vtkSequencePass.h"
#include "vtkSphereSource.h"
#include "vtkTextProperty.h"

#include "vtkAnariOpenGLTestUtilities.h"
#include "vtkAnariPass.h"
#include "vtkAnariSceneGraph.h"

/**
 * This test renders a sphere with a scalar bar on top and make sure the scalar bar shows up on the
 * screen.
 */
int TestAnariScalarBar(int argc, char* argv[])
{
  bool useDebugDevice = false;

  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "--trace"))
    {
      useDebugDevice = true;
    }
  }

  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(8);
  sphere->SetPhiResolution(8);
  vtkNew<vtkElevationFilter> elev;
  elev->SetInputConnection(sphere->GetOutputPort(0));

  vtkNew<vtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(elev->GetOutputPort(0));
  vtkNew<vtkActor> sphereActor;
  sphereActor->SetMapper(sphereMapper);

  vtkNew<vtkScalarBarActor> scalarBar1;
  vtkScalarsToColors* lut = sphereMapper->GetLookupTable();
  lut->SetAnnotation(0.0, "0.0");
  lut->SetAnnotation(0.25, "0.25");
  lut->SetAnnotation(0.50, "0.50");
  lut->SetAnnotation(0.75, "0.75");
  lut->SetAnnotation(1.00, "1.00");
  scalarBar1->SetLookupTable(lut);
  scalarBar1->DrawAnnotationsOn();
  scalarBar1->DrawTickLabelsOff();
  scalarBar1->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
  scalarBar1->GetPositionCoordinate()->SetValue(.6, .05);
  scalarBar1->SetWidth(0.15);
  scalarBar1->SetHeight(0.5);
  scalarBar1->SetTextPositionToPrecedeScalarBar();
  scalarBar1->GetTitleTextProperty()->SetColor(0., 0., 1.);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetWindowName("VTK - Scalar Bar options");
  renWin->SetSize(600, 500);
  renWin->SetMultiSamples(0);

  vtkNew<vtkRenderer> ren1;
  ren1->AddActor(sphereActor);
  ren1->AddActor(scalarBar1);
  ren1->GradientBackgroundOn();
  ren1->SetBackground(.5, .5, .5);
  ren1->SetBackground2(.0, .0, .0);

  vtkNew<vtkRenderPassCollection> renderPassCollection;
  vtkNew<vtkAnariPass> anariPass;
  vtkNew<vtkOverlayPass> overlayPass;
  renderPassCollection->AddItem(anariPass);
  renderPassCollection->AddItem(overlayPass);
  vtkNew<vtkSequencePass> sequencePass;
  sequencePass->SetPasses(renderPassCollection);
  vtkNew<vtkCameraPass> cameraPass;
  cameraPass->SetDelegatePass(sequencePass);

  ren1->SetPass(cameraPass);
  renWin->AddRenderer(ren1);
  vtkAnariOpenGLTestUtilities::SetParameterDefaults(
    anariPass, ren1, useDebugDevice, "TestAnariScalarBar");

  int retVal = vtkRegressionTestImage(renWin);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
