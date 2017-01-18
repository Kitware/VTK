/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHandleWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkAbstractWidget.h"

class vtkHandleRepresentation;


class VTKINTERACTIONWIDGETS_EXPORT vtkHandleWidget : public vtkAbstractWidget
{
public:
  /**
   * Instantiate this class.
   */
  static vtkHandleWidget *New();

  //@{
  /**
   * Standard VTK class macros.
   */
  vtkTypeMacro(vtkHandleWidget,vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkHandleRepresentation *r)
    {this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(r));}

  /**
   * Return the representation as a vtkHandleRepresentation.
   */
  vtkHandleRepresentation *GetHandleRepresentation()
    {return reinterpret_cast<vtkHandleRepresentation*>(this->WidgetRep);}

  /**
   * Create the default widget representation if one is not set. By default
   * an instance of vtkPointHandleRepresenation3D is created.
   */
  void CreateDefaultRepresentation() VTK_OVERRIDE;

  //@{
  /**
   * Enable / disable axis constrained motion of the handles. By default the
   * widget responds to the shift modifier to constrain the handle along the
   * axis closest aligned with the motion vector.
   */
  vtkSetMacro( EnableAxisConstraint, int );
  vtkGetMacro( EnableAxisConstraint, int );
  vtkBooleanMacro( EnableAxisConstraint, int );
  //@}

  //@{
  /**
   * Allow resizing of handles ? By default the right mouse button scales
   * the handle size.
   */
  vtkSetMacro( AllowHandleResize, int );
  vtkGetMacro( AllowHandleResize, int );
  vtkBooleanMacro( AllowHandleResize, int );
  //@}

  //@{
  /**
   * Get the widget state.
   */
  vtkGetMacro( WidgetState, int );
  //@}

  // Manage the state of the widget
  enum _WidgetState {Start=0,Active};

protected:
  vtkHandleWidget();
  ~vtkHandleWidget() VTK_OVERRIDE;

  // These are the callbacks for this widget
  static void GenericAction(vtkHandleWidget*);
  static void SelectAction(vtkAbstractWidget*);
  static void EndSelectAction(vtkAbstractWidget*);
  static void TranslateAction(vtkAbstractWidget*);
  static void ScaleAction(vtkAbstractWidget*);
  static void MoveAction(vtkAbstractWidget*);

  // helper methods for cursor management
  void SetCursor(int state) VTK_OVERRIDE;

  int WidgetState;
  int EnableAxisConstraint;

  // Allow resizing of handles.
  int AllowHandleResize;

private:
  vtkHandleWidget(const vtkHandleWidget&) VTK_DELETE_FUNCTION;
  void operator=(const vtkHandleWidget&) VTK_DELETE_FUNCTION;
};

#endif
