/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCoincidentHandleWidgets.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This is a test for a regression with coincident handle widgets picking issues
// when a picking manager is used.

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkHandleWidget.h"
#include "vtkNew.h"
#include "vtkPickingManager.h"
#include "vtkPointHandleRepresentation3D.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSeedRepresentation.h"
#include "vtkSeedWidget.h"
#include "vtkTesting.h"

const char TestCoincidentHandleWidgetsLog[] = "# StreamVersion 1.1\n"
                                              "EnterEvent 155 0 0 0 0 0\n"
                                              "MouseMoveEvent 155 0 0 0 0 0\n"
                                              "RenderEvent 155 0 0 0 0 0\n"
                                              "MouseMoveEvent 154 0 0 0 0 0\n"
                                              "RenderEvent 154 0 0 0 0 0\n"
                                              "MouseMoveEvent 153 1 0 0 0 0\n"
                                              "RenderEvent 153 1 0 0 0 0\n"
                                              "MouseMoveEvent 152 2 0 0 0 0\n"
                                              "RenderEvent 152 2 0 0 0 0\n"
                                              "MouseMoveEvent 151 2 0 0 0 0\n"
                                              "RenderEvent 151 2 0 0 0 0\n"
                                              "MouseMoveEvent 149 5 0 0 0 0\n"
                                              "RenderEvent 149 5 0 0 0 0\n"
                                              "MouseMoveEvent 148 6 0 0 0 0\n"
                                              "RenderEvent 148 6 0 0 0 0\n"
                                              "MouseMoveEvent 147 7 0 0 0 0\n"
                                              "RenderEvent 147 7 0 0 0 0\n"
                                              "MouseMoveEvent 146 9 0 0 0 0\n"
                                              "RenderEvent 146 9 0 0 0 0\n"
                                              "MouseMoveEvent 144 11 0 0 0 0\n"
                                              "RenderEvent 144 11 0 0 0 0\n"
                                              "MouseMoveEvent 139 14 0 0 0 0\n"
                                              "RenderEvent 139 14 0 0 0 0\n"
                                              "MouseMoveEvent 133 24 0 0 0 0\n"
                                              "RenderEvent 133 24 0 0 0 0\n"
                                              "MouseMoveEvent 127 34 0 0 0 0\n"
                                              "RenderEvent 127 34 0 0 0 0\n"
                                              "MouseMoveEvent 124 40 0 0 0 0\n"
                                              "RenderEvent 124 40 0 0 0 0\n"
                                              "MouseMoveEvent 124 51 0 0 0 0\n"
                                              "RenderEvent 124 51 0 0 0 0\n"
                                              "MouseMoveEvent 124 59 0 0 0 0\n"
                                              "RenderEvent 124 59 0 0 0 0\n"
                                              "MouseMoveEvent 124 68 0 0 0 0\n"
                                              "RenderEvent 124 68 0 0 0 0\n"
                                              "MouseMoveEvent 126 79 0 0 0 0\n"
                                              "RenderEvent 126 79 0 0 0 0\n"
                                              "MouseMoveEvent 129 90 0 0 0 0\n"
                                              "RenderEvent 129 90 0 0 0 0\n"
                                              "MouseMoveEvent 131 97 0 0 0 0\n"
                                              "RenderEvent 131 97 0 0 0 0\n"
                                              "MouseMoveEvent 132 102 0 0 0 0\n"
                                              "RenderEvent 132 102 0 0 0 0\n"
                                              "MouseMoveEvent 133 105 0 0 0 0\n"
                                              "RenderEvent 133 105 0 0 0 0\n"
                                              "MouseMoveEvent 134 106 0 0 0 0\n"
                                              "RenderEvent 134 106 0 0 0 0\n"
                                              "MouseMoveEvent 134 108 0 0 0 0\n"
                                              "RenderEvent 134 108 0 0 0 0\n"
                                              "MouseMoveEvent 135 110 0 0 0 0\n"
                                              "RenderEvent 135 110 0 0 0 0\n"
                                              "MouseMoveEvent 136 112 0 0 0 0\n"
                                              "RenderEvent 136 112 0 0 0 0\n"
                                              "MouseMoveEvent 137 114 0 0 0 0\n"
                                              "RenderEvent 137 114 0 0 0 0\n"
                                              "MouseMoveEvent 137 116 0 0 0 0\n"
                                              "RenderEvent 137 116 0 0 0 0\n"
                                              "MouseMoveEvent 137 118 0 0 0 0\n"
                                              "RenderEvent 137 118 0 0 0 0\n"
                                              "MouseMoveEvent 138 121 0 0 0 0\n"
                                              "RenderEvent 138 121 0 0 0 0\n"
                                              "MouseMoveEvent 139 126 0 0 0 0\n"
                                              "RenderEvent 139 126 0 0 0 0\n"
                                              "MouseMoveEvent 140 131 0 0 0 0\n"
                                              "RenderEvent 140 131 0 0 0 0\n"
                                              "MouseMoveEvent 142 136 0 0 0 0\n"
                                              "RenderEvent 142 136 0 0 0 0\n"
                                              "MouseMoveEvent 143 141 0 0 0 0\n"
                                              "RenderEvent 143 141 0 0 0 0\n"
                                              "MouseMoveEvent 144 145 0 0 0 0\n"
                                              "RenderEvent 144 145 0 0 0 0\n"
                                              "MouseMoveEvent 145 149 0 0 0 0\n"
                                              "RenderEvent 145 149 0 0 0 0\n"
                                              "MouseMoveEvent 145 151 0 0 0 0\n"
                                              "RenderEvent 145 151 0 0 0 0\n"
                                              "MouseMoveEvent 145 152 0 0 0 0\n"
                                              "RenderEvent 145 152 0 0 0 0\n"
                                              "MouseMoveEvent 146 153 0 0 0 0\n"
                                              "RenderEvent 146 153 0 0 0 0\n"
                                              "LeftButtonPressEvent 146 153 0 0 0 0\n"
                                              "RenderEvent 146 153 0 0 0 0\n"
                                              "MouseMoveEvent 146 153 0 0 0 0\n"
                                              "RenderEvent 146 153 0 0 0 0\n"
                                              "MouseMoveEvent 147 153 0 0 0 0\n"
                                              "RenderEvent 147 153 0 0 0 0\n"
                                              "MouseMoveEvent 147 152 0 0 0 0\n"
                                              "RenderEvent 147 152 0 0 0 0\n"
                                              "MouseMoveEvent 148 150 0 0 0 0\n"
                                              "RenderEvent 148 150 0 0 0 0\n"
                                              "MouseMoveEvent 150 149 0 0 0 0\n"
                                              "RenderEvent 150 149 0 0 0 0\n"
                                              "MouseMoveEvent 151 147 0 0 0 0\n"
                                              "RenderEvent 151 147 0 0 0 0\n"
                                              "MouseMoveEvent 151 146 0 0 0 0\n"
                                              "RenderEvent 151 146 0 0 0 0\n"
                                              "MouseMoveEvent 153 145 0 0 0 0\n"
                                              "RenderEvent 153 145 0 0 0 0\n"
                                              "MouseMoveEvent 153 144 0 0 0 0\n"
                                              "RenderEvent 153 144 0 0 0 0\n"
                                              "MouseMoveEvent 154 144 0 0 0 0\n"
                                              "RenderEvent 154 144 0 0 0 0\n"
                                              "MouseMoveEvent 154 143 0 0 0 0\n"
                                              "RenderEvent 154 143 0 0 0 0\n"
                                              "MouseMoveEvent 155 142 0 0 0 0\n"
                                              "RenderEvent 155 142 0 0 0 0\n"
                                              "MouseMoveEvent 156 142 0 0 0 0\n"
                                              "RenderEvent 156 142 0 0 0 0\n"
                                              "MouseMoveEvent 157 141 0 0 0 0\n"
                                              "RenderEvent 157 141 0 0 0 0\n"
                                              "MouseMoveEvent 158 140 0 0 0 0\n"
                                              "RenderEvent 158 140 0 0 0 0\n"
                                              "MouseMoveEvent 159 139 0 0 0 0\n"
                                              "RenderEvent 159 139 0 0 0 0\n"
                                              "MouseMoveEvent 159 139 0 0 0 0\n"
                                              "RenderEvent 159 139 0 0 0 0\n"
                                              "MouseMoveEvent 160 139 0 0 0 0\n"
                                              "RenderEvent 160 139 0 0 0 0\n"
                                              "MouseMoveEvent 160 138 0 0 0 0\n"
                                              "RenderEvent 160 138 0 0 0 0\n"
                                              "MouseMoveEvent 161 137 0 0 0 0\n"
                                              "RenderEvent 161 137 0 0 0 0\n"
                                              "MouseMoveEvent 162 137 0 0 0 0\n"
                                              "RenderEvent 162 137 0 0 0 0\n"
                                              "MouseMoveEvent 162 137 0 0 0 0\n"
                                              "RenderEvent 162 137 0 0 0 0\n"
                                              "MouseMoveEvent 163 137 0 0 0 0\n"
                                              "RenderEvent 163 137 0 0 0 0\n"
                                              "MouseMoveEvent 164 136 0 0 0 0\n"
                                              "RenderEvent 164 136 0 0 0 0\n"
                                              "MouseMoveEvent 165 136 0 0 0 0\n"
                                              "RenderEvent 165 136 0 0 0 0\n"
                                              "MouseMoveEvent 166 136 0 0 0 0\n"
                                              "RenderEvent 166 136 0 0 0 0\n"
                                              "MouseMoveEvent 166 135 0 0 0 0\n"
                                              "RenderEvent 166 135 0 0 0 0\n"
                                              "LeftButtonReleaseEvent 166 135 0 0 0 0\n"
                                              "RenderEvent 166 135 0 0 0 0\n";

int TestCoincidentHandleWidgets(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetSize(301, 300); // Intentional NPOT size

  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren.GetPointer());

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());
  // Set up the picking manager
  iren->GetPickingManager()->EnabledOn();

  vtkNew<vtkSeedWidget> seedWidget;
  vtkNew<vtkSeedRepresentation> rep;
  seedWidget->SetRepresentation(rep);
  seedWidget->CompleteInteraction();
  seedWidget->RestartInteraction();
  vtkNew<vtkPointHandleRepresentation3D> handle;
  handle->GetProperty()->SetColor(1, 0, 0);
  handle->GetProperty()->SetLineWidth(2);
  handle->GetSelectedProperty()->SetColor(1, 1, 0);
  handle->GetSelectedProperty()->SetLineWidth(4);
  rep->SetHandleRepresentation(handle);
  seedWidget->SetInteractor(iren);
  seedWidget->On();

  vtkHandleWidget* handleWidget = seedWidget->CreateNewHandle();
  handleWidget->EnabledOn();
  vtkHandleRepresentation* handleRep = rep->GetHandleRepresentation(0);
  double coords[3] = { 150, 150, 0 };
  rep->SetSeedDisplayPosition(0, coords);
  handleRep->VisibilityOn();
  seedWidget->GetSeed(0)->EnabledOn();

  vtkHandleWidget* handleWidget2 = seedWidget->CreateNewHandle();
  handleWidget2->EnabledOff();
  vtkHandleRepresentation* handleRep2 = rep->GetHandleRepresentation(1);
  rep->SetSeedDisplayPosition(1, coords);
  handleRep2->VisibilityOff();
  seedWidget->GetSeed(1)->EnabledOff();

  iren->Initialize();
  renWin->Render();
  seedWidget->CompleteInteraction();

  // Exercise the test
  return vtkTesting::InteractorEventLoop(argc, argv, iren, TestCoincidentHandleWidgetsLog);
}
