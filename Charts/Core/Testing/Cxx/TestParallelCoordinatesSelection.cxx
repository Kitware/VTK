// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <vtkCallbackCommand.h>
#include <vtkChartParallelCoordinates.h>
#include <vtkContextActor.h>
#include <vtkContextKeyEvent.h>
#include <vtkContextMouseEvent.h>
#include <vtkContextView.h>
#include <vtkDoubleArray.h>
#include <vtkIdTypeArray.h>
#include <vtkInteractorEventRecorder.h>
#include <vtkNew.h>
#include <vtkPlotParallelCoordinates.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTable.h>

static const int expectedValues[14] = { 39, 40, 41, 42, 43, 44, 45, 54, 55, 56, 57, 58, 59, 60 };

static const char* eventLog = "# StreamVersion 1.2\n"
                              "ExposeEvent 0 399 0 0 0 0 0\n"
                              "RenderEvent 0 399 0 0 0 0 0\n"
                              "KeyReleaseEvent 3288 -268 0 0 1 Return 0\n"
                              "TimerEvent 3288 -268 0 0 1 Return 0\n"
                              "RenderEvent 3288 -268 0 0 1 Return 0\n"
                              "EnterEvent 363 29 0 0 0 Return 0\n"
                              "MouseMoveEvent 363 29 0 0 0 Return 0\n"
                              "MouseMoveEvent 351 43 0 0 0 Return 0\n"
                              "MouseMoveEvent 444 93 0 0 0 Return 0\n"
                              "MouseMoveEvent 502 119 0 0 0 Return 0\n"
                              "MouseMoveEvent 560 139 0 0 0 Return 0\n"
                              "LeaveEvent 602 147 0 0 0 Return 0\n"
                              "KeyPressEvent 1457 323 0 0 1 Shift_L 0\n"
                              "CharEvent 1457 323 0 0 1 Shift_L 0\n"
                              "KeyReleaseEvent -463 350 1 0 1 Shift_L 0\n"
                              "EnterEvent 419 92 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 419 97 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 420 102 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 390 375 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 388 387 0 0 0 Shift_L 0\n"
                              "LeaveEvent 382 401 0 0 0 Shift_L 0\n"
                              "EnterEvent 336 384 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 336 384 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 320 369 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 61 161 0 0 0 Shift_L 0\n"
                              "LeftButtonPressEvent 61 161 0 0 0 Shift_L 0\n"
                              "TimerEvent 61 161 0 0 0 Shift_L 0\n"
                              "RenderEvent 61 161 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 60 160 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 159 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 157 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 155 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 153 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 151 0 0 0 Shift_L 0\n"
                              "TimerEvent 59 151 0 0 0 Shift_L 0\n"
                              "RenderEvent 59 151 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 149 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 147 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 146 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 145 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 143 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 141 0 0 0 Shift_L 0\n"
                              "TimerEvent 59 141 0 0 0 Shift_L 0\n"
                              "RenderEvent 59 141 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 136 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 127 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 124 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 120 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 116 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 113 0 0 0 Shift_L 0\n"
                              "TimerEvent 59 113 0 0 0 Shift_L 0\n"
                              "RenderEvent 59 113 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 106 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 104 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 100 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 99 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 96 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 95 0 0 0 Shift_L 0\n"
                              "TimerEvent 59 95 0 0 0 Shift_L 0\n"
                              "RenderEvent 59 95 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 92 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 91 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 89 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 88 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 87 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 86 0 0 0 Shift_L 0\n"
                              "TimerEvent 59 86 0 0 0 Shift_L 0\n"
                              "RenderEvent 59 86 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 85 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 83 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 82 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 79 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 75 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 73 0 0 0 Shift_L 0\n"
                              "TimerEvent 59 73 0 0 0 Shift_L 0\n"
                              "RenderEvent 59 73 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 70 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 68 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 67 0 0 0 Shift_L 0\n"
                              "TimerEvent 59 67 0 0 0 Shift_L 0\n"
                              "RenderEvent 59 67 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 66 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 66 0 0 0 Shift_L 0\n"
                              "TimerEvent 59 66 0 0 0 Shift_L 0\n"
                              "RenderEvent 59 66 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 65 0 0 0 Shift_L 0\n"
                              "TimerEvent 59 65 0 0 0 Shift_L 0\n"
                              "RenderEvent 59 65 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 64 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 62 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 61 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 59 0 0 0 Shift_L 0\n"
                              "TimerEvent 59 59 0 0 0 Shift_L 0\n"
                              "RenderEvent 59 59 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 58 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 58 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 57 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 55 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 53 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 52 0 0 0 Shift_L 0\n"
                              "TimerEvent 59 52 0 0 0 Shift_L 0\n"
                              "RenderEvent 59 52 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 51 0 0 0 Shift_L 0\n"
                              "TimerEvent 59 51 0 0 0 Shift_L 0\n"
                              "RenderEvent 59 51 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 50 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 59 49 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 60 48 0 0 0 Shift_L 0\n"
                              "TimerEvent 60 48 0 0 0 Shift_L 0\n"
                              "RenderEvent 60 48 0 0 0 Shift_L 0\n"
                              "LeftButtonReleaseEvent 60 48 0 0 0 Shift_L 0\n"
                              "TimerEvent 60 48 0 0 0 Shift_L 0\n"
                              "RenderEvent 60 48 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 61 49 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 61 50 0 0 0 Shift_L 0\n"
                              "MouseMoveEvent 223 224 0 0 0 Shift_L 0\n"
                              "KeyPressEvent 223 224 0 0 1 Alt_L 0\n"
                              "CharEvent 223 224 0 0 1 Alt_L 0\n"
                              "MouseMoveEvent 223 224 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 222 223 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 222 223 4 0 0 Alt_L 0\n"
                              "LeftButtonPressEvent 222 223 4 0 0 Alt_L 0\n"
                              "TimerEvent 222 223 4 0 0 Alt_L 0\n"
                              "RenderEvent 222 223 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 222 221 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 222 219 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 222 217 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 222 215 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 222 214 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 222 213 4 0 0 Alt_L 0\n"
                              "TimerEvent 222 213 4 0 0 Alt_L 0\n"
                              "RenderEvent 222 213 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 222 211 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 222 210 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 222 209 4 0 0 Alt_L 0\n"
                              "TimerEvent 222 209 4 0 0 Alt_L 0\n"
                              "RenderEvent 222 209 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 222 206 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 222 205 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 222 203 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 222 201 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 222 199 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 222 197 4 0 0 Alt_L 0\n"
                              "TimerEvent 222 197 4 0 0 Alt_L 0\n"
                              "RenderEvent 222 197 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 222 196 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 222 195 4 0 0 Alt_L 0\n"
                              "TimerEvent 222 195 4 0 0 Alt_L 0\n"
                              "RenderEvent 222 195 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 222 195 4 0 0 Alt_L 0\n"
                              "TimerEvent 222 195 4 0 0 Alt_L 0\n"
                              "RenderEvent 222 195 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 222 194 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 222 192 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 222 191 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 222 190 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 222 189 4 0 0 Alt_L 0\n"
                              "TimerEvent 222 189 4 0 0 Alt_L 0\n"
                              "RenderEvent 222 189 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 222 188 4 0 0 Alt_L 0\n"
                              "TimerEvent 222 188 4 0 0 Alt_L 0\n"
                              "RenderEvent 222 188 4 0 0 Alt_L 0\n"
                              "LeftButtonReleaseEvent 222 188 4 0 0 Alt_L 0\n"
                              "TimerEvent 222 188 4 0 0 Alt_L 0\n"
                              "RenderEvent 222 188 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 222 187 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 222 186 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 218 145 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 218 146 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 218 146 4 0 0 Alt_L 0\n"
                              "LeftButtonPressEvent 218 146 4 0 0 Alt_L 0\n"
                              "TimerEvent 218 146 4 0 0 Alt_L 0\n"
                              "RenderEvent 218 146 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 218 145 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 218 145 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 218 143 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 218 141 4 0 0 Alt_L 0\n"
                              "TimerEvent 218 141 4 0 0 Alt_L 0\n"
                              "RenderEvent 218 141 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 218 140 4 0 0 Alt_L 0\n"
                              "TimerEvent 218 140 4 0 0 Alt_L 0\n"
                              "RenderEvent 218 140 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 218 139 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 218 138 4 0 0 Alt_L 0\n"
                              "TimerEvent 218 138 4 0 0 Alt_L 0\n"
                              "RenderEvent 218 138 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 218 136 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 218 135 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 218 133 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 218 131 4 0 0 Alt_L 0\n"
                              "TimerEvent 218 131 4 0 0 Alt_L 0\n"
                              "RenderEvent 218 131 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 218 128 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 218 126 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 218 124 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 218 122 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 218 120 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 218 117 4 0 0 Alt_L 0\n"
                              "TimerEvent 218 117 4 0 0 Alt_L 0\n"
                              "RenderEvent 218 117 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 218 116 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 218 114 4 0 0 Alt_L 0\n"
                              "TimerEvent 218 114 4 0 0 Alt_L 0\n"
                              "RenderEvent 218 114 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 218 113 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 218 111 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 218 110 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 218 109 4 0 0 Alt_L 0\n"
                              "TimerEvent 218 109 4 0 0 Alt_L 0\n"
                              "RenderEvent 218 109 4 0 0 Alt_L 0\n"
                              "LeftButtonReleaseEvent 218 109 4 0 0 Alt_L 0\n"
                              "TimerEvent 218 109 4 0 0 Alt_L 0\n"
                              "RenderEvent 218 109 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 218 110 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 218 112 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 218 113 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 216 235 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 216 236 4 0 0 Alt_L 0\n"
                              "MouseMoveEvent 216 237 4 0 0 Alt_L 0\n"
                              "KeyPressEvent 216 237 4 0 1 Shift_L 0\n"
                              "CharEvent 216 237 4 0 1 Shift_L 0\n"
                              "LeftButtonPressEvent 216 237 5 0 0 Shift_L 0\n"
                              "MouseMoveEvent 216 236 5 0 0 Shift_L 0\n"
                              "TimerEvent 216 236 5 0 0 Shift_L 0\n"
                              "RenderEvent 216 236 5 0 0 Shift_L 0\n"
                              "MouseMoveEvent 216 236 5 0 0 Shift_L 0\n"
                              "MouseMoveEvent 216 235 5 0 0 Shift_L 0\n"
                              "MouseMoveEvent 217 233 5 0 0 Shift_L 0\n"
                              "MouseMoveEvent 217 231 5 0 0 Shift_L 0\n"
                              "MouseMoveEvent 217 227 5 0 0 Shift_L 0\n"
                              "TimerEvent 217 227 5 0 0 Shift_L 0\n"
                              "RenderEvent 217 227 5 0 0 Shift_L 0\n"
                              "MouseMoveEvent 217 225 5 0 0 Shift_L 0\n"
                              "MouseMoveEvent 218 222 5 0 0 Shift_L 0\n"
                              "MouseMoveEvent 218 221 5 0 0 Shift_L 0\n"
                              "MouseMoveEvent 218 219 5 0 0 Shift_L 0\n"
                              "MouseMoveEvent 218 216 5 0 0 Shift_L 0\n"
                              "MouseMoveEvent 219 214 5 0 0 Shift_L 0\n"
                              "TimerEvent 219 214 5 0 0 Shift_L 0\n"
                              "RenderEvent 219 214 5 0 0 Shift_L 0\n"
                              "MouseMoveEvent 219 211 5 0 0 Shift_L 0\n"
                              "MouseMoveEvent 219 210 5 0 0 Shift_L 0\n"
                              "MouseMoveEvent 219 209 5 0 0 Shift_L 0\n"
                              "TimerEvent 219 209 5 0 0 Shift_L 0\n"
                              "RenderEvent 219 209 5 0 0 Shift_L 0\n"
                              "MouseMoveEvent 219 208 5 0 0 Shift_L 0\n"
                              "MouseMoveEvent 219 207 5 0 0 Shift_L 0\n"
                              "MouseMoveEvent 219 204 5 0 0 Shift_L 0\n"
                              "MouseMoveEvent 219 202 5 0 0 Shift_L 0\n"
                              "MouseMoveEvent 219 201 5 0 0 Shift_L 0\n"
                              "MouseMoveEvent 219 196 5 0 0 Shift_L 0\n"
                              "TimerEvent 219 196 5 0 0 Shift_L 0\n"
                              "RenderEvent 219 196 5 0 0 Shift_L 0\n"
                              "MouseMoveEvent 219 195 5 0 0 Shift_L 0\n"
                              "MouseMoveEvent 219 194 5 0 0 Shift_L 0\n"
                              "TimerEvent 219 194 5 0 0 Shift_L 0\n"
                              "RenderEvent 219 194 5 0 0 Shift_L 0\n"
                              "MouseMoveEvent 219 192 5 0 0 Shift_L 0\n"
                              "MouseMoveEvent 219 192 5 0 0 Shift_L 0\n"
                              "TimerEvent 219 192 5 0 0 Shift_L 0\n"
                              "RenderEvent 219 192 5 0 0 Shift_L 0\n"
                              "LeftButtonReleaseEvent 219 192 5 0 0 Shift_L 0\n"
                              "TimerEvent 219 192 5 0 0 Shift_L 0\n"
                              "RenderEvent 219 192 5 0 0 Shift_L 0\n"
                              "MouseMoveEvent 222 192 5 0 0 Shift_L 0\n"
                              "MouseMoveEvent 226 192 5 0 0 Shift_L 0\n"
                              "MouseMoveEvent 369 151 5 0 0 Shift_L 0\n"
                              "MouseMoveEvent 371 151 5 0 0 Shift_L 0\n"
                              "MouseMoveEvent 372 151 5 0 0 Shift_L 0\n"
                              "KeyReleaseEvent 372 151 5 0 1 Shift_L 0\n"
                              "KeyReleaseEvent 372 151 4 0 1 Alt_L 0\n"
                              "MouseMoveEvent 372 152 0 0 0 Alt_L 0\n"
                              "MouseMoveEvent 372 152 0 0 0 Alt_L 0\n"
                              "MouseMoveEvent 566 380 0 0 0 Alt_L 0\n"
                              "MouseMoveEvent 570 387 0 0 0 Alt_L 0\n"
                              "MouseMoveEvent 572 391 0 0 0 Alt_L 0\n"
                              "MouseMoveEvent 573 396 0 0 0 Alt_L 0\n"
                              "MouseMoveEvent 573 399 0 0 0 Alt_L 0\n"
                              "LeaveEvent 574 403 0 0 0 Alt_L 0\n"
                              "ExitEvent 574 403 0 0 0 Alt_L 0\n"
                              "EnterEvent 588 399 0 0 0 Alt_L 0\n"
                              "MouseMoveEvent 588 399 0 0 0 Alt_L 0\n"
                              "MouseMoveEvent 598 382 0 0 0 Alt_L 0\n"
                              "LeaveEvent 606 359 0 0 0 Alt_L 0\n";

namespace
{
void UpdateSelectionMode(vtkObject* caller, unsigned long vtkNotUsed(eventId), void* clientData,
  void* vtkNotUsed(callData))
{
  const auto iren = static_cast<vtkRenderWindowInteractor*>(caller);
  auto chart = static_cast<vtkChartParallelCoordinates*>(clientData);

  if (iren->GetControlKey() != 0)
  {
    chart->SetSelectionMode(vtkContextScene::SELECTION_ADDITION);
  }

  if (iren->GetShiftKey() != 0)
  {
    chart->SetSelectionMode(vtkContextScene::SELECTION_SUBTRACTION);
  }

  if (iren->GetAltKey() != 0)
  {
    chart->SetSelectionMode(vtkContextScene::SELECTION_TOGGLE);
  }
}
}

int TestParallelCoordinatesSelection(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // Set up a 2D scene, add an parallel coordinate chart to it
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetWindowName("TestParallelCoordinateSelection");
  view->GetRenderWindow()->SetSize(600, 400);
  view->GetRenderWindow()->SetMultiSamples(0);
  vtkNew<vtkChartParallelCoordinates> chart;
  view->GetScene()->AddItem(chart);

  // Create a table with some points in it...
  vtkNew<vtkTable> table;
  vtkNew<vtkDoubleArray> arrX;
  arrX->SetName("x");
  table->AddColumn(arrX);
  vtkNew<vtkDoubleArray> arrC;
  arrC->SetName("cosine");
  table->AddColumn(arrC);
  vtkNew<vtkDoubleArray> arrS;
  arrS->SetName("sine");
  table->AddColumn(arrS);
  vtkNew<vtkDoubleArray> arrS2;
  arrS2->SetName("tangent");
  table->AddColumn(arrS2);

  int numPoints = 200;
  float inc = 7.5 / (numPoints - 1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
  {
    table->SetValue(i, 0, i * inc);
    table->SetValue(i, 1, cos(i * inc));
    table->SetValue(i, 2, sin(i * inc));
    table->SetValue(i, 3, tan(i * inc) + 0.5);
  }
  chart->GetPlot(0)->SetInputData(table);

  // Link some key events to switch between each selection mode.
  vtkNew<vtkCallbackCommand> keypressCallback;
  keypressCallback->SetCallback(::UpdateSelectionMode);
  keypressCallback->SetClientData(chart);
  view->GetInteractor()->AddObserver(vtkCommand::KeyPressEvent, keypressCallback);

  // record events
  vtkSmartPointer<vtkInteractorEventRecorder> recorder =
    vtkSmartPointer<vtkInteractorEventRecorder>::New();
  recorder->SetInteractor(view->GetInteractor());
  view->GetInteractor()->Initialize();
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(eventLog);
  recorder->Play();

  if (chart->GetNumberOfPlots() != 1)
  {
    std::cerr << "Wrong number of plot. Expected 1 but got " << chart->GetNumberOfPlots()
              << std::endl;
    return EXIT_FAILURE;
  }

  auto ids = chart->GetPlot(0)->GetSelection();
  if (!ids)
  {
    std::cerr << "Selection shouldn't be null" << std::endl;
    return EXIT_FAILURE;
  }

  if (ids->GetSize() != 14)
  {
    std::cerr << "Wrong number of id selection. Expected to have 14 ids but got " << ids->GetSize()
              << std::endl;
    return EXIT_FAILURE;
  }

  for (int i = 0; i < ids->GetSize(); i++)
  {
    if (ids->GetValue(i) != expectedValues[i])
    {
      std::cerr << "Wrong id values in the current selection. Expected to have "
                << expectedValues[i] << " id but got " << ids->GetValue(i) << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
