/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompassWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/


// .NAME vtkCompassWidget - set a value by manipulating something
// .SECTION Description
// The vtkCompassWidget is used to adjust a scalar value in an
// application. Note that the actual appearance of the widget depends on
// the specific representation for the widget.
//
// To use this widget, set the widget representation. (the details may
// vary depending on the particulars of the representation).
//
//
// .SECTION Event Bindings
// By default, the widget responds to the following VTK events (i.e., it
// watches the vtkRenderWindowInteractor for these events):
// <pre>
// If the slider bead is selected:
//   LeftButtonPressEvent - select slider
//   LeftButtonReleaseEvent - release slider
//   MouseMoveEvent - move slider
// </pre>
//
// Note that the event bindings described above can be changed using this
// class's vtkWidgetEventTranslator. This class translates VTK events
// into the vtkCompassWidget's widget events:
// <pre>
//   vtkWidgetEvent::Select -- some part of the widget has been selected
//   vtkWidgetEvent::EndSelect -- the selection process has completed
//   vtkWidgetEvent::Move -- a request for slider motion has been invoked
// </pre>
//
// In turn, when these widget events are processed, the vtkCompassWidget
// invokes the following VTK events on itself (which observers can listen for):
// <pre>
//   vtkCommand::StartInteractionEvent (on vtkWidgetEvent::Select)
//   vtkCommand::EndInteractionEvent (on vtkWidgetEvent::EndSelect)
//   vtkCommand::InteractionEvent (on vtkWidgetEvent::Move)
// </pre>
//

#ifndef vtkCompassWidget_h
#define vtkCompassWidget_h

#include "vtkGeovisCoreModule.h" // For export macro
#include "vtkAbstractWidget.h"

class vtkCompassRepresentation;


class VTKGEOVISCORE_EXPORT vtkCompassWidget : public vtkAbstractWidget
{
public:
  // Description:
  // Instantiate the class.
  static vtkCompassWidget *New();

  // Description:
  // Standard macros.
  vtkTypeMacro(vtkCompassWidget,vtkAbstractWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify an instance of vtkWidgetRepresentation used to represent this
  // widget in the scene. Note that the representation is a subclass of vtkProp
  // so it can be added to the renderer independent of the widget.
  void SetRepresentation(vtkCompassRepresentation *r)
  {this->Superclass::SetWidgetRepresentation
     (reinterpret_cast<vtkWidgetRepresentation*>(r));}

  // Description:
  // Create the default widget representation if one is not set.
  void CreateDefaultRepresentation();

  // Description:
  // Get the value for this widget.
  double GetHeading();
  void SetHeading(double v);
  double GetTilt();
  void SetTilt(double t);
  double GetDistance();
  void SetDistance(double t);

protected:
  vtkCompassWidget();
  ~vtkCompassWidget() {}

  // These are the events that are handled
  static void SelectAction(vtkAbstractWidget*);
  static void EndSelectAction(vtkAbstractWidget*);
  static void MoveAction(vtkAbstractWidget*);
  static void TimerAction(vtkAbstractWidget*);

//BTX - manage the state of the widget
  int WidgetState;
  enum _WidgetState
  {
    Start=0,
    Highlighting,
    Adjusting,
    TiltAdjusting,
    DistanceAdjusting
  };
//ETX

  int TimerId;
  int TimerDuration;
  double StartTime;

private:
  vtkCompassWidget(const vtkCompassWidget&);  //Not implemented
  void operator=(const vtkCompassWidget&);  //Not implemented
};

#endif
