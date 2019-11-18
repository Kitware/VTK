/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPolylineWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInteractorEventRecorder.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPolyLineRepresentation.h"
#include "vtkPolyLineWidget.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"

const char events[] = R"(# StreamVersion 1.1
KeyPressEvent 158 154 0 0 1 Control_L
CharEvent 158 154 0 0 1 Control_L
LeftButtonPressEvent 151 150 2 0 0 Control_L
RenderEvent 151 150 2 0 0 Control_L
LeftButtonReleaseEvent 151 150 2 0 0 Control_L
RenderEvent 151 150 2 0 0 Control_L
MouseMoveEvent 153 144 2 0 0 Control_L
KeyReleaseEvent 153 144 2 0 1 Control_L
MouseMoveEvent 151 150 0 0 0 Control_L
LeftButtonPressEvent 151 150 0 0 0 Control_L
RenderEvent 151 150 0 0 0 Control_L
MouseMoveEvent 151 151 0 0 0 Control_L
RenderEvent 151 151 0 0 0 Control_L
MouseMoveEvent 151 153 0 0 0 Control_L
RenderEvent 151 153 0 0 0 Control_L
MouseMoveEvent 151 153 0 0 0 Control_L
RenderEvent 151 153 0 0 0 Control_L
MouseMoveEvent 151 154 0 0 0 Control_L
RenderEvent 151 154 0 0 0 Control_L
MouseMoveEvent 151 156 0 0 0 Control_L
RenderEvent 151 156 0 0 0 Control_L
MouseMoveEvent 151 156 0 0 0 Control_L
RenderEvent 151 156 0 0 0 Control_L
MouseMoveEvent 151 158 0 0 0 Control_L
RenderEvent 151 158 0 0 0 Control_L
MouseMoveEvent 151 159 0 0 0 Control_L
RenderEvent 151 159 0 0 0 Control_L
MouseMoveEvent 151 160 0 0 0 Control_L
RenderEvent 151 160 0 0 0 Control_L
MouseMoveEvent 151 161 0 0 0 Control_L
RenderEvent 151 161 0 0 0 Control_L
MouseMoveEvent 151 163 0 0 0 Control_L
RenderEvent 151 163 0 0 0 Control_L
MouseMoveEvent 151 165 0 0 0 Control_L
RenderEvent 151 165 0 0 0 Control_L
MouseMoveEvent 151 166 0 0 0 Control_L
RenderEvent 151 166 0 0 0 Control_L
MouseMoveEvent 151 167 0 0 0 Control_L
RenderEvent 151 167 0 0 0 Control_L
MouseMoveEvent 152 168 0 0 0 Control_L
RenderEvent 152 168 0 0 0 Control_L
MouseMoveEvent 152 170 0 0 0 Control_L
RenderEvent 152 170 0 0 0 Control_L
MouseMoveEvent 152 172 0 0 0 Control_L
RenderEvent 152 172 0 0 0 Control_L
MouseMoveEvent 152 174 0 0 0 Control_L
RenderEvent 152 174 0 0 0 Control_L
MouseMoveEvent 152 176 0 0 0 Control_L
RenderEvent 152 176 0 0 0 Control_L
MouseMoveEvent 152 177 0 0 0 Control_L
RenderEvent 152 177 0 0 0 Control_L
MouseMoveEvent 152 179 0 0 0 Control_L
RenderEvent 152 179 0 0 0 Control_L
MouseMoveEvent 152 180 0 0 0 Control_L
RenderEvent 152 180 0 0 0 Control_L
MouseMoveEvent 152 182 0 0 0 Control_L
RenderEvent 152 182 0 0 0 Control_L
MouseMoveEvent 152 184 0 0 0 Control_L
RenderEvent 152 184 0 0 0 Control_L
MouseMoveEvent 152 185 0 0 0 Control_L
RenderEvent 152 185 0 0 0 Control_L
MouseMoveEvent 152 187 0 0 0 Control_L
RenderEvent 152 187 0 0 0 Control_L
MouseMoveEvent 152 188 0 0 0 Control_L
RenderEvent 152 188 0 0 0 Control_L
MouseMoveEvent 152 190 0 0 0 Control_L
RenderEvent 152 190 0 0 0 Control_L
MouseMoveEvent 152 193 0 0 0 Control_L
RenderEvent 152 193 0 0 0 Control_L
MouseMoveEvent 152 194 0 0 0 Control_L
RenderEvent 152 194 0 0 0 Control_L
MouseMoveEvent 153 196 0 0 0 Control_L
RenderEvent 153 196 0 0 0 Control_L
MouseMoveEvent 153 198 0 0 0 Control_L
RenderEvent 153 198 0 0 0 Control_L
MouseMoveEvent 153 199 0 0 0 Control_L
RenderEvent 153 199 0 0 0 Control_L
MouseMoveEvent 153 201 0 0 0 Control_L
RenderEvent 153 201 0 0 0 Control_L
MouseMoveEvent 153 202 0 0 0 Control_L
RenderEvent 153 202 0 0 0 Control_L
MouseMoveEvent 153 204 0 0 0 Control_L
RenderEvent 153 204 0 0 0 Control_L
MouseMoveEvent 153 205 0 0 0 Control_L
RenderEvent 153 205 0 0 0 Control_L
MouseMoveEvent 153 206 0 0 0 Control_L
RenderEvent 153 206 0 0 0 Control_L
MouseMoveEvent 153 207 0 0 0 Control_L
RenderEvent 153 207 0 0 0 Control_L
MouseMoveEvent 153 208 0 0 0 Control_L
RenderEvent 153 208 0 0 0 Control_L
MouseMoveEvent 152 208 0 0 0 Control_L
RenderEvent 152 208 0 0 0 Control_L
LeftButtonReleaseEvent 152 208 0 0 0 Control_L
RenderEvent 152 208 0 0 0 Control_L
MouseMoveEvent 100 150 0 0 0 Control_L
LeftButtonPressEvent 100 150 0 0 0 Control_L
RenderEvent 100 150 0 0 0 Control_L
LeftButtonReleaseEvent 100 150 0 0 0 Control_L
RenderEvent 100 150 0 0 0 Control_L
MouseMoveEvent 72 180 0 0 0 Control_L
KeyPressEvent 72 180 0 0 1 Alt_L
CharEvent 72 180 0 0 1 Alt_L
MouseMoveEvent 60 232 4 0 0 Alt_L
LeftButtonPressEvent 60 232 4 0 0 Alt_L
RenderEvent 60 232 4 0 0 Alt_L
LeftButtonReleaseEvent 60 232 4 0 0 Alt_L
RenderEvent 60 232 4 0 0 Alt_L
MouseMoveEvent 14 202 4 0 0 Alt_L
LeftButtonPressEvent 14 202 4 0 0 Alt_L
RenderEvent 14 202 4 0 0 Alt_L
LeftButtonReleaseEvent 14 202 4 0 0 Alt_L
RenderEvent 14 202 4 0 0 Alt_L
MouseMoveEvent 106 133 4 0 0 Alt_L
KeyReleaseEvent 106 133 4 0 1 Alt_L
MouseMoveEvent 101 145 0 0 0 Alt_L
KeyPressEvent 101 145 0 0 1 Shift_L
CharEvent 101 145 0 0 1 Shift_L
LeftButtonPressEvent 101 145 1 0 0 Shift_L
RenderEvent 101 145 1 0 0 Shift_L
LeftButtonReleaseEvent 101 145 1 0 0 Shift_L
RenderEvent 101 145 1 0 0 Shift_L
KeyReleaseEvent 101 145 1 0 1 Shift_L
MouseMoveEvent 204 147 0 0 0 Shift_L
LeftButtonPressEvent 204 147 0 0 0 Shift_L
RenderEvent 204 147 0 0 0 Shift_L
LeftButtonReleaseEvent 204 147 0 0 0 Shift_L
RenderEvent 204 147 0 0 0 Shift_L
MouseMoveEvent 202 113 0 0 0 Shift_L
KeyPressEvent 202 113 0 0 1 Alt_L
CharEvent 202 113 0 0 1 Alt_L
MouseMoveEvent 214 72 4 0 0 Alt_L
LeftButtonPressEvent 214 72 4 0 0 Alt_L
RenderEvent 214 72 4 0 0 Alt_L
LeftButtonReleaseEvent 214 72 4 0 0 Alt_L
RenderEvent 214 72 4 0 0 Alt_L
MouseMoveEvent 98 71 4 0 0 Alt_L
LeftButtonPressEvent 98 71 4 0 0 Alt_L
RenderEvent 98 71 4 0 0 Alt_L
LeftButtonReleaseEvent 98 71 4 0 0 Alt_L
RenderEvent 98 71 4 0 0 Alt_L
MouseMoveEvent 205 8 4 0 0 Alt_L
KeyReleaseEvent 205 8 4 0 1 Alt_L
)";

int TestPolylineWidget(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  vtkNew<vtkRenderer> renderer;
  renderer->ResetCamera(0.0, 1.0, 0.0, 1.0, 0.0, 0.0);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  renWin->SetSize(300, 300);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // Points
  vtkNew<vtkPoints> points;
  points->InsertNextPoint(0.25, 0.5, 0.0);
  points->InsertNextPoint(0.75, 0.5, 0.0);

  // Create the widget
  vtkNew<vtkPolyLineRepresentation> rep;
  rep->InitializeHandles(points);

  vtkNew<vtkPolyLineWidget> widget;
  widget->SetInteractor(iren);
  widget->SetRepresentation(rep);
  widget->On();

  // record events
  vtkNew<vtkInteractorEventRecorder> recorder;
  recorder->SetInteractor(iren);
  // recorder->SetFileName("/tmp/record.log");
  // recorder->On();
  // recorder->Record();
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(events);

  // Render the image
  iren->Initialize();
  renWin->Render();

  recorder->Play();
  recorder->Off();

  iren->Start();

  return EXIT_SUCCESS;
}
