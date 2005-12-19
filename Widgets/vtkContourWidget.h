/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContourWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkContourWidget - create a contour with a set of points
// .SECTION Description
// The vtkContourWidget is used to select a set of points, and draw lines
// between these points. The contour may be opened or closed, depending on
// how the last point is added. The widget handles all processing of widget
// events (that are triggered by VTK events). The vtkContourRepresentation is
// responsible for all placement of the points, calculation of the lines, and
// contour manipulation. This is done through two main helper classes:
// vtkPointPlacer and vtkContourLineInterpolator. The representation is also
// responsible for drawing the points and lines.
//
// .SECTION Event Bindings
// By default, the widget responds to the following VTK events (i.e., it
// watches the vtkRenderWindowInteractor for these events):
// <pre>
//   LeftButtonPressEvent - triggers a Select event
//   RightButtonPressEvent - triggers a FinalAddPoint event
//   MouseMoveEvent - triggers a Move event
//   LeftButtonReleaseEvent - triggers an EndSelect event
//   Delete key event - triggers a Delete event
// </pre>
//
// Note that the event bindings described above can be changed using this
// class's vtkWidgetEventTranslator. This class translates VTK events 
// into the vtkContourWidget's widget events:
// <pre>
//   vtkWidgetEvent::Select 
//        widget state is: 
//            Start or
//            Define: If we already have at least 2 nodes, test
//                 whether the current (X,Y) location is near an existing
//                 node. If so, close the contour and change to Manipulate
//                 state. Otherwise, attempt to add a node at this (X,Y)
//                 location.
//            Manipulate: If this (X,Y) location activates a node, then
//                 set the current operation to Translate. Otherwise, if
//                 this location is near the contour, attempt to add a 
//                 new node on the contour at this (X,Y) location.
//
//   vtkWidgetEvent::AddFinalPoint
//        widget state is: 
//            Start: Do nothing.
//            Define: If we already have at least 2 nodes, test
//                 whether the current (X,Y) location is near an existing
//                 node. If so, close the contour and change to Manipulate
//                 state. Otherwise, attempt to add a node at this (X,Y)
//                 location. If we do, then leave the contour open and
//                 change to Manipulate state.
//            Manipulate: Do nothing.
//
//   vtkWidgetEvent::Move
//        widget state is: 
//            Start or
//            Define: Do nothing.
//            Manipulate: If our operation is Translate, then call
//                  WidgetInteration on the representation. If our 
//                  operation is Inactive, then just attempt to activate
//                  a node at this (X,Y) location.
//
//   vtkWidgetEvent::EndSelect
//        widget state is: 
//            Start or
//            Define: Do nothing.
//            Manipulate: If our operation is not Inactive, set it to
//                  Inactive.
//
//   vtkWidgetEvent::Delete
//        widget state is: 
//            Start: Do nothing.
//            Define: Remove the last point on the contour.
//            Manipulate: Attempt to activate a node at (X,Y). If
//                   we do activate a node, delete it. If we now
//                   have less than 3 nodes, go back to Define state.
// </pre>
//
// This widget invokes the following VTK events on itself (which observers
// can listen for):
// <pre>
//   vtkCommand::StartInteractionEvent (beginning to interact)
//   vtkCommand::EndInteractionEvent (completing interaction)
//   vtkCommand::InteractionEvent (moving after selecting something)
//   vtkCommand::PlacePointEvent (after point is positioned; 
//                                call data includes handle id (0,1))
//   vtkCommand::WidgetValueChangedEvent (Invoked when the contour is closed
//                                        for the first time. )
// </pre>

// .SECTION See Also
// vtkHandleWidget 


#ifndef __vtkContourWidget_h
#define __vtkContourWidget_h

#include "vtkAbstractWidget.h"

class vtkContourRepresentation;

class VTK_WIDGETS_EXPORT vtkContourWidget : public vtkAbstractWidget
{
public:
  // Description:
  // Instantiate this class.
  static vtkContourWidget *New();

  // Description:
  // Standard methods for a VTK class.
  vtkTypeRevisionMacro(vtkContourWidget,vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The method for activiating and deactiviating this widget. This method
  // must be overridden because it is a composite widget and does more than
  // its superclasses' vtkAbstractWidget::SetEnabled() method.
  virtual void SetEnabled(int);

  // Description:
  // Specify an instance of vtkWidgetRepresentation used to represent this
  // widget in the scene. Note that the representation is a subclass of vtkProp
  // so it can be added to the renderer independent of the widget.
  void SetRepresentation(vtkContourRepresentation *r)
    {this->Superclass::SetWidgetRepresentation(reinterpret_cast<vtkWidgetRepresentation*>(r));}
  
  // Description:
  // Create the default widget representation if one is not set. 
  void CreateDefaultRepresentation();

protected:
  vtkContourWidget();
  ~vtkContourWidget();

  // The state of the widget
//BTX
  enum {Start,Define,Manipulate};
//ETX
  
  int WidgetState;
  int CurrentHandle;

  // Callback interface to capture events when
  // placing the widget.
  static void SelectAction(vtkAbstractWidget*);
  static void AddFinalPointAction(vtkAbstractWidget*);
  static void MoveAction(vtkAbstractWidget*);
  static void EndSelectAction(vtkAbstractWidget*);
  static void DeleteAction(vtkAbstractWidget*);
  
  // Methods invoked when the handles at the
  // end points of the widget are manipulated
  void StartInteraction();
  void Interaction();
  void EndInteraction();

  void SelectNode();
  void AddNode();
  
private:
  vtkContourWidget(const vtkContourWidget&);  //Not implemented
  void operator=(const vtkContourWidget&);  //Not implemented
};

#endif
