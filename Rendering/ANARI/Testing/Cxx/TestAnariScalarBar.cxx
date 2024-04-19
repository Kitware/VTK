// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkElevationFilter.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty2D.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkScalarBarActor.h"
#include "vtkScalarsToColors.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkTextProperty.h"

#include "vtkAnariPass.h"
#include "vtkAnariRendererNode.h"

int TestAnariScalarBar(int argc, char* argv[])
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

  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(8);
  sphere->SetPhiResolution(8);
  vtkNew<vtkElevationFilter> elev;
  elev->SetInputConnection(sphere->GetOutputPort(0));

  vtkNew<vtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(elev->GetOutputPort(0));
  vtkNew<vtkActor> sphereActor;
  sphereActor->SetMapper(sphereMapper);

  // Create the RenderWindow, Renderer and all Actors
  vtkNew<vtkRenderer> ren1;
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(ren1);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkScalarBarActor> scalarBar1;
  vtkScalarsToColors* lut = sphereMapper->GetLookupTable();
  lut->SetAnnotation(0.0, "0.0");
  lut->SetAnnotation(0.25, "0.25");
  lut->SetAnnotation(0.50, "0.50");
  lut->SetAnnotation(0.75, "0.75");
  lut->SetAnnotation(1.00, "1.00");
  // scalarBar1->SetTitle("Density");
  scalarBar1->SetLookupTable(lut);
  scalarBar1->DrawAnnotationsOn();
  scalarBar1->DrawTickLabelsOff();
  scalarBar1->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
  scalarBar1->GetPositionCoordinate()->SetValue(.6, .05);
  scalarBar1->SetWidth(0.15);
  scalarBar1->SetHeight(0.5);
  scalarBar1->SetTextPositionToPrecedeScalarBar();
  // scalarBar1->SetTextPositionToSucceedScalarBar();
  scalarBar1->GetTitleTextProperty()->SetColor(0., 0., 1.);
  // scalarBar1->GetLabelTextProperty()->SetColor( 0., 0., 1. );
  // scalarBar1->SetDrawFrame( 1 );
  // scalarBar1->GetFrameProperty()->SetColor( 0., 0., 0. );
  // scalarBar1->SetDrawBackground( 1 );
  // scalarBar1->GetBackgroundProperty()->SetColor( 1., 1., 1. );

  // Add the actors to the renderer, set the background and size
  ren1->AddActor(sphereActor);
  ren1->AddActor(scalarBar1);
  ren1->GradientBackgroundOn();
  ren1->SetBackground(.5, .5, .5);
  ren1->SetBackground2(.0, .0, .0);

  // render the image
  renWin->SetWindowName("VTK - Scalar Bar options");
  renWin->SetSize(600, 500);
  renWin->SetMultiSamples(0);

  vtkNew<vtkAnariPass> anariPass;
  ren1->SetPass(anariPass);

  if (useDebugDevice)
  {
    vtkAnariRendererNode::SetUseDebugDevice(1, ren1);
    vtkNew<vtkTesting> testing;

    std::string traceDir = testing->GetTempDirectory();
    traceDir += "/anari-trace";
    traceDir += "/TestAnariScalarBar";
    vtkAnariRendererNode::SetDebugDeviceDirectory(traceDir.c_str(), ren1);
  }

  vtkAnariRendererNode::SetLibraryName("environment", ren1);
  vtkAnariRendererNode::SetSamplesPerPixel(4, ren1);
  vtkAnariRendererNode::SetLightFalloff(.5, ren1);
  vtkAnariRendererNode::SetUseDenoiser(1, ren1);
  vtkAnariRendererNode::SetCompositeOnGL(1, ren1);

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);

  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
