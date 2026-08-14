// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkScivisSelector
 * @brief   Selection for a vtkScivisView.
 *
 * vtkScivisSelector is the part of a render view that turns a region of the
 * screen into a selection: how the region is interpreted, whether points or
 * cells come back, and what was picked last.  A view owns one and hands it out
 * through vtkScivisView::GetSelector(), which keeps selection in one
 * place instead of spread across the view's own API.
 *
 * @par Example usage:
 * @code
 * view->GetSelector()->SetModeToFrustum();
 * view->GetSelector()->SelectCells();
 * view->GetSelector()->SelectRegion(10, 10, 200, 200);
 * vtkSelection* picked = view->GetSelector()->GetCurrentSelection();
 * @endcode
 *
 * @par
 * The same in Python:
 * @code
 * view.selector.mode = "frustum"
 * view.selector.SelectCells()
 * view.selector.SelectRegion(10, 10, 200, 200)
 * picked = view.selector.current_selection
 * @endcode
 *
 * Interaction is not here: which interactor style is installed, and therefore
 * whether a drag draws a rubber band at all, belongs to the view.  This object
 * is what a rubber band drives, and what application code drives directly when
 * it wants a selection without one.
 *
 * @sa vtkScivisView vtkSelection vtkHardwareSelector
 */

#ifndef vtkScivisSelector_h
#define vtkScivisSelector_h

#include "vtkNew.h" // For ivars
#include "vtkObject.h"
#include "vtkSmartPointer.h"      // For ivars
#include "vtkViewsScivisModule.h" // For export macro
#include "vtkWeakPointer.h"       // For ivars

VTK_ABI_NAMESPACE_BEGIN
class vtkHardwareSelector;
class vtkSelection;
class vtkScivisView;

class VTKVIEWSSCIVIS_EXPORT vtkScivisSelector : public vtkObject
{
public:
  static vtkScivisSelector* New();
  vtkTypeMacro(vtkScivisSelector, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * How a screen region becomes a selection.
   *
   * SURFACE picks what is visible, through vtkHardwareSelector, so nothing
   * behind anything else is selected.  FRUSTUM selects everything inside the
   * viewing frustum the region describes, whether it can be seen or not.
   * Default is SURFACE.
   */
  enum SelectionModeType
  {
    SURFACE = 0,
    FRUSTUM = 1
  };
  void SetMode(int mode);
  int GetMode();
  void SetModeToSurface() { this->SetMode(SURFACE); }
  void SetModeToFrustum() { this->SetMode(FRUSTUM); }
  ///@}

  ///@{
  /**
   * Whether cells or points are selected: one of
   * vtkDataObject::FIELD_ASSOCIATION_CELLS, the default, or
   * vtkDataObject::FIELD_ASSOCIATION_POINTS.  SelectCells() and SelectPoints()
   * are the same thing said more plainly.
   */
  void SetFieldAssociation(int assoc);
  int GetFieldAssociation();
  void SelectCells();
  void SelectPoints();
  ///@}

  /**
   * Select the screen-space region bounded by the two corner points, exactly as
   * an interactive rubber-band drag would.  The corners are given in display
   * coordinates and may be in any order; the region is normalized and clamped
   * to the renderer, and a degenerate region -- a click -- is expanded slightly
   * so that it still picks.
   *
   * The selection honors the current mode and field association, is applied to
   * every representation in the view, becomes the current selection, and fires
   * SelectionChangedEvent.  When @a extend is true it adds to the existing
   * selection rather than replacing it.
   *
   * This is what the view's rubber band drives, and the supported way for
   * application code or an interactor style of your own to select without
   * synthesizing interactor events.
   */
  void SelectRegion(int x0, int y0, int x1, int y1, bool extend = false);

  /**
   * Drop the current selection, on every representation in the view.
   */
  void Clear();

  /**
   * What the last selection picked, or null if nothing has been selected.
   *
   * SelectionChangedEvent is fired when this changes, with the vtkSelection as
   * call data.  It is fired both here and on the view, so an observer can watch
   * whichever of the two it already has.
   */
  vtkSelection* GetCurrentSelection();

  ///@{
  /**
   * The view this selects in.  Set by the view that owns this object; there is
   * no reason for anything else to call it.
   *
   * Held weakly, so a selector kept alive after its view has gone reports no
   * selection rather than following a dangling pointer.
   */
  void SetView(vtkScivisView* view);
  vtkScivisView* GetView();
  ///@}

protected:
  vtkScivisSelector();
  ~vtkScivisSelector() override;

private:
  vtkScivisSelector(const vtkScivisSelector&) = delete;
  void operator=(const vtkScivisSelector&) = delete;

  /**
   * Build a selection for the given display-space region, which must already be
   * normalized and clamped to the renderer.
   */
  void GenerateSelection(const int region[4], vtkSelection* selection);

  vtkWeakPointer<vtkScivisView> View;
  vtkNew<vtkHardwareSelector> HardwareSelector;
  vtkSmartPointer<vtkSelection> CurrentSelection;
  int Mode;
  int FieldAssociation;
};

VTK_ABI_NAMESPACE_END
#endif
