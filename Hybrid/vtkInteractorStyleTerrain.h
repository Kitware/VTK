/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleTerrain.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInteractorStyleTerrain - manipulate camera in scene with natural view up (e.g., terrain)
// .SECTION Description
// vtkInteractorStyleTerrain is used to manipulate a camera which is viewing
// a scene with a natural view up, e.g., terrain. The camera in such a
// scene is manipulated by specifying azimuth (angle around the view
// up vector) and elevation (the angle from the horizon).
//

// .SECTION See Also
// vtkInteractorObserver

#ifndef __vtkInteractorStyleTerrain_h
#define __vtkInteractorStyleTerrain_h

#include "vtkInteractorStyle.h"

class vtkCallbackCommand;

class VTK_HYBRID_EXPORT vtkInteractorStyleTerrain : public vtkInteractorStyle
{
public:
  // Description:
  // Instantiate the object.
  static vtkInteractorStyleTerrain *New();

  vtkTypeRevisionMacro(vtkInteractorStyleTerrain,vtkInteractorObserver);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the Interactor wrapper being controlled by this object.
  // (Satisfy superclass API.)
  virtual void SetInteractor(vtkRenderWindowInteractor *interactor);

protected:
  vtkInteractorStyleTerrain();
  ~vtkInteractorStyleTerrain();

//BTX - manage the state of the widget
  int State;
  enum WidgetState
  {
    Start=0,
    LeftDown,
    LeftMoving,
    Outside
  };
//ETX
    
  //handles the char widget activation event. Also handles the delete event.
  static void ProcessEvents(vtkObject* object, unsigned long event,
                            void* clientdata, void* calldata);

  // ProcessEvents() dispatches to these methods.
  void OnLeftButtonDown(int ctrl, int shift, int X, int Y);
  void OnLeftButtonUp(int ctrl, int shift, int X, int Y);
  void OnMouseMove(int ctrl, int shift, int X, int Y);
  void OnChar(int ctrl, int shift, char keycode, int repeatcount);

private:
  vtkInteractorStyleTerrain(const vtkInteractorStyleTerrain&);  // Not implemented.
  void operator=(const vtkInteractorStyleTerrain&);  // Not implemented.
  
};

#endif

