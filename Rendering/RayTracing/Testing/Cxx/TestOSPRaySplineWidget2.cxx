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
                                  "MouseMoveEvent 95 293 0 0 0 0\n"
                                  "MouseMoveEvent 93 293 0 0 0 0\n"
                                  "LeaveEvent 87 301 0 0 0 0\n"
                                  "EnterEvent 52 292 0 0 0 0\n"
                                  "MouseMoveEvent 52 292 0 0 0 0\n"
                                  "MouseMoveEvent 302 227 0 0 0 0\n"
                                  "MouseMoveEvent 302 227 0 0 0 0\n"
                                  "LeftButtonPressEvent 302 227 0 0 0 0\n"
                                  "MouseMoveEvent 301 226 0 0 0 0\n"
                                  "MouseMoveEvent 294 221 0 0 0 0\n"
                                  "MouseMoveEvent 288 216 0 0 0 0\n"
                                  "MouseMoveEvent 284 214 0 0 0 0\n"
                                  "MouseMoveEvent 279 210 0 0 0 0\n"
                                  "MouseMoveEvent 275 208 0 0 0 0\n"
                                  "MouseMoveEvent 270 205 0 0 0 0\n"
                                  "MouseMoveEvent 265 202 0 0 0 0\n"
                                  "MouseMoveEvent 256 197 0 0 0 0\n"
                                  "MouseMoveEvent 249 194 0 0 0 0\n"
                                  "MouseMoveEvent 240 188 0 0 0 0\n"
                                  "MouseMoveEvent 230 184 0 0 0 0\n"
                                  "MouseMoveEvent 213 173 0 0 0 0\n"
                                  "MouseMoveEvent 207 171 0 0 0 0\n"
                                  "MouseMoveEvent 201 168 0 0 0 0\n"
                                  "MouseMoveEvent 196 166 0 0 0 0\n"
                                  "MouseMoveEvent 189 165 0 0 0 0\n"
                                  "MouseMoveEvent 179 160 0 0 0 0\n"
                                  "MouseMoveEvent 169 157 0 0 0 0\n"
                                  "MouseMoveEvent 161 155 0 0 0 0\n"
                                  "MouseMoveEvent 151 150 0 0 0 0\n"
                                  "MouseMoveEvent 146 148 0 0 0 0\n"
                                  "MouseMoveEvent 144 148 0 0 0 0\n"
                                  "MouseMoveEvent 143 147 0 0 0 0\n"
                                  "LeftButtonReleaseEvent 143 147 0 0 0 0\n"
                                  "MouseMoveEvent 144 146 0 0 0 0\n"
                                  "MouseMoveEvent 306 229 0 0 0 0\n"
                                  "LeftButtonPressEvent 306 229 0 0 0 0\n"
                                  "MouseMoveEvent 306 227 0 0 0 0\n"
                                  "MouseMoveEvent 305 220 0 0 0 0\n"
                                  "MouseMoveEvent 305 209 0 0 0 0\n"
                                  "MouseMoveEvent 305 197 0 0 0 0\n"
                                  "MouseMoveEvent 306 190 0 0 0 0\n"
                                  "MouseMoveEvent 313 172 0 0 0 0\n"
                                  "MouseMoveEvent 318 165 0 0 0 0\n"
                                  "MouseMoveEvent 321 159 0 0 0 0\n"
                                  "MouseMoveEvent 324 155 0 0 0 0\n"
                                  "MouseMoveEvent 328 149 0 0 0 0\n"
                                  "MouseMoveEvent 330 145 0 0 0 0\n"
                                  "MouseMoveEvent 334 140 0 0 0 0\n"
                                  "MouseMoveEvent 339 134 0 0 0 0\n"
                                  "MouseMoveEvent 343 130 0 0 0 0\n"
                                  "MouseMoveEvent 347 126 0 0 0 0\n"
                                  "MouseMoveEvent 351 121 0 0 0 0\n"
                                  "MouseMoveEvent 357 118 0 0 0 0\n"
                                  "MouseMoveEvent 361 115 0 0 0 0\n"
                                  "MouseMoveEvent 366 113 0 0 0 0\n"
                                  "MouseMoveEvent 369 112 0 0 0 0\n"
                                  "MouseMoveEvent 370 111 0 0 0 0\n"
                                  "MouseMoveEvent 373 110 0 0 0 0\n"
                                  "MouseMoveEvent 378 108 0 0 0 0\n"
                                  "MouseMoveEvent 383 107 0 0 0 0\n"
                                  "MouseMoveEvent 387 104 0 0 0 0\n"
                                  "MouseMoveEvent 391 101 0 0 0 0\n"
                                  "MouseMoveEvent 395 99 0 0 0 0\n"
                                  "MouseMoveEvent 404 93 0 0 0 0\n"
                                  "MouseMoveEvent 411 88 0 0 0 0\n"
                                  "MouseMoveEvent 414 87 0 0 0 0\n"
                                  "MouseMoveEvent 415 87 0 0 0 0\n"
                                  "LeftButtonReleaseEvent 415 87 0 0 0 0\n"
                                  "MouseMoveEvent 415 88 0 0 0 0\n"
                                  "MouseMoveEvent 304 230 0 0 0 0\n"
                                  "LeftButtonPressEvent 304 230 0 0 0 0\n"
                                  "MouseMoveEvent 304 228 0 0 0 0\n"
                                  "MouseMoveEvent 304 223 0 0 0 0\n"
                                  "MouseMoveEvent 304 214 0 0 0 0\n"
                                  "MouseMoveEvent 304 207 0 0 0 0\n"
                                  "MouseMoveEvent 304 200 0 0 0 0\n"
                                  "MouseMoveEvent 304 196 0 0 0 0\n"
                                  "MouseMoveEvent 299 189 0 0 0 0\n"
                                  "MouseMoveEvent 294 182 0 0 0 0\n"
                                  "MouseMoveEvent 293 180 0 0 0 0\n"
                                  "MouseMoveEvent 292 178 0 0 0 0\n"
                                  "MouseMoveEvent 290 176 0 0 0 0\n"
                                  "MouseMoveEvent 285 172 0 0 0 0\n"
                                  "MouseMoveEvent 281 168 0 0 0 0\n"
                                  "MouseMoveEvent 279 167 0 0 0 0\n"
                                  "MouseMoveEvent 277 167 0 0 0 0\n"
                                  "MouseMoveEvent 275 166 0 0 0 0\n"
                                  "MouseMoveEvent 270 164 0 0 0 0\n"
                                  "MouseMoveEvent 265 163 0 0 0 0\n"
                                  "MouseMoveEvent 262 163 0 0 0 0\n"
                                  "MouseMoveEvent 261 162 0 0 0 0\n"
                                  "MouseMoveEvent 260 162 0 0 0 0\n"
                                  "MouseMoveEvent 259 162 0 0 0 0\n"
                                  "MouseMoveEvent 254 162 0 0 0 0\n"
                                  "MouseMoveEvent 250 162 0 0 0 0\n"
                                  "MouseMoveEvent 248 163 0 0 0 0\n"
                                  "MouseMoveEvent 247 164 0 0 0 0\n"
                                  "MouseMoveEvent 246 164 0 0 0 0\n"
                                  "MouseMoveEvent 241 165 0 0 0 0\n"
                                  "MouseMoveEvent 240 166 0 0 0 0\n"
                                  "MouseMoveEvent 238 166 0 0 0 0\n"
                                  "MouseMoveEvent 237 167 0 0 0 0\n"
                                  "MouseMoveEvent 236 168 0 0 0 0\n"
                                  "MouseMoveEvent 233 170 0 0 0 0\n"
                                  "MouseMoveEvent 228 173 0 0 0 0\n"
                                  "MouseMoveEvent 227 174 0 0 0 0\n"
                                  "MouseMoveEvent 224 176 0 0 0 0\n"
                                  "MouseMoveEvent 221 178 0 0 0 0\n"
                                  "MouseMoveEvent 220 179 0 0 0 0\n"
                                  "MouseMoveEvent 219 180 0 0 0 0\n"
                                  "MouseMoveEvent 218 181 0 0 0 0\n"
                                  "MouseMoveEvent 212 186 0 0 0 0\n"
                                  "MouseMoveEvent 207 188 0 0 0 0\n"
                                  "MouseMoveEvent 203 192 0 0 0 0\n"
                                  "MouseMoveEvent 199 195 0 0 0 0\n"
                                  "MouseMoveEvent 197 195 0 0 0 0\n"
                                  "LeftButtonReleaseEvent 197 195 0 0 0 0\n"
                                  "MouseMoveEvent 197 195 0 0 0 0\n"
                                  "MouseMoveEvent 338 229 0 0 0 0\n"
                                  "LeftButtonPressEvent 338 229 0 0 0 0\n"
                                  "MouseMoveEvent 338 229 0 0 0 0\n"
                                  "MouseMoveEvent 340 224 0 0 0 0\n"
                                  "MouseMoveEvent 342 218 0 0 0 0\n"
                                  "MouseMoveEvent 342 213 0 0 0 0\n"
                                  "MouseMoveEvent 343 210 0 0 0 0\n"
                                  "MouseMoveEvent 343 209 0 0 0 0\n"
                                  "MouseMoveEvent 343 205 0 0 0 0\n"
                                  "MouseMoveEvent 344 202 0 0 0 0\n"
                                  "MouseMoveEvent 345 200 0 0 0 0\n"
                                  "MouseMoveEvent 345 196 0 0 0 0\n"
                                  "MouseMoveEvent 346 193 0 0 0 0\n"
                                  "MouseMoveEvent 346 190 0 0 0 0\n"
                                  "MouseMoveEvent 346 189 0 0 0 0\n"
                                  "MouseMoveEvent 346 186 0 0 0 0\n"
                                  "MouseMoveEvent 346 184 0 0 0 0\n"
                                  "MouseMoveEvent 347 179 0 0 0 0\n"
                                  "MouseMoveEvent 348 174 0 0 0 0\n"
                                  "MouseMoveEvent 348 171 0 0 0 0\n"
                                  "MouseMoveEvent 348 168 0 0 0 0\n"
                                  "MouseMoveEvent 348 165 0 0 0 0\n"
                                  "MouseMoveEvent 348 164 0 0 0 0\n"
                                  "MouseMoveEvent 350 159 0 0 0 0\n"
                                  "MouseMoveEvent 350 156 0 0 0 0\n"
                                  "MouseMoveEvent 350 151 0 0 0 0\n"
                                  "MouseMoveEvent 350 148 0 0 0 0\n"
                                  "MouseMoveEvent 351 144 0 0 0 0\n"
                                  "MouseMoveEvent 352 142 0 0 0 0\n"
                                  "MouseMoveEvent 352 141 0 0 0 0\n"
                                  "MouseMoveEvent 352 140 0 0 0 0\n"
                                  "MouseMoveEvent 352 138 0 0 0 0\n"
                                  "MouseMoveEvent 352 133 0 0 0 0\n"
                                  "MouseMoveEvent 353 130 0 0 0 0\n"
                                  "MouseMoveEvent 353 128 0 0 0 0\n"
                                  "MouseMoveEvent 354 124 0 0 0 0\n"
                                  "MouseMoveEvent 355 123 0 0 0 0\n"
                                  "MouseMoveEvent 355 119 0 0 0 0\n"
                                  "MouseMoveEvent 356 115 0 0 0 0\n"
                                  "MouseMoveEvent 356 114 0 0 0 0\n"
                                  "MouseMoveEvent 356 109 0 0 0 0\n"
                                  "MouseMoveEvent 356 106 0 0 0 0\n"
                                  "MouseMoveEvent 357 104 0 0 0 0\n"
                                  "MouseMoveEvent 357 100 0 0 0 0\n"
                                  "MouseMoveEvent 358 98 0 0 0 0\n"
                                  "MouseMoveEvent 358 95 0 0 0 0\n"
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
                                  "MouseMoveEvent 364 53 0 0 0 0\n"
                                  "MouseMoveEvent 308 195 0 0 0 0\n"
                                  "LeftButtonPressEvent 308 195 0 0 0 0\n"
                                  "MouseMoveEvent 308 194 0 0 0 0\n"
                                  "MouseMoveEvent 305 187 0 0 0 0\n"
                                  "MouseMoveEvent 304 185 0 0 0 0\n"
                                  "MouseMoveEvent 302 176 0 0 0 0\n"
                                  "MouseMoveEvent 300 168 0 0 0 0\n"
                                  "MouseMoveEvent 298 162 0 0 0 0\n"
                                  "MouseMoveEvent 297 156 0 0 0 0\n"
                                  "MouseMoveEvent 297 153 0 0 0 0\n"
                                  "MouseMoveEvent 296 148 0 0 0 0\n"
                                  "MouseMoveEvent 295 145 0 0 0 0\n"
                                  "MouseMoveEvent 293 140 0 0 0 0\n"
                                  "MouseMoveEvent 292 137 0 0 0 0\n"
                                  "MouseMoveEvent 292 133 0 0 0 0\n"
                                  "MouseMoveEvent 290 130 0 0 0 0\n"
                                  "MouseMoveEvent 290 128 0 0 0 0\n"
                                  "MouseMoveEvent 289 123 0 0 0 0\n"
                                  "MouseMoveEvent 288 119 0 0 0 0\n"
                                  "MouseMoveEvent 288 115 0 0 0 0\n"
                                  "MouseMoveEvent 288 113 0 0 0 0\n"
                                  "MouseMoveEvent 288 110 0 0 0 0\n"
                                  "MouseMoveEvent 287 108 0 0 0 0\n"
                                  "MouseMoveEvent 287 104 0 0 0 0\n"
                                  "MouseMoveEvent 286 101 0 0 0 0\n"
                                  "MouseMoveEvent 285 99 0 0 0 0\n"
                                  "MouseMoveEvent 285 98 0 0 0 0\n"
                                  "MouseMoveEvent 285 97 0 0 0 0\n"
                                  "MouseMoveEvent 284 95 0 0 0 0\n"
                                  "MouseMoveEvent 284 94 0 0 0 0\n"
                                  "MouseMoveEvent 284 92 0 0 0 0\n"
                                  "MouseMoveEvent 283 89 0 0 0 0\n"
                                  "MouseMoveEvent 282 88 0 0 0 0\n"
                                  "MouseMoveEvent 282 87 0 0 0 0\n"
                                  "MouseMoveEvent 281 86 0 0 0 0\n"
                                  "MouseMoveEvent 280 85 0 0 0 0\n"
                                  "MouseMoveEvent 279 83 0 0 0 0\n"
                                  "MouseMoveEvent 278 79 0 0 0 0\n"
                                  "MouseMoveEvent 276 77 0 0 0 0\n"
                                  "MouseMoveEvent 274 76 0 0 0 0\n"
                                  "MouseMoveEvent 272 74 0 0 0 0\n"
                                  "LeftButtonReleaseEvent 272 74 0 0 0 0\n"
                                  "MouseMoveEvent 272 75 0 0 0 0\n"
                                  "LeftButtonPressEvent 154 198 0 0 0 0\n"
                                  "MouseMoveEvent 155 194 0 0 0 0\n"
                                  "MouseMoveEvent 155 187 0 0 0 0\n"
                                  "MouseMoveEvent 157 180 0 0 0 0\n"
                                  "MouseMoveEvent 166 165 0 0 0 0\n"
                                  "MouseMoveEvent 170 158 0 0 0 0\n"
                                  "MouseMoveEvent 179 153 0 0 0 0\n"
                                  "MouseMoveEvent 187 150 0 0 0 0\n"
                                  "MouseMoveEvent 200 148 0 0 0 0\n"
                                  "MouseMoveEvent 216 148 0 0 0 0\n"
                                  "MouseMoveEvent 231 148 0 0 0 0\n"
                                  "MouseMoveEvent 237 148 0 0 0 0\n"
                                  "MouseMoveEvent 245 146 0 0 0 0\n"
                                  "MouseMoveEvent 252 146 0 0 0 0\n"
                                  "MouseMoveEvent 259 146 0 0 0 0\n"
                                  "MouseMoveEvent 269 146 0 0 0 0\n"
                                  "MouseMoveEvent 284 146 0 0 0 0\n"
                                  "MouseMoveEvent 302 146 0 0 0 0\n"
                                  "MouseMoveEvent 319 146 0 0 0 0\n"
                                  "MouseMoveEvent 332 146 0 0 0 0\n"
                                  "MouseMoveEvent 346 146 0 0 0 0\n"
                                  "MouseMoveEvent 357 147 0 0 0 0\n"
                                  "MouseMoveEvent 369 148 0 0 0 0\n"
                                  "MouseMoveEvent 376 149 0 0 0 0\n"
                                  "MouseMoveEvent 380 151 0 0 0 0\n"
                                  "MouseMoveEvent 382 152 0 0 0 0\n"
                                  "MouseMoveEvent 383 152 0 0 0 0\n"
                                  "MouseMoveEvent 383 152 0 0 0 0\n"
                                  "MouseMoveEvent 385 153 0 0 0 0\n"
                                  "MouseMoveEvent 386 153 0 0 0 0\n"
                                  "MouseMoveEvent 388 154 0 0 0 0\n"
                                  "MouseMoveEvent 390 155 0 0 0 0\n"
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
