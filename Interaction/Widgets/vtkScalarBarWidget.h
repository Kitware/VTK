// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkScalarBarWidget
 * @brief   2D widget for manipulating a scalar bar
 *
 * This class provides support for interactively manipulating the position,
 * size, and orientation of a scalar bar. It listens to Left mouse events and
 * mouse movement. It also listens to Right mouse events and notifies any
 * observers of Right mouse events on this object when they occur.
 * It will change the cursor shape based on its location. If
 * the cursor is over an edge of the scalar bar it will change the cursor
 * shape to a resize edge shape. If the position of a scalar bar is moved to
 * be close to the center of one of the four edges of the viewport, then the
 * scalar bar will change its orientation to align with that edge. This
 * orientation is sticky in that it will stay that orientation until the
 * position is moved close to another edge.
 *
 * @sa
 * vtkInteractorObserver
 */

#ifndef vtkScalarBarWidget_h
#define vtkScalarBarWidget_h

#include "vtkBorderWidget.h"
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkScalarBarActor;
class vtkScalarBarRepresentation;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkScalarBarWidget : public vtkBorderWidget
{
public:
  static vtkScalarBarWidget* New();
  vtkTypeMacro(vtkScalarBarWidget, vtkBorderWidget);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independent of the widget.
   */
  virtual void SetRepresentation(vtkScalarBarRepresentation* rep);

  /**
   * Return the representation as a vtkScalarBarRepresentation.
   */
  vtkScalarBarRepresentation* GetScalarBarRepresentation()
  {
    return reinterpret_cast<vtkScalarBarRepresentation*>(this->GetRepresentation());
  }

  ///@{
  /**
   * Get the ScalarBar used by this Widget. One is created automatically.
   */
  virtual void SetScalarBarActor(vtkScalarBarActor* actor);
  virtual vtkScalarBarActor* GetScalarBarActor();
  ///@}

  ///@{
  /**
   * Can the widget be moved. On by default. If off, the widget cannot be moved
   * around.

   * TODO: This functionality should probably be moved to the superclass.
   */
  vtkSetMacro(Repositionable, vtkTypeBool);
  vtkGetMacro(Repositionable, vtkTypeBool);
  vtkBooleanMacro(Repositionable, vtkTypeBool);
  ///@}

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

  /**
   * Reimplement ProcessEvents to use vtkAbstractWidget instead of vtkBorderWidget,
   * for interaction with the scalar bar, even when the scalar bar's position is not AnyLocation.
   */
  vtkTypeBool GetProcessEvents() override;

protected:
  vtkScalarBarWidget();
  ~vtkScalarBarWidget() override;

  vtkTypeBool Repositionable;

  // Handle the case of Repositionable == 0
  static void MoveAction(vtkAbstractWidget*);

  // set the cursor to the correct shape based on State argument
  void SetCursor(int State) override;

private:
  vtkScalarBarWidget(const vtkScalarBarWidget&) = delete;
  void operator=(const vtkScalarBarWidget&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
