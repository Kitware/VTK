// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#include "vtkDataRepresentation.h"
#include "vtkDataSetAttributes.h"
#include "vtkGraphLayoutView.h"
#include "vtkIdTypeArray.h"
#include "vtkNew.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderedGraphRepresentation.h"
#include "vtkScalarBarActor.h"
#include "vtkScalarBarWidget.h"
#include "vtkStringArray.h"
#include "vtkStringToNumeric.h"
#include "vtkTesting.h"
#include "vtkXMLTreeReader.h"

static char GraphLayoutViewEventLog[] = "# StreamVersion 1\n"
                                        "EnterEvent 291 110 0 0 0 0 0\n"

                                        "MiddleButtonPressEvent 144 162 0 0 0 0 0\n"
                                        "StartInteractionEvent 144 162 0 0 0 0 0\n"
                                        "MouseMoveEvent 144 164 0 0 0 0 0\n"
                                        "MouseMoveEvent 139 146 0 0 0 0 0\n"
                                        "MiddleButtonReleaseEvent 139 146 0 0 0 0 0\n"
                                        "EndInteractionEvent 139 146 0 0 0 0 0\n"

                                        "LeftButtonPressEvent 136 146 0 0 0 0 0\n"
                                        "StartInteractionEvent 136 146 0 0 0 0 0\n"
                                        "MouseMoveEvent 136 148 0 0 0 0 0\n"
                                        "MouseMoveEvent 278 247 0 0 0 0 0\n"
                                        "LeftButtonReleaseEvent 278 247 0 0 0 0 0\n"
                                        "EndInteractionEvent 278 247 0 0 0 0 0\n"

                                        "RightButtonPressEvent 153 153 0 0 0 0 0\n"
                                        "StartInteractionEvent 153 153 0 0 0 0 0\n"
                                        "MouseMoveEvent 153 156 0 0 0 0 0\n"
                                        "MouseMoveEvent 118 117 0 0 0 0 0\n"
                                        "RightButtonReleaseEvent 118 117 0 0 0 0 0\n"
                                        "EndInteractionEvent 118 117 0 0 0 0 0\n"

                                        "LeftButtonPressEvent 38 147 0 0 0 0 0\n"
                                        "StartInteractionEvent 38 147 0 0 0 0 0\n"
                                        "MouseMoveEvent 39 147 0 0 0 0 0\n"
                                        "MouseMoveEvent 158 254 0 0 0 0 0\n"
                                        "LeftButtonReleaseEvent 158 254 0 0 0 0 0\n"
                                        "EndInteractionEvent 158 254 0 0 0 0 0\n"

                                        "LeftButtonPressEvent 40 137 0 0 0 0 0\n"
                                        "StartInteractionEvent 40 137 0 0 0 0 0\n"
                                        "MouseMoveEvent 41 137 0 0 0 0 0\n"
                                        "MouseMoveEvent 231 38 0 0 0 0 0\n"
                                        "LeftButtonReleaseEvent 231 38 0 0 0 0 0\n"
                                        "EndInteractionEvent 231 38 0 0 0 0 0\n"

                                        "LeftButtonPressEvent 193 126 0 0 0 0 0\n"
                                        "StartInteractionEvent 193 126 0 0 0 0 0\n"
                                        "MouseMoveEvent 191 126 0 0 0 0 0\n"
                                        "MouseMoveEvent 69 44 0 0 0 0 0\n"
                                        "LeftButtonReleaseEvent 69 44 0 0 0 0 0\n"
                                        "EndInteractionEvent 69 44 0 0 0 0 0\n"

                                        "RightButtonPressEvent 122 87 0 0 0 0 0\n"
                                        "StartInteractionEvent 122 87 0 0 0 0 0\n"
                                        "MouseMoveEvent 123 87 0 0 0 0 0\n"
                                        "MouseMoveEvent 109 47 0 0 0 0 0\n"
                                        "RightButtonReleaseEvent 109 47 0 0 0 0 0\n"
                                        "EndInteractionEvent 109 47 0 0 0 0 0\n"

                                        "MiddleButtonPressEvent 115 94 0 0 0 0 0\n"
                                        "StartInteractionEvent 115 94 0 0 0 0 0\n"
                                        "MouseMoveEvent 115 95 0 0 0 0 0\n"
                                        "MouseMoveEvent 153 132 0 0 0 0 0\n"
                                        "MiddleButtonReleaseEvent 153 132 0 0 0 0 0\n"
                                        "EndInteractionEvent 153 132 0 0 0 0 0\n"

                                        "LeaveEvent 276 299 0 0 0 0 0\n"
                                        "ExitEvent 276 299 0 0 0 0 0\n";

int TestGraphLayoutView(int argc, char* argv[])
{
  vtkNew<vtkTesting> testHelper;
  testHelper->AddArguments(argc, const_cast<const char**>(argv));
  std::string dataRoot = testHelper->GetDataRoot();
  std::string file = dataRoot + "/Data/treetest.xml";

  vtkNew<vtkXMLTreeReader> reader;
  reader->SetFileName(file.c_str());
  reader->SetMaskArrays(true);
  reader->Update();
  vtkTree* t = reader->GetOutput();
  vtkNew<vtkStringArray> label;
  label->SetName("edge label");
  vtkNew<vtkIdTypeArray> dist;
  dist->SetName("distance");
  for (vtkIdType i = 0; i < t->GetNumberOfEdges(); i++)
  {
    dist->InsertNextValue(i);
    switch (i % 3)
    {
      case 0:
        label->InsertNextValue("a");
        break;
      case 1:
        label->InsertNextValue("b");
        break;
      case 2:
        label->InsertNextValue("c");
        break;
    }
  }
  t->GetEdgeData()->AddArray(dist);
  t->GetEdgeData()->AddArray(label);

  vtkNew<vtkStringToNumeric> numeric;
  numeric->SetInputData(t);

  // Graph layout view
  vtkNew<vtkGraphLayoutView> view;
  view->DisplayHoverTextOn();
  view->SetLayoutStrategyToCircular();
  view->SetVertexLabelArrayName("name");
  view->VertexLabelVisibilityOn();
  view->SetVertexColorArrayName("size");
  view->ColorVerticesOn();
  view->SetRepresentationFromInputConnection(numeric->GetOutputPort());
  view->SetVertexScalarBarVisibility(true);
  view->SetEdgeColorArrayName("distance");
  view->ColorEdgesOn();
  view->SetEdgeLabelArrayName("edge label");
  view->EdgeLabelVisibilityOn();
  vtkRenderedGraphRepresentation* rep =
    vtkRenderedGraphRepresentation::SafeDownCast(view->GetRepresentation());
  rep->SetVertexHoverArrayName("name");
  rep->SetEdgeHoverArrayName("edge label");
  rep->GetVertexScalarBar()->GetScalarBarActor()->SetOrientation(VTK_ORIENT_HORIZONTAL);
  rep->GetVertexScalarBar()->GetScalarBarActor()->SetPosition(0.05, 0.05);
  rep->GetVertexScalarBar()->GetScalarBarActor()->SetPosition2(0.55, 0.15);

  view->ResetCamera();

  view->GetInteractor()->Initialize();
  return vtkTesting::InteractorEventLoop(
    argc, argv, view->GetInteractor(), GraphLayoutViewEventLog);
}
