// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class vtkImplicitFrustumWidget
 * @brief 3D widget for manipulating an infinite frustum
 *
 * This 3D widget defines an infinite frustum that can be
 * interactively placed in a scene.
 *
 * To use this widget, you generally pair it with a vtkImplicitFrustumRepresentation
 * (or a subclass). Various options are available for controlling how the
 * representation appears, and how the widget functions.
 *
 * @par Event Bindings:
 * By default, the widget responds to the following VTK events (i.e., it
 * watches the vtkRenderWindowInteractor for these events):
 *
 * - LeftButtonPressEvent - select a widget handle
 * - LeftButtonReleaseEvent - release the currently held widget handle
 * - MouseMoveEvent - dependent on the current manipulation mode:
 *  - Origin handle: Translate the frustum origin (constrained to the x, y or z axis one of the
 * corresponding key is held)
 *  - Near plane edges handle: Adjust the near plane distance
 *  - Far plane edges handle: Adjust the horizontal/vertical frustum angles
 *
 * In all the cases, independent of what is picked, the widget responds to the
 * following VTK events:
 *
 * - MiddleButtonPressEvent - grab the frustum
 * - MiddleButtonReleaseEvent - release the frustum
 * - MouseMoveEvent - move the widget (if middle button is pressed)
 *
 * @sa
 * vtk3DWidget vtkImplicitFrustumRepresentation vtkFrustum
 */

#ifndef vtkImplicitFrustumWidget_h
#define vtkImplicitFrustumWidget_h

#include "vtkAbstractWidget.h"
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkImplicitFrustumRepresentation;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkImplicitFrustumWidget
  : public vtkAbstractWidget
{
public:
  static vtkImplicitFrustumWidget* New();
  vtkTypeMacro(vtkImplicitFrustumWidget, vtkAbstractWidget);

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkImplicitFrustumRepresentation* rep);

  /**
   * Return the representation as a vtkImplicitFrustumRepresentation.
   */
  vtkImplicitFrustumRepresentation* GetFrustumRepresentation()
  {
    return reinterpret_cast<vtkImplicitFrustumRepresentation*>(this->WidgetRep);
  }

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

private:
  enum class WidgetStateType
  {
    Idle,
    Active
  };

  vtkImplicitFrustumWidget();
  ~vtkImplicitFrustumWidget() override = default;

  vtkImplicitFrustumWidget(const vtkImplicitFrustumWidget&) = delete;
  void operator=(const vtkImplicitFrustumWidget&) = delete;

  ///@{
  /**
   * Callbacks for widget events
   */
  static void SelectAction(vtkAbstractWidget* widget);
  static void EndSelectAction(vtkAbstractWidget* widget);
  static void TranslateAction(vtkAbstractWidget* widget);
  static void MoveAction(vtkAbstractWidget* widget);
  static void TranslationAxisLock(vtkAbstractWidget* widget);
  static void TranslationAxisUnLock(vtkAbstractWidget* widget);
  /// @}

  /**
   * Update the cursor shape based on the interaction state. Returns true
   * if the cursor shape requested is different from the existing one.
   */
  bool UpdateCursorShape(int interactionState);

  WidgetStateType State = WidgetStateType::Idle;
};

VTK_ABI_NAMESPACE_END
#endif
