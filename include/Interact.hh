/*=========================================================================

  Program:   Visualization Library
  Module:    Interact.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlInteractiveRenderer - provide event driven interface to renderer
// .SECTION Description
// vlInteractiveRenderer is a convenience object that provides event 
// bindings to common graphics functions. For example, camera 
// zoom-in/zoom-out, azimuth, and roll.

// .SECTION Event Bindings
// These are the current keyboard bindings:
//    i - up camera elevation
//    m - down camera elevation
//    j - left camera azimuth
//    k - right camera azimuth
//    a - zoom in
//    z - zoom out
//    w - turn all actors wireframe
//    s - turn all actors surface

#ifndef __vlInteractiveRenderer_h
#define __vlInteractiveRenderer_h

#include "RenderW.hh"
#include "Camera.hh"
#include "Light.hh"

class vlInteractiveRenderer : public vlObject
{
public:
  vlInteractiveRenderer();
  ~vlInteractiveRenderer();
  char *GetClassName() {return "vlInteractiveRenderer";};
  void PrintSelf(ostream& os, vlIndent indent);

  virtual void Initialize() = 0;
  virtual void Start() = 0;

  // Description:
  // Set the rendering window being controlled by this object.
  vlSetObjectMacro(RenderWindow,vlRenderWindow);
  vlGetObjectMacro(RenderWindow,vlRenderWindow);

  // Description:
  // Specify the camera being controlled by this object.
  vlSetObjectMacro(Camera,vlCamera);
  vlGetObjectMacro(Camera,vlCamera);

  // Description:
  // Specify the light being controlled by this object.
  vlSetObjectMacro(Light,vlLight);
  vlGetObjectMacro(Light,vlLight);

  // Description:
  // Turn on/off the automatic repositioning of lights as the camera moves.
  vlSetMacro(LightFollowCamera,int);
  vlGetMacro(LightFollowCamera,int);
  vlBooleanMacro(LightFollowCamera,int);

protected:
  vlRenderWindow *RenderWindow;
  vlCamera   *Camera;
  vlLight    *Light;
  int LightFollowCamera;
};

#endif
