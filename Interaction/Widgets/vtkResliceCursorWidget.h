// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkResliceCursorWidget
 * @brief   represent a reslice cursor
 *
 * This class represents a reslice cursor that can be used to
 * perform interactive thick slab MPR's through data. It
 * consists of two cross sectional hairs, with an optional thickness. The
 * hairs may have a hole in the center. These may be translated or rotated
 * independent of each other in the view. The result is used to reslice
 * the data along these cross sections. This allows the user to perform
 * multi-planar thin or thick reformat of the data on an image view, rather
 * than a 3D view. The class internally uses vtkImageSlabReslice
 * or vtkImageReslice depending on the modes in vtkResliceCursor to
 * do its reslicing. The slab thickness is set interactively from
 * the widget. The slab resolution (ie the number of blend points) is
 * set as the minimum spacing along any dimension from the dataset.
 * @sa
 * vtkImageSlabReslice vtkResliceCursorLineRepresentation
 * vtkResliceCursor
 */

#ifndef vtkResliceCursorWidget_h
#define vtkResliceCursorWidget_h

#include "vtkAbstractWidget.h"
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkResliceCursorRepresentation;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkResliceCursorWidget : public vtkAbstractWidget
{
public:
  /**
   * Instantiate this class.
   */
  static vtkResliceCursorWidget* New();

  ///@{
  /**
   * Standard VTK class macros.
   */
  vtkTypeMacro(vtkResliceCursorWidget, vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkResliceCursorRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(r));
  }

  /**
   * Return the representation as a vtkResliceCursorRepresentation.
   */
  vtkResliceCursorRepresentation* GetResliceCursorRepresentation()
  {
    return reinterpret_cast<vtkResliceCursorRepresentation*>(this->WidgetRep);
  }

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

  /**
   * Methods for activating this widget. This implementation extends the
   * superclasses' in order to resize the widget handles due to a render
   * start event.
   */
  void SetEnabled(int) override;

  ///@{
  /**
   * Also perform window level ?
   */
  vtkSetMacro(ManageWindowLevel, vtkTypeBool);
  vtkGetMacro(ManageWindowLevel, vtkTypeBool);
  vtkBooleanMacro(ManageWindowLevel, vtkTypeBool);
  ///@}

  /**
   * Events
   */
  enum
  {
    WindowLevelEvent = 1055,
    ResliceAxesChangedEvent,
    ResliceThicknessChangedEvent,
    ResetCursorEvent
  };

  /**
   * Reset the cursor back to its initial state
   */
  virtual void ResetResliceCursor();

protected:
  vtkResliceCursorWidget();
  ~vtkResliceCursorWidget() override;

  // These are the callbacks for this widget
  static void SelectAction(vtkAbstractWidget*);
  static void RotateAction(vtkAbstractWidget*);
  static void TranslateAction(vtkAbstractWidget*);
  static void EndSelectAction(vtkAbstractWidget*);
  static void ResizeThicknessAction(vtkAbstractWidget*);
  static void EndResizeThicknessAction(vtkAbstractWidget*);
  static void MoveAction(vtkAbstractWidget*);
  static void ResetResliceCursorAction(vtkAbstractWidget*);

  // helper methods for cursor management
  void SetCursor(int state) override;

  // Start Window Level
  void StartWindowLevel();

  // Invoke the appropriate event based on state
  void InvokeAnEvent();

  // Manage the state of the widget
  int WidgetState;
  enum WidgetStateType
  {
    Start = 0,
    Active
  };

  // Keep track whether key modifier key is pressed
  int ModifierActive;
  vtkTypeBool ManageWindowLevel;

private:
  vtkResliceCursorWidget(const vtkResliceCursorWidget&) = delete;
  void operator=(const vtkResliceCursorWidget&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
