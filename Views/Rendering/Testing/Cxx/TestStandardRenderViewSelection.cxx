// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkDataObject.h"
#include "vtkIdTypeArray.h"
#include "vtkInteractorStyleTerrain.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkNew.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSphereSource.h"
#include "vtkStandardRenderView.h"
#include "vtkSurfaceRepresentation.h"

#include <iostream>

#define CHECK(expr, msg)                                                                           \
  do                                                                                               \
  {                                                                                                \
    if (!(expr))                                                                                   \
    {                                                                                              \
      std::cerr << "FAILED: " << msg << "\n";                                                      \
      return EXIT_FAILURE;                                                                         \
    }                                                                                              \
  } while (false)

int TestStandardRenderViewSelection(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // Create view with off-screen rendering
  vtkNew<vtkStandardRenderView> view;
  view->GetRenderWindow()->SetOffScreenRendering(true);
  view->SetWindowSize(400, 400);

  // Create a sphere source and representation
  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(16);
  sphere->SetPhiResolution(16);
  sphere->Update();

  vtkNew<vtkSurfaceRepresentation> rep;
  rep->SetInputConnection(sphere->GetOutputPort());
  view->AddRepresentation(rep);

  // --- Test interaction mode API ---
  CHECK(view->GetInteractionMode() == vtkStandardRenderView::INTERACTION_MODE_3D,
    "Default interaction mode should be 3D");

  view->SetInteractionModeToSelection();
  CHECK(view->GetInteractionMode() == vtkStandardRenderView::INTERACTION_MODE_SELECTION,
    "Interaction mode should be SELECTION after SetInteractionModeToSelection");

  view->SetInteractionModeTo3D();
  CHECK(view->GetInteractionMode() == vtkStandardRenderView::INTERACTION_MODE_3D,
    "Interaction mode should be 3D after SetInteractionModeTo3D");

  view->SetInteractionMode(vtkStandardRenderView::INTERACTION_MODE_SELECTION);
  CHECK(view->GetInteractionMode() == vtkStandardRenderView::INTERACTION_MODE_SELECTION,
    "SetInteractionMode with enum should work");

  // --- Test that style state survives a mode round trip ---
  view->SetInteractionModeTo3D();
  vtkInteractorObserver* trackball = view->GetInteractorStyle();
  CHECK(trackball, "The view should have an interactor style in 3D mode");
  vtkInteractorStyleTrackballCamera::SafeDownCast(trackball)->SetMotionFactor(42.0);
  view->SetInteractionModeToSelection();
  view->SetInteractionModeTo3D();
  CHECK(view->GetInteractorStyle() == trackball,
    "The view should reuse its trackball style rather than build a new one");
  CHECK(vtkInteractorStyleTrackballCamera::SafeDownCast(view->GetInteractorStyle())
          ->GetMotionFactor() == 42.0,
    "Style configuration should survive an interaction mode round trip");

  // --- Test custom interactor styles ---
  vtkNew<vtkInteractorStyleTerrain> terrain;
  view->SetInteractorStyle(terrain);
  CHECK(view->GetInteractionMode() == vtkStandardRenderView::INTERACTION_MODE_CUSTOM,
    "Setting a custom style should report INTERACTION_MODE_CUSTOM");
  CHECK(view->GetInteractorStyle() == terrain.Get(),
    "The custom style should be the one installed on the interactor");

  // A built-in mode takes over, and the custom style can be restored.
  view->SetInteractionModeTo3D();
  CHECK(view->GetInteractorStyle() == trackball, "A built-in mode should replace the custom style");
  view->SetInteractionMode(vtkStandardRenderView::INTERACTION_MODE_CUSTOM);
  CHECK(view->GetInteractorStyle() == terrain.Get(),
    "The remembered custom style should be restorable by mode");

  // Passing null returns to the default mode.
  view->SetInteractorStyle(nullptr);
  CHECK(view->GetInteractionMode() == vtkStandardRenderView::INTERACTION_MODE_3D,
    "A null custom style should return to 3D");

  view->SetInteractionMode(vtkStandardRenderView::INTERACTION_MODE_SELECTION);

  // --- Test selection mode API ---
  CHECK(view->GetSelectionMode() == vtkStandardRenderView::SELECTION_MODE_SURFACE,
    "Default selection mode should be SURFACE");

  view->SetSelectionModeToFrustum();
  CHECK(view->GetSelectionMode() == vtkStandardRenderView::SELECTION_MODE_FRUSTUM,
    "Selection mode should be FRUSTUM after SetSelectionModeToFrustum");

  view->SetSelectionModeToSurface();
  CHECK(view->GetSelectionMode() == vtkStandardRenderView::SELECTION_MODE_SURFACE,
    "Selection mode should be SURFACE after SetSelectionModeToSurface");

  // --- Test field association API ---
  CHECK(view->GetSelectionFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_CELLS,
    "Default field association should be CELLS");

  view->SelectPoints();
  CHECK(view->GetSelectionFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_POINTS,
    "Field association should be POINTS after SelectPoints");

  view->SelectCells();
  CHECK(view->GetSelectionFieldAssociation() == vtkDataObject::FIELD_ASSOCIATION_CELLS,
    "Field association should be CELLS after SelectCells");

  // Render once to initialize the pipeline
  view->Render();

  // --- Test programmatic index selection ---
  vtkNew<vtkSelection> selection;
  vtkNew<vtkSelectionNode> node;
  node->SetContentType(vtkSelectionNode::INDICES);
  node->SetFieldType(vtkSelectionNode::CELL);

  vtkNew<vtkIdTypeArray> ids;
  ids->InsertNextValue(0);
  ids->InsertNextValue(1);
  ids->InsertNextValue(2);
  node->SetSelectionList(ids);
  selection->AddNode(node);

  rep->Select(view, selection, false);
  CHECK(rep->GetSelectionActor()->GetVisibility(),
    "Selection actor should be visible after Select with indices");

  // --- Test clear selection ---
  view->ClearSelection();
  CHECK(!rep->GetSelectionActor()->GetVisibility(),
    "Selection actor should be invisible after ClearSelection");

  // --- Test region selection ---
  // SelectRegion drives the same path as a rubber-band drag, without needing
  // to synthesize interactor events.
  view->SelectRegion(0, 0, 400, 400);
  CHECK(view->GetCurrentSelection(), "SelectRegion should produce a selection");
  CHECK(rep->GetSelectionActor()->GetVisibility(),
    "Selection actor should be visible after selecting the whole viewport");

  // The corners may be given in any order.
  view->ClearSelection();
  view->SelectRegion(400, 400, 0, 0);
  CHECK(rep->GetSelectionActor()->GetVisibility(),
    "A region given from the far corner should select the same thing");

  // A click is degenerate; it must still be given enough area to pick with.
  view->ClearSelection();
  view->SelectRegion(200, 200, 200, 200);
  CHECK(view->GetCurrentSelection(), "A single-point region should still select");

  // Positions outside the window are reported as negative numbers by the
  // interactor.  They must be clamped, not reinterpreted as huge unsigned
  // coordinates.
  view->ClearSelection();
  view->SelectRegion(-50, -50, 200, 200);
  CHECK(view->GetCurrentSelection(), "A region starting off-window should select");
  view->ClearSelection();
  view->SelectRegion(-100, -100, -50, -50);
  CHECK(view->GetCurrentSelection(), "A fully off-window region should not crash");

  // Selecting past the far edge is clamped the same way.
  view->ClearSelection();
  view->SelectRegion(200, 200, 100000, 100000);
  CHECK(view->GetCurrentSelection(), "A region past the far edge should select");

  view->ClearSelection();

  // --- Test selection color/style API ---
  rep->SetSelectionColor(0.0, 1.0, 0.0);
  double* color = rep->GetSelectionActor()->GetProperty()->GetColor();
  CHECK(color[0] == 0.0 && color[1] == 1.0 && color[2] == 0.0,
    "Selection color should be green after SetSelectionColor");

  rep->SetSelectionOpacity(0.5);
  CHECK(rep->GetSelectionActor()->GetProperty()->GetOpacity() == 0.5,
    "Selection opacity should be 0.5");

  rep->SetSelectionLineWidth(4.0);
  CHECK(rep->GetSelectionActor()->GetProperty()->GetLineWidth() == 4.0,
    "Selection line width should be 4.0");

  rep->SetSelectionPointSize(8.0);
  CHECK(rep->GetSelectionActor()->GetProperty()->GetPointSize() == 8.0,
    "Selection point size should be 8.0");

  // Without an explicit style, the selection picks one from its field type.
  CHECK(rep->GetSelectionRepresentation() == vtkSurfaceRepresentation::WIREFRAME,
    "A cell selection should be drawn as wireframe");

  rep->SetSelectionRepresentation(vtkSurfaceRepresentation::SURFACE_WITH_EDGES);
  CHECK(rep->GetSelectionRepresentation() == vtkSurfaceRepresentation::SURFACE_WITH_EDGES,
    "GetSelectionRepresentation should report the value that was set");
  CHECK(rep->GetSelectionActor()->GetProperty()->GetRepresentation() == VTK_SURFACE &&
      rep->GetSelectionActor()->GetProperty()->GetEdgeVisibility(),
    "SURFACE_WITH_EDGES should be a surface with edges on");

  // An explicit style must survive a selection, which would otherwise reset it
  // to points or wireframe.
  rep->Select(view, selection, false);
  CHECK(rep->GetSelectionRepresentation() == vtkSurfaceRepresentation::SURFACE_WITH_EDGES,
    "An explicit selection representation should survive a selection");
  CHECK(rep->GetSelectionActor()->GetProperty()->GetRepresentation() == VTK_SURFACE,
    "The selection actor should still be a surface after a selection");

  rep->SetSelectionRepresentation(vtkSurfaceRepresentation::SURFACE);
  CHECK(rep->GetSelectionActor()->GetProperty()->GetRepresentation() == VTK_SURFACE,
    "Selection representation should be SURFACE");

  std::cout << "All selection tests passed.\n";
  return EXIT_SUCCESS;
}
