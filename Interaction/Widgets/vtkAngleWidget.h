/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAngleWidget.h,v

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAngleWidget - measure the angle between two rays (defined by three points)
// .SECTION Description
// The vtkAngleWidget is used to measure the angle between two rays (defined
// by three points). The three points (two end points and a center)
// can be positioned independently, and when they are released, a special
// PlacePointEvent is invoked so that special operations may be take to
// reposition the point (snap to grid, etc.) The widget has two different
// modes of interaction: when initially defined (i.e., placing the three
// points) and then a manipulate mode (adjusting the position of the
// three points).
//
// To use this widget, specify an instance of vtkAngleWidget and a
// representation (a subclass of vtkAngleRepresentation). The widget is
// implemented using three instances of vtkHandleWidget which are used to
// position the three points. The representations for these handle widgets
// are provided by the vtkAngleRepresentation.
//
// .SECTION Event Bindings
// By default, the widget responds to the following VTK events (i.e., it
// watches the vtkRenderWindowInteractor for these events):
// <pre>
//   LeftButtonPressEvent - add a point or select a handle
//   MouseMoveEvent - position the second or third point, or move a handle
//   LeftButtonReleaseEvent - release the selected handle
// </pre>
//
// Note that the event bindings described above can be changed using this
// class's vtkWidgetEventTranslator. This class translates VTK events
// into the vtkAngleWidget's widget events:
// <pre>
//   vtkWidgetEvent::AddPoint -- add one point; depending on the state
//                               it may the first, second or third point
//                               added. Or, if near a handle, select the handle.
//   vtkWidgetEvent::Move -- position the second or third point, or move the
//                           handle depending on the state.
//   vtkWidgetEvent::EndSelect -- the handle manipulation process has completed.
// </pre>
//
// This widget invokes the following VTK events on itself (which observers
// can listen for):
// <pre>
//   vtkCommand::StartInteractionEvent (beginning to interact)
//   vtkCommand::EndInteractionEvent (completing interaction)
//   vtkCommand::InteractionEvent (moving a handle)
//   vtkCommand::PlacePointEvent (after a point is positioned;
//                                call data includes handle id (0,1,2))
// </pre>

// .SECTION See Also
// vtkHandleWidget vtkDistanceWidget


#ifndef __vtkAngleWidget_h
#define __vtkAngleWidget_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkAbstractWidget.h"

class vtkAngleRepresentation;
class vtkHandleWidget;
class vtkAngleWidgetCallback;


class VTKINTERACTIONWIDGETS_EXPORT vtkAngleWidget : public vtkAbstractWidget
{
public:
  // Description:
  // Instantiate this class.
  static vtkAngleWidget *New();

  // Description:
  // Standard methods for a VTK class.
  vtkTypeMacro(vtkAngleWidget,vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The method for activating and deactivating this widget. This method
  // must be overridden because it is a composite widget and does more than
  // its superclasses' vtkAbstractWidget::SetEnabled() method.
  virtual void SetEnabled(int);

  // Description:
  // Specify an instance of vtkWidgetRepresentation used to represent this
  // widget in the scene. Note that the representation is a subclass of vtkProp
  // so it can be added to the renderer independent of the widget.
  void SetRepresentation(vtkAngleRepresentation *r)
    {this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(r));}

  // Description:
  // Create the default widget representation if one is not set.
  void CreateDefaultRepresentation();

  // Description:
  // Return the representation as a vtkAngleRepresentation.
  vtkAngleRepresentation *GetAngleRepresentation()
    {return reinterpret_cast<vtkAngleRepresentation*>(this->WidgetRep);}

  // Description:
  // A flag indicates whether the angle is valid. The angle value only becomes
  // valid after two of the three points are placed.
  int IsAngleValid();

  // Description:
  // Methods to change the whether the widget responds to interaction.
  // Overridden to pass the state to component widgets.
  virtual void SetProcessEvents(int);

  // Description:
  // Enum defining the state of the widget. By default the widget is in Start mode,
  // and expects to be interactively placed. While placing the points the widget
  // transitions to Define state. Once placed, the widget enters the Manipulate state.
  //BTX
  enum {Start=0,Define,Manipulate};
  //ETX

  // Description:
  // Set the state of the widget. If the state is set to "Manipulate" then it
  // is assumed that the widget and its representation will be initialized
  // programmatically and is not interactively placed. Initially the widget
  // state is set to "Start" which means nothing will appear and the user
  // must interactively place the widget with repeated mouse selections. Set
  // the state to "Start" if you want interactive placement. Generally state
  // changes must be followed by a Render() for things to visually take
  // effect.
  virtual void SetWidgetStateToStart();
  virtual void SetWidgetStateToManipulate();

  // Description:
  // Return the current widget state.
  virtual int GetWidgetState()
    {return this->WidgetState;}

protected:
  vtkAngleWidget();
  ~vtkAngleWidget();

  // The state of the widget
  int WidgetState;
  int CurrentHandle;

  // Callback interface to capture events when
  // placing the widget.
  static void AddPointAction(vtkAbstractWidget*);
  static void MoveAction(vtkAbstractWidget*);
  static void EndSelectAction(vtkAbstractWidget*);

  // The positioning handle widgets
  vtkHandleWidget *Point1Widget;
  vtkHandleWidget *CenterWidget;
  vtkHandleWidget *Point2Widget;
  vtkAngleWidgetCallback *AngleWidgetCallback1;
  vtkAngleWidgetCallback *AngleWidgetCenterCallback;
  vtkAngleWidgetCallback *AngleWidgetCallback2;

  // Methods invoked when the handles at the
  // end points of the widget are manipulated
  void StartAngleInteraction(int handleNum);
  void AngleInteraction(int handleNum);
  void EndAngleInteraction(int handleNum);

//BTX
  friend class vtkAngleWidgetCallback;
//ETX

private:
  vtkAngleWidget(const vtkAngleWidget&);  //Not implemented
  void operator=(const vtkAngleWidget&);  //Not implemented
};

#endif
