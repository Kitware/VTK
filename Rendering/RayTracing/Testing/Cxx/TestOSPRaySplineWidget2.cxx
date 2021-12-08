/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSPRaySplineWidget2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Description
// This test covers the use of spline widget with the OSPRay rendering backend

#include <vtkCamera.h>
#include <vtkOSPRayPass.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSplineRepresentation.h>
#include <vtkSplineWidget2.h>
#include <vtkTestUtilities.h>
#include <vtkTesting.h>

#include "vtkOSPRayTestInteractor.h"

static char OSPRayTSWeventLog[] = "# StreamVersion 1.1\n"
                                  "ExposeEvent 0 299 0 0 0 0\n"
                                  "EnterEvent 96 296 0 0 0 0\n"
                                  "MouseMoveEvent 96 296 0 0 0 0\n"
                                  "MouseMoveEvent 93 293 0 0 0 0\n"
                                  "LeaveEvent 87 301 0 0 0 0\n"
                                  "EnterEvent 52 292 0 0 0 0\n"
                                  "MouseMoveEvent 52 292 0 0 0 0\n"
                                  "MouseMoveEvent 302 227 0 0 0 0\n"
                                  "LeftButtonPressEvent 302 227 0 0 0 0\n"
                                  "MouseMoveEvent 301 226 0 0 0 0\n"
                                  "MouseMoveEvent 143 147 0 0 0 0\n"
                                  "LeftButtonReleaseEvent 143 147 0 0 0 0\n"
                                  "LeftButtonPressEvent 306 229 0 0 0 0\n"
                                  "MouseMoveEvent 306 227 0 0 0 0\n"
                                  "MouseMoveEvent 415 87 0 0 0 0\n"
                                  "LeftButtonReleaseEvent 415 87 0 0 0 0\n"
                                  "LeftButtonPressEvent 304 230 0 0 0 0\n"
                                  "MouseMoveEvent 304 228 0 0 0 0\n"
                                  "MouseMoveEvent 197 195 0 0 0 0\n"
                                  "LeftButtonReleaseEvent 197 195 0 0 0 0\n"
                                  "MouseMoveEvent 197 195 0 0 0 0\n"
                                  "MouseMoveEvent 338 229 0 0 0 0\n"
                                  "LeftButtonPressEvent 338 229 0 0 0 0\n"
                                  "MouseMoveEvent 338 229 0 0 0 0\n"
                                  "MouseMoveEvent 342 218 0 0 0 0\n"
                                  "MouseMoveEvent 343 210 0 0 0 0\n"
                                  "MouseMoveEvent 343 209 0 0 0 0\n"
                                  "MouseMoveEvent 344 202 0 0 0 0\n"
                                  "MouseMoveEvent 345 196 0 0 0 0\n"
                                  "MouseMoveEvent 346 190 0 0 0 0\n"
                                  "MouseMoveEvent 346 186 0 0 0 0\n"
                                  "MouseMoveEvent 347 179 0 0 0 0\n"
                                  "MouseMoveEvent 348 171 0 0 0 0\n"
                                  "MouseMoveEvent 348 164 0 0 0 0\n"
                                  "MouseMoveEvent 350 151 0 0 0 0\n"
                                  "MouseMoveEvent 352 142 0 0 0 0\n"
                                  "MouseMoveEvent 352 138 0 0 0 0\n"
                                  "MouseMoveEvent 353 128 0 0 0 0\n"
                                  "MouseMoveEvent 355 119 0 0 0 0\n"
                                  "MouseMoveEvent 356 109 0 0 0 0\n"
                                  "MouseMoveEvent 357 100 0 0 0 0\n"
                                  "MouseMoveEvent 358 94 0 0 0 0\n"
                                  "MouseMoveEvent 358 91 0 0 0 0\n"
                                  "MouseMoveEvent 360 87 0 0 0 0\n"
                                  "MouseMoveEvent 360 84 0 0 0 0\n"
                                  "MouseMoveEvent 360 83 0 0 0 0\n"
                                  "MouseMoveEvent 361 79 0 0 0 0\n"
                                  "MouseMoveEvent 361 78 0 0 0 0\n"
                                  "MouseMoveEvent 361 73 0 0 0 0\n"
                                  "MouseMoveEvent 362 69 0 0 0 0\n"
                                  "MouseMoveEvent 362 64 0 0 0 0\n"
                                  "MouseMoveEvent 363 61 0 0 0 0\n"
                                  "MouseMoveEvent 363 60 0 0 0 0\n"
                                  "MouseMoveEvent 363 59 0 0 0 0\n"
                                  "MouseMoveEvent 363 55 0 0 0 0\n"
                                  "MouseMoveEvent 363 53 0 0 0 0\n"
                                  "MouseMoveEvent 364 52 0 0 0 0\n"
                                  "LeftButtonReleaseEvent 364 52 0 0 0 0\n"
                                  "LeftButtonPressEvent 308 195 0 0 0 0\n"
                                  "MouseMoveEvent 308 194 0 0 0 0\n"
                                  "MouseMoveEvent 272 74 0 0 0 0\n"
                                  "LeftButtonReleaseEvent 272 74 0 0 0 0\n"
                                  "LeftButtonPressEvent 154 198 0 0 0 0\n"
                                  "MouseMoveEvent 155 194 0 0 0 0\n"
                                  "MouseMoveEvent 392 156 0 0 0 0\n"
                                  "MouseMoveEvent 400 156 0 0 0 0\n"
                                  "MouseMoveEvent 406 155 0 0 0 0\n"
                                  "MouseMoveEvent 411 154 0 0 0 0\n"
                                  "MouseMoveEvent 414 153 0 0 0 0\n"
                                  "MouseMoveEvent 416 153 0 0 0 0\n"
                                  "MouseMoveEvent 417 153 0 0 0 0\n"
                                  "MouseMoveEvent 417 153 0 0 0 0\n"
                                  "LeftButtonReleaseEvent 417 153 0 0 0 0\n"
                                  "";

int TestOSPRaySplineWidget2(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkSplineWidget2> splineWidget;
  vtkNew<vtkSplineRepresentation> spline;
  splineWidget->SetRepresentation(spline);
  splineWidget->SetInteractor(iren);
  splineWidget->SetPriority(1.0);
  splineWidget->KeyPressActivationOff();

  renderer->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(600, 300);

  splineWidget->On();
  spline->SetNumberOfHandles(4);
  spline->SetNumberOfHandles(5);
  spline->SetResolution(399);

  // Set up an interesting viewpoint
  vtkCamera* camera = renderer->GetActiveCamera();
  camera->Elevation(110);
  camera->SetViewUp(0, 0, -1);
  camera->Azimuth(45);
  camera->SetFocalPoint(100.8, 100.8, 69);
  camera->SetPosition(560.949, 560.949, -167.853);
  renderer->ResetCameraClippingRange();

  // Test On Off mechanism
  splineWidget->EnabledOff();
  splineWidget->EnabledOn();

  // Test Set Get handle positions
  double pos[3];
  int i;
  for (i = 0; i < spline->GetNumberOfHandles(); i++)
  {
    spline->GetHandlePosition(i, pos);
    spline->SetHandlePosition(i, pos);
  }

  // Test Closed On Off
  spline->ClosedOn();
  spline->ClosedOff();

  vtkNew<vtkOSPRayPass> ospray;
  renderer->SetPass(ospray);

  vtkNew<vtkOSPRayTestInteractor> style;
  style->SetPipelineControlPoints(renderer, ospray, nullptr);
  iren->SetInteractorStyle(style);
  style->SetCurrentRenderer(renderer);

  // Render the image
  iren->Initialize();
  renWin->Render();

  return vtkTesting::InteractorEventLoop(argc, argv, iren, OSPRayTSWeventLog);

  return EXIT_SUCCESS;
}
