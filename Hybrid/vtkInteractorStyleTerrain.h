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
// The mouse binding for this class is as follows. Left mouse click followed
// rotates the camera around the focal point using both elevation and azimuth
// invocations on the camera. Left mouse motion in the horizontal direction
// results in azimuth motion; left mouse motion in the vertical direction
// results in elevation motion. Therefore, diagonal motion results in a
// combination of azimuth and elevation. (If the shift key is held during
// motion, then only one of elevation or azimuth is invoked, depending on the
// whether the mouse motion is primarily horizontal or vertical.) Middle
// mouse button pans the camera across the scene (again the shift key has a
// similar effect on limiting the motion to the vertical or horizontal
// direction. The right mouse is used to dolly (e.g., a type of zoom) towards
// or away from the focal point.
//
// The class also supports some keypress events. The "r" key resets the
// camera.  The "e" key invokes the exit callback and by default exits the
// program. The "f" key sets a new camera focal point and flys towards that
// point. The "u" key invokes the user event. The "3" key toggles between 
// stereo and non-stero mode. The "l" key toggles on/off a latitude/longitude
// markers that can be used to estimate/control position.
// 

// .SECTION See Also
// vtkInteractorObserver vtkInteractorStyle vtk3DWidget

#ifndef __vtkInteractorStyleTerrain_h
#define __vtkInteractorStyleTerrain_h

#include "vtkInteractorObserver.h"

class vtkCallbackCommand;
class vtkPropPicker;
class vtkPolyDataMapper;
class vtkSphereSource;

class VTK_HYBRID_EXPORT vtkInteractorStyleTerrain : public vtkInteractorObserver
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
  virtual void SetEnabled(int);

  // Description:
  // Turn on/off the latitude/longitude lines.
  vtkSetMacro(LatLongLines,int);
  vtkGetMacro(LatLongLines,int);
  vtkBooleanMacro(LatLongLines,int);

protected:
  vtkInteractorStyleTerrain();
  ~vtkInteractorStyleTerrain();

//BTX - manage the state of the widget
  int State;
  enum WidgetState
  {
    Start=0,
    Rotating,
    Panning,
    Zooming,
    Outside
  };
//ETX
    
  //handles the char widget activation event. Also handles the delete event.
  static void ProcessEvents(vtkObject* object, unsigned long event,
                            void* clientdata, void* calldata);

  // ProcessEvents() dispatches to these methods.
  void OnLeftButtonDown(int ctrl, int shift, int X, int Y);
  void OnLeftButtonUp(int ctrl, int shift, int X, int Y);
  void OnMiddleButtonDown(int ctrl, int shift, int X, int Y);
  void OnMiddleButtonUp(int ctrl, int shift, int X, int Y);
  void OnRightButtonDown(int ctrl, int shift, int X, int Y);
  void OnRightButtonUp(int ctrl, int shift, int X, int Y);
  void OnMouseMove(int ctrl, int shift, int X, int Y);
  void OnChar();

  // Internal helper attributes
  int LatLongLines;
  void SelectRepresentation();
  vtkPropPicker *Picker;
  vtkSphereSource *LatLongSphere;
  vtkPolyDataMapper *LatLongMapper;
  vtkActor *LatLongActor;

private:
  vtkInteractorStyleTerrain(const vtkInteractorStyleTerrain&);  // Not implemented.
  void operator=(const vtkInteractorStyleTerrain&);  // Not implemented.
  
};

#endif

