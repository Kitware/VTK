/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScalarBarWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkScalarBarWidget - 2D widget for manipulating a scalar bar
// .SECTION Description
// This class provides support for interactively manipulating the position,
// size, and orientation of a scalar bar. It listens to Left mouse events and
// mouse movement. It also listens to Right mouse events and notifies any 
// observers of Right mouse events on this object when they occur.
// It will change the cursor shape based on its location. If
// the cursor is over an edge of the scalar bar it will change the cursor
// shape to a resize edge shape. If the position of a scalar bar is moved to
// be close to the center of one of the four edges of the viewport, then the
// scalar bar will change its orientation to align with that edge. This
// orientation is sticky in that it will stay that orientation until the
// position is moved close to another edge.

// .SECTION See Also
// vtkInteractorObserver


#ifndef __vtkScalarBarWidget_h
#define __vtkScalarBarWidget_h

#include "vtkInteractorObserver.h"
class vtkScalarBarActor;

class VTK_WIDGETS_EXPORT vtkScalarBarWidget : public vtkInteractorObserver
{
public:
  static vtkScalarBarWidget *New();
  vtkTypeRevisionMacro(vtkScalarBarWidget,vtkInteractorObserver);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the ScalarBar used by this Widget. One is created automatically.
  void SetScalarBarActor(vtkScalarBarActor *scalarbar);
  vtkGetObjectMacro(ScalarBarActor,vtkScalarBarActor);
  
  // Description:
  // Methods for turning the interactor observer on and off.
  virtual void SetEnabled(int);

protected:
  vtkScalarBarWidget();
  ~vtkScalarBarWidget();

  // the actor that is used
  vtkScalarBarActor *ScalarBarActor;

  //handles the events
  static void ProcessEvents(vtkObject* object, 
                            unsigned long event,
                            void* clientdata, 
                            void* calldata);

  // ProcessEvents() dispatches to these methods.
  void OnLeftButtonDown();
  void OnLeftButtonUp();
  void OnRightButtonDown();
  void OnRightButtonUp();
  void OnMouseMove();

  // used to compute relative movements
  double StartPosition[2];
  
//BTX - manage the state of the widget
  int State;
  // use this to track whether left/right button was pressed to gate
  // action on button up event.
  int LeftButtonDown;
  int RightButtonDown;
  enum WidgetState
  {
    Moving=0,
    AdjustingP1,
    AdjustingP2,
    AdjustingP3,
    AdjustingP4,
    AdjustingE1,
    AdjustingE2,
    AdjustingE3,
    AdjustingE4,
    Inside,
    Outside
  };
//ETX

  // use to determine what state the mouse is over, edge1 p1, etc.
  // returns a state from the WidgetState enum above
  int ComputeStateBasedOnPosition(int X, int Y, int *pos1, int *pos2);

  // set the cursor to the correct shape based on State argument
  void SetCursor(int State);

private:
  vtkScalarBarWidget(const vtkScalarBarWidget&);  //Not implemented
  void operator=(const vtkScalarBarWidget&);  //Not implemented
};

#endif
