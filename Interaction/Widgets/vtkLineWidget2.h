// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLineWidget2
 * @brief   3D widget for manipulating a finite, straight line
 *
 * This 3D widget defines a straight line that can be interactively placed in
 * a scene. The widget is assumed to consist of two parts: 1) two end points
 * and 2) a straight line connecting the two points. (The representation
 * paired with this widget determines the actual geometry of the widget.) The
 * positioning of the two end points is facilitated by using vtkHandleWidgets
 * to position the points.
 *
 * To use this widget, you generally pair it with a vtkLineRepresentation
 * (or a subclass). Various options are available in the representation for
 * controlling how the widget appears, and how the widget functions.
 *
 * @par Event Bindings:
 * By default, the widget responds to the following VTK events (i.e., it
 * watches the vtkRenderWindowInteractor for these events):
 * <pre>
 * If one of the two end points are selected:
 *   LeftButtonPressEvent - activate the associated handle widget
 *   LeftButtonReleaseEvent - release the handle widget associated with the point
 *   MouseMoveEvent - move the point
 * If the line is selected:
 *   LeftButtonPressEvent - activate a handle widget associated with the line
 *   LeftButtonReleaseEvent - release the handle widget associated with the line
 *   MouseMoveEvent - translate the line
 * In all the cases, independent of what is picked, the widget responds to the
 * following VTK events:
 *   MiddleButtonPressEvent - translate the widget
 *   MiddleButtonReleaseEvent - release the widget
 *   RightButtonPressEvent - scale the widget's representation
 *   RightButtonReleaseEvent - stop scaling the widget
 *   MouseMoveEvent - scale (if right button) or move (if middle button) the widget
 * </pre>
 *
 * @par Event Bindings:
 * Note that the event bindings described above can be changed using this
 * class's vtkWidgetEventTranslator. This class translates VTK events
 * into the vtkLineWidget2's widget events:
 * <pre>
 *   vtkWidgetEvent::Select -- some part of the widget has been selected
 *   vtkWidgetEvent::EndSelect -- the selection process has completed
 *   vtkWidgetEvent::Move -- a request for slider motion has been invoked
 * </pre>
 *
 * @par Event Bindings:
 * In turn, when these widget events are processed, the vtkLineWidget2
 * invokes the following VTK events on itself (which observers can listen for):
 * <pre>
 *   vtkCommand::StartInteractionEvent (on vtkWidgetEvent::Select)
 *   vtkCommand::EndInteractionEvent (on vtkWidgetEvent::EndSelect)
 *   vtkCommand::InteractionEvent (on vtkWidgetEvent::Move)
 * </pre>
 *
 *
 *
 * @par Event Bindings:
 * This class, and vtkLineRepresentation, are next generation VTK widgets. An
 * earlier version of this functionality was defined in the class
 * vtkLineWidget.
 *
 * @sa
 * vtkLineRepresentation vtkLineWidget vtk3DWidget vtkImplicitPlaneWidget
 * vtkImplicitPlaneWidget2
 */

#ifndef vtkLineWidget2_h
#define vtkLineWidget2_h

#include "vtkAbstractWidget.h"
#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkLineRepresentation;
class vtkHandleWidget;

class VTKINTERACTIONWIDGETS_EXPORT VTK_MARSHALAUTO vtkLineWidget2 : public vtkAbstractWidget
{
public:
  /**
   * Instantiate the object.
   */
  static vtkLineWidget2* New();

  ///@{
  /**
   * Standard vtkObject methods
   */
  vtkTypeMacro(vtkLineWidget2, vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Override superclasses' SetEnabled() method because the line
   * widget must enable its internal handle widgets.
   */
  void SetEnabled(int enabling) override;

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkLineRepresentation* r)
  {
    this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(r));
  }

  /**
   * Return the representation as a vtkLineRepresentation.
   */
  vtkLineRepresentation* GetLineRepresentation()
  {
    return reinterpret_cast<vtkLineRepresentation*>(this->WidgetRep);
  }

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() override;

  /**
   * Methods to change the whether the widget responds to interaction.
   * Overridden to pass the state to component widgets.
   */
  void SetProcessEvents(vtkTypeBool) override;

protected:
  vtkLineWidget2();
  ~vtkLineWidget2() override;

  // Manage the state of the widget
  int WidgetState;
  enum WidgetStateType
  {
    Start = 0,
    Active
  };

  int CurrentHandle;

  // These methods handle events
  static void SelectAction(vtkAbstractWidget*);
  static void TranslateAction(vtkAbstractWidget*);
  static void ScaleAction(vtkAbstractWidget*);
  static void EndSelectAction(vtkAbstractWidget*);
  static void MoveAction(vtkAbstractWidget*);

  // The positioning handle widgets
  vtkHandleWidget* Point1Widget; // first end point
  vtkHandleWidget* Point2Widget; // second end point
  vtkHandleWidget* LineHandle;   // used when selecting the line

  vtkCallbackCommand* KeyEventCallbackCommand;
  static void ProcessKeyEvents(vtkObject*, unsigned long, void*, void*);

private:
  vtkLineWidget2(const vtkLineWidget2&) = delete;
  void operator=(const vtkLineWidget2&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
