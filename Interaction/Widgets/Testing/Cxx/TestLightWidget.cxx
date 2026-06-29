// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
//
// This example tests the vtkLightWidget

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkInteractorEventRecorder.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkLightRepresentation.h>
#include <vtkLightWidget.h>
#include <vtkNew.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>

constexpr char eventLog[] = "# StreamVersion 1.1\n"
                            "KeyReleaseEvent 675 -515 0 13 1 KP_Enter\n"
                            "EnterEvent 242 0 0 0 0 KP_Enter\n"
                            "RenderEvent 242 0 0 0 0 KP_Enter\n"
                            "MouseMoveEvent 157 151 0 0 0 KP_Enter\n"
                            "LeftButtonPressEvent 157 151 0 0 0 KP_Enter\n"
                            "MouseMoveEvent 156 151 0 0 0 KP_Enter\n"
                            "RenderEvent 156 151 0 0 0 KP_Enter\n"
                            "MouseMoveEvent 232 240 0 0 0 KP_Enter\n"
                            "RenderEvent 232 240 0 0 0 KP_Enter\n"
                            "LeftButtonReleaseEvent 232 240 0 0 0 KP_Enter\n"
                            "RenderEvent 232 240 0 0 0 KP_Enter\n"
                            "MouseMoveEvent 231 240 0 0 0 KP_Enter\n"
                            "MouseMoveEvent 171 149 0 0 0 KP_Enter\n"
                            "LeftButtonPressEvent 171 149 0 0 0 KP_Enter\n"
                            "MouseMoveEvent 172 149 0 0 0 KP_Enter\n"
                            "RenderEvent 172 149 0 0 0 KP_Enter\n"
                            "MouseMoveEvent 244 128 0 0 0 KP_Enter\n"
                            "RenderEvent 244 128 0 0 0 KP_Enter\n"
                            "LeftButtonReleaseEvent 244 128 0 0 0 KP_Enter\n"
                            "RenderEvent 244 128 0 0 0 KP_Enter\n"
                            "MouseMoveEvent 244 128 0 0 0 KP_Enter\n"
                            "MouseMoveEvent 145 148 0 0 0 KP_Enter\n"
                            "LeftButtonPressEvent 145 148 0 0 0 KP_Enter\n"
                            "MouseMoveEvent 144 148 0 0 0 KP_Enter\n"
                            "RenderEvent 144 148 0 0 0 KP_Enter\n"
                            "MouseMoveEvent 74 230 0 0 0 KP_Enter\n"
                            "RenderEvent 74 230 0 0 0 KP_Enter\n"
                            "LeftButtonReleaseEvent 74 230 0 0 0 KP_Enter\n"
                            "RenderEvent 74 230 0 0 0 KP_Enter\n"
                            "MouseMoveEvent 74 230 0 0 0 KP_Enter\n"
                            "MouseMoveEvent 140 162 0 0 0 KP_Enter\n"
                            "LeftButtonPressEvent 140 162 0 0 0 KP_Enter\n"
                            "MouseMoveEvent 140 161 0 0 0 KP_Enter\n"
                            "RenderEvent 140 161 0 0 0 KP_Enter\n"
                            "MouseMoveEvent 53 114 0 0 0 KP_Enter\n"
                            "RenderEvent 53 114 0 0 0 KP_Enter\n"
                            "LeftButtonReleaseEvent 48 115 0 0 0 KP_Enter\n"
                            "RenderEvent 48 115 0 0 0 KP_Enter\n"
                            "MouseMoveEvent 91 111 0 0 0 KP_Enter\n"
                            "MouseMoveEvent 191 137 0 0 0 KP_Enter\n"
                            "RightButtonPressEvent 191 137 0 0 0 KP_Enter\n"
                            "RenderEvent 191 137 0 0 0 KP_Enter\n"
                            "MouseMoveEvent 191 138 0 0 0 KP_Enter\n"
                            "MouseMoveEvent 167 109 0 0 0 KP_Enter\n"
                            "RenderEvent 167 109 0 0 0 KP_Enter\n"
                            "RightButtonReleaseEvent 167 109 0 0 0 KP_Enter\n"
                            "RenderEvent 168 109 0 0 0 KP_Enter\n";

int TestLightWidget(int, char*[])
{
  // Create a renderer, render window and interactor
  vtkNew<vtkRenderer> ren;
  ren->SetBackground(0.1, 0.2, 0.4);
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(300, 300);
  renWin->AddRenderer(ren);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkLightRepresentation> lightRep;
  lightRep->SetPositional(true);

  vtkNew<vtkLightWidget> lightWidget;
  lightWidget->SetInteractor(iren);
  lightWidget->SetRepresentation(lightRep);
  lightWidget->On();

  vtkNew<vtkLightRepresentation> lightRep2;
  double color[3] = { 1.0, 1.0, 0.0 };
  lightRep2->SetLightColor(color);

  vtkNew<vtkLightWidget> lightWidget2;
  lightWidget2->SetInteractor(iren);
  lightWidget2->SetRepresentation(lightRep2);
  lightWidget2->On();

  renWin->SetMultiSamples(0);
  renWin->Render();
  ren->ResetCamera();
  renWin->Render();

  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->Initialize();
  iren->SetInteractorStyle(style);

  vtkNew<vtkInteractorEventRecorder> recorder;
  recorder->SetInteractor(iren);
#if 0
  recorder->SetFileName("./record.log");
  recorder->Record();
  recorder->On();
#else
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(eventLog);
  recorder->Play();
#endif

  iren->Start();

  recorder->Stop();

  return EXIT_SUCCESS;
}
