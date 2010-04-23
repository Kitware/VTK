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
// .NAME vtkHandleWidget - a general widget for moving handles
// .SECTION Description
// The vtkHandleWidget is used to position a handle.  A handle is a widget
// with a position (in display and world space). Various appearances are
// available depending on its associated representation. The widget provides
// methods for translation, including constrained translation along
// coordinate axes. To use this widget, create and associate a representation
// with the widget.
//
// .SECTION Event Bindings
// By default, the widget responds to the following VTK events (i.e., it
// watches the vtkRenderWindowInteractor for these events):
// <pre>
//   LeftButtonPressEvent - select focal point of widget
//   LeftButtonReleaseEvent - end selection
//   MiddleButtonPressEvent - translate widget
//   MiddleButtonReleaseEvent - end translation
//   RightButtonPressEvent - scale widget
//   RightButtonReleaseEvent - end scaling
//   MouseMoveEvent - interactive movement across widget
// </pre>
//
// Note that the event bindings described above can be changed using this
// class's vtkWidgetEventTranslator. This class translates VTK events 
// into the vtkHandleWidget's widget events:
// <pre>
//   vtkWidgetEvent::Select -- focal point is being selected
//   vtkWidgetEvent::EndSelect -- the selection process has completed
//   vtkWidgetEvent::Translate -- translate the widget
//   vtkWidgetEvent::EndTranslate -- end widget translation
//   vtkWidgetEvent::Scale -- scale the widget
//   vtkWidgetEvent::EndScale -- end scaling the widget
//   vtkWidgetEvent::Move -- a request for widget motion
// </pre>
//
// In turn, when these widget events are processed, the vtkHandleWidget
// invokes the following VTK events on itself (which observers can listen for):
// <pre>
//   vtkCommand::StartInteractionEvent (on vtkWidgetEvent::Select)
//   vtkCommand::EndInteractionEvent (on vtkWidgetEvent::EndSelect)
//   vtkCommand::InteractionEvent (on vtkWidgetEvent::Move)
// </pre>
//

#ifndef __vtkHandleWidget_h
#define __vtkHandleWidget_h

#include "vtkAbstractWidget.h"

class vtkHandleRepresentation;


class VTK_WIDGETS_EXPORT vtkHandleWidget : public vtkAbstractWidget
{
public:
  // Description:
  // Instantiate this class.
  static vtkHandleWidget *New();

  // Description:
  // Standard VTK class macros.
  vtkTypeMacro(vtkHandleWidget,vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify an instance of vtkWidgetRepresentation used to represent this
  // widget in the scene. Note that the representation is a subclass of vtkProp
  // so it can be added to the renderer independent of the widget.
  void SetRepresentation(vtkHandleRepresentation *r)
    {this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(r));}
  
  // Description:
  // Create the default widget representation if one is not set. By default
  // an instance of vtkPointHandleRepresenation3D is created.
  void CreateDefaultRepresentation();
  
  // Description:
  // Enable / disable axis constrained motion of the handles. By default the
  // widget responds to the shift modifier to constrain the handle along the
  // axis closest aligned with the motion vector.
  vtkSetMacro( EnableAxisConstraint, int );
  vtkGetMacro( EnableAxisConstraint, int );
  vtkBooleanMacro( EnableAxisConstraint, int );

  // Description:
  // Allow resizing of handles ? By default the right mouse button scales
  // the handle size.
  vtkSetMacro( AllowHandleResize, int );
  vtkGetMacro( AllowHandleResize, int );
  vtkBooleanMacro( AllowHandleResize, int );

  // Description:
  // Get the widget state.
  vtkGetMacro( WidgetState, int );

  //BTX - manage the state of the widget
  enum _WidgetState {Start=0,Active};
  //ETX
  
protected:
  vtkHandleWidget();
  ~vtkHandleWidget();

  // These are the callbacks for this widget
  static void GenericAction(vtkHandleWidget*);
  static void SelectAction(vtkAbstractWidget*);
  static void EndSelectAction(vtkAbstractWidget*);
  static void TranslateAction(vtkAbstractWidget*);
  static void ScaleAction(vtkAbstractWidget*);
  static void MoveAction(vtkAbstractWidget*);

  // helper methods for cursor management
  void SetCursor(int state);

  int WidgetState;
  int EnableAxisConstraint;

  // Allow resizing of handles.
  int AllowHandleResize;

private:
  vtkHandleWidget(const vtkHandleWidget&);  //Not implemented
  void operator=(const vtkHandleWidget&);  //Not implemented
};

#endif
