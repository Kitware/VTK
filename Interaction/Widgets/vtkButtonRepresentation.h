/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkButtonRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkButtonRepresentation - abstract class defines the representation for a vtkButtonWidget
// .SECTION Description
// This abstract class is used to specify how the vtkButtonWidget should
// interact with representations of the vtkButtonWidget. This class may be
// subclassed so that alternative representations can be created. The class
// defines an API, and a default implementation, that the vtkButtonWidget
// interacts with to render itself in the scene.
//
// The vtkButtonWidget assumes an n-state button so that traveral methods
// are available for changing, querying and manipulating state. Derived
// classed determine the actual appearance. The state is represented by an
// integral value 0<=state<numStates.
//
// To use this representation, always begin by specifying the number of states.
// Then follow with the necessary information to represent each state (done through
// a subclass API).

// .SECTION See Also
// vtkButtonWidget


#ifndef __vtkButtonRepresentation_h
#define __vtkButtonRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkWidgetRepresentation.h"


class VTKINTERACTIONWIDGETS_EXPORT vtkButtonRepresentation : public vtkWidgetRepresentation
{
public:
  // Description:
  // Standard methods for the class.
  vtkTypeMacro(vtkButtonRepresentation,vtkWidgetRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Retrieve the current button state.
  vtkSetClampMacro(NumberOfStates,int,1,VTK_INT_MAX);

  // Description:
  // Retrieve the current button state.
  vtkGetMacro(State,int);

  // Description:
  // Manipulate the state. Note that the NextState() and PreviousState() methods
  // use modulo traveral. The "state" integral value will be clamped within
  // the possible state values (0<=state<NumberOfStates). Note that subclasses
  // will override these methods in many cases.
  virtual void SetState(int state);
  virtual void NextState();
  virtual void PreviousState();

  enum _InteractionState
  {
    Outside=0,
    Inside
  };
//ETX

  // Description:
  // These methods control the appearance of the button as it is being
  // interacted with. Subclasses will behave differently depending on their
  // particulars.  HighlightHovering is used when the mouse pointer moves
  // over the button. HighlightSelecting is set when the button is selected.
  // Otherwise, the HighlightNormal is used. The Highlight() method will throw
  // a vtkCommand::HighlightEvent.
  enum _HighlightState {HighlightNormal,HighlightHovering,HighlightSelecting};
  virtual void Highlight(int);
  vtkGetMacro(HighlightState,int);

  // Description:
  // Satisfy some of vtkProp's API.
  virtual void ShallowCopy(vtkProp *prop);

protected:
  vtkButtonRepresentation();
  ~vtkButtonRepresentation();

  // Values
  int NumberOfStates;
  int State;
  int HighlightState;

private:
  vtkButtonRepresentation(const vtkButtonRepresentation&);  //Not implemented
  void operator=(const vtkButtonRepresentation&);  //Not implemented
};

#endif
