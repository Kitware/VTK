/*=========================================================================

  Program:   Visualization Library
  Module:    XInter.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlXInteractiveRenderer - provide Xevent driven interface to renderer
// .SECTION Description
// vlXInteractiveRenderer is a convenience object that provides event 
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

#ifndef __vlXInteractiveRenderer_h
#define __vlXInteractiveRenderer_h

//===========================================================
// now we define the C++ class

#include "Interact.hh"
#include "XRenWin.hh"
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>

class vlXInteractiveRenderer : public vlInteractiveRenderer
{
public:
  vlXInteractiveRenderer();
  ~vlXInteractiveRenderer();
  char *GetClassName() {return "vlXInteractiveRenderer";};
  void PrintSelf(ostream& os, vlIndent indent);

  virtual void Initialize();
  virtual void Start();
  void UpdateSize(int,int);
  void StartRotate();
  void EndRotate();

  friend void vlXInteractiveRendererCallback(Widget,XtPointer,
					     XEvent *,Boolean *);
  friend void vlXInteractiveRendererTimer(XtPointer,XtIntervalId *);

protected:
  Widget top;
  float Center[2];
  int Size[2];
  float DeltaAzimuth;
  float DeltaElevation;
  int   State;
  XtAppContext App;
};

#endif


