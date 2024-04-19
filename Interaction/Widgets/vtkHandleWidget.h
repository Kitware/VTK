// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHandleWidget
 * @brief   a general widget for moving handles
 *
 * The vtkHandleWidget is used to position a handle.  A handle is a widget
 * with a position (in display and world space). Various appearances are
 * available depending on its associated representation. The widget provides
 * methods for translation, including constrained translation along
 * coordinate axes. To use this widget, create and associate a representation
 * with the widget.
 *
 * @par Event Bindings:
 * By default, the widget responds to the following VTK events (i.e., it
 * watches the vtkRenderWindowInteractor for these events):
 * <pre>
 *   LeftButtonPressEvent - select focal point of widget
 *   LeftButtonReleaseEvent - end selection
 *   MiddleButtonPressEvent - translate widget
 *   MiddleButtonReleaseEvent - end translation
 *   RightButtonPressEvent - scale widget
 *   RightButtonReleaseEvent - end scaling
 *   MouseMoveEvent - interactive movement across widget
 * </pre>
 *
 * @par Event Bindings:
 * Note that the event bindings described above can be changed using this
 * class's vtkWidgetEventTranslator. This class translates VTK events
 * into the vtkHandleWidget's widget events:
 * <pre>
 *   vtkWidgetEvent::Select -- focal point is being selected
 *   vtkWidgetEvent::EndSelect -- the selection process has completed
 *   vtkWidgetEvent::Translate -- translate the widget
 *   vtkWidgetEvent::EndTranslate -- end widget translation
 *   vtkWidgetEvent::Scale -- scale the widget
 *   vtkWidgetEvent::EndScale -- end scaling the widget
 *   vtkWidgetEvent::Move -- a request for widget motion
 * </pre>
 *
 * @par Event Bindings:
 * In turn, when these widget events are processed, the vtkHandleWidget
 * invokes the following VTK events on itself (which observers can listen for):
 * <pre>
 *   vtkCommand::StartInteractionEvent (on vtkWidgetEvent::Select)
 *   vtkCommand::EndInteractionEvent (on vtkWidgetEvent::EndSelect)
 *   vtkCommand::InteractionEvent (on vtkWidgetEvent::Move)
 * </pre>
 *
 */

#ifndef vtkHandleWidget_h
#define vtkHandleWidget_h

#include "vtkAbstractWidget.h"
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkHandleRepresentation;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkHandleWidget : public vtkAbstractWidget
{
public:
  /**
   * Instantiate this class.
   */
  static vtkHandleWidget* New();

  ///@{
  /**
   * Standard VTK class macros.
   */
  vtkTypeMacro(vtkHandleWidget, vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkHandleRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(r));
  }

  /**
   * Return the representation as a vtkHandleRepresentation.
   */
  vtkHandleRepresentation* GetHandleRepresentation()
  {
    return reinterpret_cast<vtkHandleRepresentation*>(this->WidgetRep);
  }

  /**
   * Create the default widget representation if one is not set. By default
   * an instance of vtkPointHandleRepresentation3D is created.
   */
  void CreateDefaultRepresentation() override;

  ///@{
  /**
   * Enable / disable axis constrained motion of the handles. By default the
   * widget responds to the shift modifier to constrain the handle along the
   * axis closest aligned with the motion vector.
   */
  vtkSetMacro(EnableAxisConstraint, vtkTypeBool);
  vtkGetMacro(EnableAxisConstraint, vtkTypeBool);
  vtkBooleanMacro(EnableAxisConstraint, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Enable moving of handles. By default, the handle can be moved.
   */
  vtkSetMacro(EnableTranslation, vtkTypeBool);
  vtkGetMacro(EnableTranslation, vtkTypeBool);
  vtkBooleanMacro(EnableTranslation, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Allow resizing of handles ? By default the right mouse button scales
   * the handle size.
   */
  vtkSetMacro(AllowHandleResize, vtkTypeBool);
  vtkGetMacro(AllowHandleResize, vtkTypeBool);
  vtkBooleanMacro(AllowHandleResize, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Get the widget state.
   */
  vtkGetMacro(WidgetState, int);
  ///@}

  ///@{
  /**
   * Allow the widget to be visible as an inactive representation when disabled.
   * By default, this is false i.e. the representation is not visible when the
   * widget is disabled.
   */
  vtkSetMacro(ShowInactive, vtkTypeBool);
  vtkGetMacro(ShowInactive, vtkTypeBool);
  vtkBooleanMacro(ShowInactive, vtkTypeBool);
  ///@}

  // Manage the state of the widget
  enum WidgetStateType
  {
    Start = 0,
    Active,
    Inactive
  };

  /**
   * Enable/disable widget.
   * Custom override for the SetEnabled method to allow for the inactive state.
   **/
  void SetEnabled(int enabling) override;

protected:
  vtkHandleWidget();
  ~vtkHandleWidget() override;

  // These are the callbacks for this widget
  static void GenericAction(vtkHandleWidget*);
  static void SelectAction(vtkAbstractWidget*);
  static void EndSelectAction(vtkAbstractWidget*);
  static void TranslateAction(vtkAbstractWidget*);
  static void ScaleAction(vtkAbstractWidget*);
  static void MoveAction(vtkAbstractWidget*);
  static void SelectAction3D(vtkAbstractWidget*);
  static void MoveAction3D(vtkAbstractWidget*);
  static void ProcessKeyEvents(vtkObject*, unsigned long, void*, void*);

  // helper methods for cursor management
  void SetCursor(int state) override;

  int WidgetState;
  vtkTypeBool EnableAxisConstraint;
  vtkTypeBool EnableTranslation;

  // Allow resizing of handles.
  vtkTypeBool AllowHandleResize;

  // Keep representation visible when disabled
  vtkTypeBool ShowInactive;

  vtkCallbackCommand* KeyEventCallbackCommand;

private:
  vtkHandleWidget(const vtkHandleWidget&) = delete;
  void operator=(const vtkHandleWidget&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
