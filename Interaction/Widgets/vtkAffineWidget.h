/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAffineWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkAffineWidget
 * @brief   perform affine transformations
 *
 * The vtkAffineWidget is used to perform affine transformations on objects.
 * (Affine transformations are transformations that keep parallel lines parallel.
 * Affine transformations include translation, scaling, rotation, and shearing.)
 *
 * To use this widget, set the widget representation. The representation
 * maintains a transformation matrix and other instance variables consistent
 * with the transformations applied by this widget.
 *
 * @par Event Bindings:
 * By default, the widget responds to the following VTK events (i.e., it
 * watches the vtkRenderWindowInteractor for these events):
 * <pre>
 *   LeftButtonPressEvent - select widget: depending on which part is selected
 *                          translation, rotation, scaling, or shearing may follow.
 *   LeftButtonReleaseEvent - end selection of widget.
 *   MouseMoveEvent - interactive movement across widget
 * </pre>
 *
 * @par Event Bindings:
 * Note that the event bindings described above can be changed using this
 * class's vtkWidgetEventTranslator. This class translates VTK events
 * into the vtkAffineWidget's widget events:
 * <pre>
 *   vtkWidgetEvent::Select -- focal point is being selected
 *   vtkWidgetEvent::EndSelect -- the selection process has completed
 *   vtkWidgetEvent::Move -- a request for widget motion
 * </pre>
 *
 * @par Event Bindings:
 * In turn, when these widget events are processed, the vtkAffineWidget
 * invokes the following VTK events on itself (which observers can listen for):
 * <pre>
 *   vtkCommand::StartInteractionEvent (on vtkWidgetEvent::Select)
 *   vtkCommand::EndInteractionEvent (on vtkWidgetEvent::EndSelect)
 *   vtkCommand::InteractionEvent (on vtkWidgetEvent::Move)
 * </pre>
 *
*/

#ifndef vtkAffineWidget_h
#define vtkAffineWidget_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkAbstractWidget.h"

class vtkAffineRepresentation;


class VTKINTERACTIONWIDGETS_EXPORT vtkAffineWidget : public vtkAbstractWidget
{
public:
  /**
   * Instantiate this class.
   */
  static vtkAffineWidget *New();

  //@{
  /**
   * Standard VTK class macros.
   */
  vtkTypeMacro(vtkAffineWidget,vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  /**
   * Specify an instance of vtkWidgetRepresentation used to represent this
   * widget in the scene. Note that the representation is a subclass of vtkProp
   * so it can be added to the renderer independent of the widget.
   */
  void SetRepresentation(vtkAffineRepresentation *r)
    {this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(r));}

  /**
   * Return the representation as a vtkAffineRepresentation.
   */
  vtkAffineRepresentation *GetAffineRepresentation()
    {return reinterpret_cast<vtkAffineRepresentation*>(this->WidgetRep);}

  /**
   * Create the default widget representation if one is not set.
   */
  void CreateDefaultRepresentation() VTK_OVERRIDE;

  /**
   * Methods for activating this widget. This implementation extends the
   * superclasses' in order to resize the widget handles due to a render
   * start event.
   */
  void SetEnabled(int) VTK_OVERRIDE;

protected:
  vtkAffineWidget();
  ~vtkAffineWidget() VTK_OVERRIDE;

  // These are the callbacks for this widget
  static void SelectAction(vtkAbstractWidget*);
  static void EndSelectAction(vtkAbstractWidget*);
  static void MoveAction(vtkAbstractWidget*);
  static void ModifyEventAction(vtkAbstractWidget*);

  // helper methods for cursor management
  void SetCursor(int state) VTK_OVERRIDE;

  // Manage the state of the widget
  int WidgetState;
  enum _WidgetState
  {
    Start=0,
    Active
  };

  // Keep track whether key modifier key is pressed
  int ModifierActive;

private:
  vtkAffineWidget(const vtkAffineWidget&) VTK_DELETE_FUNCTION;
  void operator=(const vtkAffineWidget&) VTK_DELETE_FUNCTION;
};

#endif
