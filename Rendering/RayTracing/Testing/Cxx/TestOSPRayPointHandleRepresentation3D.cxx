/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSPRayPointHandleRepresentation3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Description
// This tests the vtkPointHandleRepresentation3D using the ospray backend

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkHandleWidget.h>
#include <vtkNew.h>
#include <vtkOSPRayPass.h>
#include <vtkOSPRayRendererNode.h>
#include <vtkPointHandleRepresentation3D.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSeedRepresentation.h>
#include <vtkSeedWidget.h>
#include <vtkTestUtilities.h>
#include <vtkTesting.h>

#include "vtkOSPRayTestInteractor.h"

const char TestOSPRayPointHandleRepresentation3DLog[] = "# StreamVersion 1.1\n"
                                                        "ExposeEvent 0 299 0 0 0 0\n"
                                                        "MouseMoveEvent 269 268 0 0 0 0\n"
                                                        "LeftButtonPressEvent 269 268 0 0 0 0\n"
                                                        "MouseMoveEvent 268 268 0 0 0 0\n"
                                                        "MouseMoveEvent 266 267 0 0 0 0\n"
                                                        "MouseMoveEvent 265 266 0 0 0 0\n"
                                                        "MouseMoveEvent 261 263 0 0 0 0\n"
                                                        "MouseMoveEvent 256 261 0 0 0 0\n"
                                                        "MouseMoveEvent 248 256 0 0 0 0\n"
                                                        "MouseMoveEvent 237 250 0 0 0 0\n"
                                                        "MouseMoveEvent 233 247 0 0 0 0\n"
                                                        "MouseMoveEvent 228 245 0 0 0 0\n"
                                                        "MouseMoveEvent 221 243 0 0 0 0\n"
                                                        "MouseMoveEvent 217 241 0 0 0 0\n"
                                                        "MouseMoveEvent 209 237 0 0 0 0\n"
                                                        "MouseMoveEvent 206 236 0 0 0 0\n"
                                                        "MouseMoveEvent 201 234 0 0 0 0\n"
                                                        "MouseMoveEvent 197 231 0 0 0 0\n"
                                                        "MouseMoveEvent 191 227 0 0 0 0\n"
                                                        "MouseMoveEvent 187 225 0 0 0 0\n"
                                                        "MouseMoveEvent 182 220 0 0 0 0\n"
                                                        "MouseMoveEvent 178 218 0 0 0 0\n"
                                                        "MouseMoveEvent 178 217 0 0 0 0\n"
                                                        "MouseMoveEvent 179 217 0 0 0 0\n"
                                                        "MouseMoveEvent 179 217 0 0 0 0\n"
                                                        "MouseMoveEvent 180 217 0 0 0 0\n"
                                                        "MouseMoveEvent 181 217 0 0 0 0\n"
                                                        "MouseMoveEvent 182 217 0 0 0 0\n"
                                                        "MouseMoveEvent 182 218 0 0 0 0\n"
                                                        "LeftButtonReleaseEvent 182 218 0 0 0 0\n"
                                                        "LeftButtonPressEvent 97 117 0 0 0 0\n"
                                                        "MouseMoveEvent 97 116 0 0 0 0\n"
                                                        "MouseMoveEvent 96 117 0 0 0 0\n"
                                                        "MouseMoveEvent 94 118 0 0 0 0\n"
                                                        "MouseMoveEvent 93 119 0 0 0 0\n"
                                                        "MouseMoveEvent 90 121 0 0 0 0\n"
                                                        "MouseMoveEvent 87 123 0 0 0 0\n"
                                                        "MouseMoveEvent 85 125 0 0 0 0\n"
                                                        "MouseMoveEvent 82 127 0 0 0 0\n"
                                                        "MouseMoveEvent 81 128 0 0 0 0\n"
                                                        "MouseMoveEvent 80 128 0 0 0 0\n"
                                                        "MouseMoveEvent 79 128 0 0 0 0\n"
                                                        "MouseMoveEvent 77 128 0 0 0 0\n"
                                                        "MouseMoveEvent 76 128 0 0 0 0\n"
                                                        "MouseMoveEvent 76 128 0 0 0 0\n"
                                                        "MouseMoveEvent 75 129 0 0 0 0\n"
                                                        "MouseMoveEvent 74 130 0 0 0 0\n"
                                                        "MouseMoveEvent 73 132 0 0 0 0\n"
                                                        "MouseMoveEvent 71 134 0 0 0 0\n"
                                                        "MouseMoveEvent 71 133 0 0 0 0\n"
                                                        "MouseMoveEvent 71 130 0 0 0 0\n"
                                                        "MouseMoveEvent 72 127 0 0 0 0\n"
                                                        "MouseMoveEvent 72 126 0 0 0 0\n"
                                                        "MouseMoveEvent 73 121 0 0 0 0\n"
                                                        "MouseMoveEvent 74 116 0 0 0 0\n"
                                                        "MouseMoveEvent 75 112 0 0 0 0\n"
                                                        "MouseMoveEvent 79 107 0 0 0 0\n"
                                                        "MouseMoveEvent 80 105 0 0 0 0\n"
                                                        "MouseMoveEvent 83 100 0 0 0 0\n"
                                                        "MouseMoveEvent 84 98 0 0 0 0\n"
                                                        "MouseMoveEvent 85 96 0 0 0 0\n"
                                                        "MouseMoveEvent 86 95 0 0 0 0\n"
                                                        "MouseMoveEvent 87 95 0 0 0 0\n"
                                                        "MouseMoveEvent 55 139 0 0 0 0\n"
                                                        "LeftButtonReleaseEvent 55 139 0 0 0 0\n"
                                                        "LeftButtonPressEvent 183 226 0 0 0 0\n"
                                                        "MouseMoveEvent 183 225 0 0 0 0\n"
                                                        "MouseMoveEvent 183 223 0 0 0 0\n"
                                                        "MouseMoveEvent 183 222 0 0 0 0\n"
                                                        "MouseMoveEvent 184 218 0 0 0 0\n"
                                                        "MouseMoveEvent 187 212 0 0 0 0\n"
                                                        "MouseMoveEvent 188 210 0 0 0 0\n"
                                                        "MouseMoveEvent 192 201 0 0 0 0\n"
                                                        "MouseMoveEvent 193 200 0 0 0 0\n"
                                                        "MouseMoveEvent 195 193 0 0 0 0\n"
                                                        "MouseMoveEvent 198 187 0 0 0 0\n"
                                                        "MouseMoveEvent 200 180 0 0 0 0\n"
                                                        "MouseMoveEvent 206 165 0 0 0 0\n"
                                                        "MouseMoveEvent 210 162 0 0 0 0\n"
                                                        "MouseMoveEvent 212 157 0 0 0 0\n"
                                                        "MouseMoveEvent 215 150 0 0 0 0\n"
                                                        "MouseMoveEvent 219 142 0 0 0 0\n"
                                                        "MouseMoveEvent 221 139 0 0 0 0\n"
                                                        "MouseMoveEvent 224 135 0 0 0 0\n"
                                                        "MouseMoveEvent 225 133 0 0 0 0\n"
                                                        "MouseMoveEvent 227 131 0 0 0 0\n"
                                                        "MouseMoveEvent 229 130 0 0 0 0\n"
                                                        "MouseMoveEvent 230 131 0 0 0 0\n"
                                                        "MouseMoveEvent 230 133 0 0 0 0\n"
                                                        "MouseMoveEvent 229 135 0 0 0 0\n"
                                                        "MouseMoveEvent 229 137 0 0 0 0\n"
                                                        "MouseMoveEvent 228 143 0 0 0 0\n"
                                                        "MouseMoveEvent 227 148 0 0 0 0\n"
                                                        "MouseMoveEvent 227 153 0 0 0 0\n"
                                                        "MouseMoveEvent 226 157 0 0 0 0\n"
                                                        "MouseMoveEvent 226 159 0 0 0 0\n"
                                                        "MouseMoveEvent 226 162 0 0 0 0\n"
                                                        "MouseMoveEvent 226 165 0 0 0 0\n"
                                                        "MouseMoveEvent 226 169 0 0 0 0\n"
                                                        "MouseMoveEvent 226 170 0 0 0 0\n"
                                                        "MouseMoveEvent 226 172 0 0 0 0\n"
                                                        "MouseMoveEvent 226 173 0 0 0 0\n"
                                                        "MouseMoveEvent 226 174 0 0 0 0\n"
                                                        "MouseMoveEvent 226 175 0 0 0 0\n"
                                                        "LeftButtonReleaseEvent 226 175 0 0 0 0\n"
                                                        "MouseMoveEvent 93 235 0 0 0 0\n"
                                                        "LeftButtonPressEvent 93 235 0 0 0 0\n"
                                                        "LeftButtonReleaseEvent 93 235 0 0 0 0\n"
                                                        "MouseMoveEvent 97 234 0 0 0 0\n"
                                                        "LeftButtonPressEvent 97 234 0 0 0 0\n"
                                                        "MouseMoveEvent 160 191 0 0 0 0\n"
                                                        "LeftButtonReleaseEvent 160 191 0 0 0 0\n"
                                                        "MouseMoveEvent 262 32 0 0 0 0\n"
                                                        "";

int TestOSPRayPointHandleRepresentation3D(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  // Create the RenderWindow, Renderer and both Actors
  //
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer);

  for (int i = 0; i < argc; ++i)
  {
    if (!strcmp(argv[i], "--OptiX"))
    {
      vtkOSPRayRendererNode::SetRendererType("optix pathtracer", renderer);
      break;
    }
  }

  // Create the widget and its representation
  vtkNew<vtkPointHandleRepresentation3D> handlePointRep3D;
  handlePointRep3D->AllOn();
  handlePointRep3D->GetProperty()->SetColor(1., 0., 1.);
  handlePointRep3D->GetProperty()->SetLineWidth(2.0);
  handlePointRep3D->GetProperty()->SetAmbient(0.0);
  handlePointRep3D->GetSelectedProperty()->SetLineWidth(4.0);
  handlePointRep3D->GetSelectedProperty()->SetAmbient(0.0);

  vtkNew<vtkSeedRepresentation> seedRep;
  seedRep->SetHandleRepresentation(handlePointRep3D);

  vtkNew<vtkSeedWidget> seedWidget;

  seedWidget->SetRepresentation(seedRep);
  seedWidget->SetInteractor(iren);
  seedWidget->On();
  renWin->Render();

  // Place two different points in different translation mode.
  double bounds[6] = { 0.1, 0.25, 0.1, 0.25, 0.1, 0.25 };
  double bounds2[6] = { -0.2, 0, -0.2, 0, -0.2, 0 };

  vtkHandleWidget* currentHandle = seedWidget->CreateNewHandle();
  currentHandle->SetEnabled(1);
  vtkPointHandleRepresentation3D* handleRep =
    vtkPointHandleRepresentation3D::SafeDownCast(currentHandle->GetRepresentation());
  handleRep->PlaceWidget(bounds);

  currentHandle = seedWidget->CreateNewHandle();
  currentHandle->SetEnabled(1);
  handleRep = vtkPointHandleRepresentation3D::SafeDownCast(currentHandle->GetRepresentation());
  handleRep->TranslationModeOff();
  handleRep->PlaceWidget(bounds2);

  seedWidget->Off();
  seedWidget->On();

  // Add the actors to the renderer, set the background and size
  //
  renderer->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  vtkNew<vtkOSPRayPass> ospray;
  renderer->SetPass(ospray);
  vtkNew<vtkOSPRayTestInteractor> style;
  style->SetPipelineControlPoints(renderer, ospray, nullptr);
  iren->SetInteractorStyle(style);
  style->SetCurrentRenderer(renderer);

  // render the image
  iren->Initialize();
  renWin->Render();

  return vtkTesting::InteractorEventLoop(
    argc, argv, iren, TestOSPRayPointHandleRepresentation3DLog);
}
