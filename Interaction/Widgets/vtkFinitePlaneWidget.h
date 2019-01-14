/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFinitePlaneWidget.h

  Copyright (c)
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkFinitePlaneWidget
 * @brief   3D widget for manipulating a finite plane
 *
 * This 3D widget interacts with a vtkFinitePlaneRepresentation class (i.e., it
 * handles the events that drive its corresponding representation). This 3D
 * widget defines a finite plane that can be interactively placed in a scene.
 * The widget is assumed to consist of four parts: 1) a plane with 2) a normal
 * and 3) three handles that can be moused on and manipulated.
 * The green and red handles represent the semi finite plane definition,
 * the third is in the center of the plane.
 * Operation like rotation of the plane (using normal), origin translation and
 * geometry plane modification using green and red handles are availables.
 *
 * To use this widget, you generally pair it with a vtkFinitePlaneRepresentation
 * (or a subclass). Various options are available in the representation for
 * controlling how the widget appears, and how the widget reacts.
 *
 * @par Event Bindings:
 * By default, the widget responds to the following VTK events (i.e., it
 * watches the vtkRenderWindowInteractor for these events):
 * <pre>
 * If one of the 3 handles are selected:
 *   LeftButtonPressEvent - select the appropriate handle
 *   LeftButtonReleaseEvent - release the currently selected handle
 *   MouseMoveEvent - move the handle
 * In all the cases, independent of what is picked, the widget responds to the
 * following VTK events:
 *   LeftButtonPressEvent - start select action
 *   LeftButtonReleaseEvent - stop select action
 * </pre>
 *
 * @par Event Bindings:
 * Note that the event bindings described above can be changed using this
 * class's vtkWidgetEventTranslator. This class translates VTK events
 * into the vtkFinitePlaneWidget's widget events:
 * <pre>
 *   vtkWidgetEvent::Select -- some part of the widget has been selected
 *   vtkWidgetEvent::EndSelect -- the selection process has completed
 *   vtkWidgetEvent::Move -- a request for motion has been invoked
 * </pre>
 *
 * @par Event Bindings:
 * In turn, when these widget events are processed, the vtkFinitePlaneWidget
 * invokes the following VTK events on itself (which observers can listen for):
 * <pre>
 *   vtkCommand::StartInteractionEvent (on vtkWidgetEvent::Select)
 *   vtkCommand::EndInteractionEvent (on vtkWidgetEvent::EndSelect)
 *   vtkCommand::InteractionEvent (on vtkWidgetEvent::Move)
 * </pre>
 * @sa
 * vtkFinitePlaneRepresentation
*/

#ifndef vtkFinitePlaneWidget_h
#define vtkFinitePlaneWidget_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkAbstractWidget.h"

class vtkFinitePlaneRepresentation;
class vtkHandleWidget;

class VTKINTERACTIONWIDGETS_EXPORT vtkFinitePlaneWidget : public vtkAbstractWidget
{
public:
  /**
   * Instantiate the object.
   */
  static vtkFinitePlaneWidget *New();

  //@{
  /**
   * Standard vtkObject methods
   */
  vtkTypeMacro(vtkFinitePlaneWidget,vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkFinitePlaneRepresentation *r);

  /**
   * Create the default widget representation if one is not set. By default,
   * this is an instance of the vtkFinitePlaneRepresentation class.
   */
  void CreateDefaultRepresentation() override;

protected:
  vtkFinitePlaneWidget();
  ~vtkFinitePlaneWidget() override;

  int WidgetState;
  enum _WidgetState {Start = 0, Active};

  // These methods handle events
  static void SelectAction(vtkAbstractWidget*);
  static void EndSelectAction(vtkAbstractWidget*);
  static void MoveAction(vtkAbstractWidget*);

  /**
   * Update the cursor shape based on the interaction state. Returns 1
   * if the cursor shape requested is different from the existing one.
   */
  int UpdateCursorShape( int interactionState );

private:
  vtkFinitePlaneWidget(const vtkFinitePlaneWidget&) = delete;
  void operator=(const vtkFinitePlaneWidget&) = delete;
};

#endif
