/*=========================================================================

Program:   Visualization Toolkit
Module:    TestOrientationMarkerWidget2.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSmartPointer.h"
#include "vtkActor.h"
#include "vtkAxesActor.h"
#include "vtkCamera.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkInteractorStyleImage.h"
#include "vtkNew.h"
#include "vtkOrientationMarkerWidget.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTextSource.h"

const char TestOMWidgetEventLog[] =
  "# StreamVersion 1\n"
  "EnterEvent 230 19 0 0 0 0 0\n"
  "MouseMoveEvent 230 19 0 0 0 0 0\n"
  "MouseMoveEvent 212 35 0 0 0 0 0\n"
  "MouseMoveEvent 196 46 0 0 0 0 0\n"
  "MouseMoveEvent 318 153 0 0 0 0 0\n"
  "MouseMoveEvent 319 166 0 0 0 0 0\n"
  "MouseMoveEvent 321 178 0 0 0 0 0\n"
  "MouseMoveEvent 321 192 0 0 0 0 0\n"
  "MouseMoveEvent 321 203 0 0 0 0 0\n"
  "MouseMoveEvent 321 213 0 0 0 0 0\n"
  "MouseMoveEvent 321 220 0 0 0 0 0\n"
  "MouseMoveEvent 321 228 0 0 0 0 0\n"
  "MouseMoveEvent 320 234 0 0 0 0 0\n"
  "MouseMoveEvent 318 243 0 0 0 0 0\n"
  "MouseMoveEvent 214 146 0 0 0 0 0\n"
  "MouseMoveEvent 215 147 0 0 0 0 0\n"
  "KeyPressEvent 215 147 0 0 98 1 b\n"
  "KeyReleaseEvent 300 185 0 0 98 1 b\n"
  "MouseMoveEvent 300 188 0 0 0 0 b\n"
  "MouseMoveEvent 301 191 0 0 0 0 b\n"
  "MouseMoveEvent 302 196 0 0 0 0 b\n"
  "MouseMoveEvent 303 202 0 0 0 0 b\n"
  "MouseMoveEvent 308 212 0 0 0 0 b\n"
  "MouseMoveEvent 370 299 0 0 0 0 b\n"
  "LeaveEvent 370 299 0 0 0 0 b\n"
  "EnterEvent 402 294 0 0 0 0 b\n"
  "MouseMoveEvent 402 294 0 0 0 0 b\n"
  "MouseMoveEvent 403 281 0 0 0 0 b\n"
  "MouseMoveEvent 408 263 0 0 0 0 b\n"
  "MouseMoveEvent 411 242 0 0 0 0 b\n"
  "MouseMoveEvent 416 226 0 0 0 0 b\n"
  "MouseMoveEvent 422 199 0 0 0 0 b\n"
  "MouseMoveEvent 428 161 0 0 0 0 b\n"
  "MouseMoveEvent 443 29 0 0 0 0 b\n"
  "LeaveEvent 443 29 0 0 0 0 b\n"
  "EnterEvent 428 110 0 0 0 0 b\n"
  "MouseMoveEvent 428 110 0 0 0 0 b\n"
  "MouseMoveEvent 413 160 0 0 0 0 b\n"
  "MouseMoveEvent 392 209 0 0 0 0 b\n"
  "MouseMoveEvent 390 241 0 0 0 0 b\n"
  "MouseMoveEvent 386 270 0 0 0 0 b\n"
  "MouseMoveEvent 385 287 0 0 0 0 b\n"
  "LeaveEvent 385 287 0 0 0 0 b\n"
  "ExitEvent 385 287 0 0 0 0 b\n"
;

int TestOrientationMarkerWidget2(int argc, char *argv[])
{
  // Create a text actor to move around
  vtkNew<vtkTextSource> textSource;
  textSource->SetText("Hello");
  textSource->SetForegroundColor(1.0, 0.0, 0.0);
  textSource->BackingOff();
  textSource->Update();

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(textSource->GetOutputPort());

  vtkNew<vtkActor> textActor;
  textActor->SetMapper(mapper.GetPointer());

  vtkNew<vtkRenderer> smallViewRenderer;
  smallViewRenderer->SetViewport(0.5, 0.5, 0.75, 0.75);

  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderer> backgroundRenderer;
  renWin->AddRenderer(backgroundRenderer.GetPointer());
  renWin->AddRenderer(smallViewRenderer.GetPointer());

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());
  vtkNew<vtkInteractorStyleImage> style;
  iren->SetInteractorStyle(style.GetPointer());

  // Create the widget
  vtkNew<vtkOrientationMarkerWidget> orientationWidget;
  orientationWidget->SetInteractor(iren.GetPointer());
  orientationWidget->SetDefaultRenderer(smallViewRenderer.GetPointer());
  orientationWidget->SetViewport(0, 0, 1, 1);
  orientationWidget->SetOrientationMarker(textActor.GetPointer());
  orientationWidget->On();

  vtkSmartPointer<vtkInteractorEventRecorder> recorder =
    vtkSmartPointer<vtkInteractorEventRecorder>::New();
  recorder->SetInteractor(iren.GetPointer());
  recorder->SetFileName("record.log");
  recorder->SetKeyPressActivationValue('b');

  recorder->ReadFromInputStringOn();
  recorder->SetInputString(TestOMWidgetEventLog);

  smallViewRenderer->SetBackground(0.5, 0.5, 0.5);
  renWin->SetSize(450, 300);
  renWin->Render();

  recorder->Play();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  iren->Start();

  int ret = vtkRegressionTestImage(renWin.GetPointer());
  return ret == vtkTesting::PASSED ? EXIT_SUCCESS : EXIT_FAILURE;
}
