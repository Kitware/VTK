/*=========================================================================

  Program:   Visualization Library
  Module:    Cursor3D.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlCursor3D - generate a 3D cursor representation
// .SECTION Description
// vlCursor3D is an object that generates a 3D representation of a cursor.
// The cursor consists of a wireframe bounding box, three intersecting 
// axes lines that meet at the cursor focus, and "shadows" or projections
// of the axes against the sides of the bounding box. Each of these
// components can be turned on/off.

#ifndef __vlCursor3D_h
#define __vlCursor3D_h

#include "PolySrc.hh"

class vlCursor3D : public vlPolySource 
{
public:
  vlCursor3D();
  char *GetClassName() {return "vlCursor3D";};
  void PrintSelf(ostream& os, vlIndent indent);

  void SetModelBounds(float *bounds);
  void SetModelBounds(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax);
  vlGetVectorMacro(ModelBounds,float);

  // Description:
  // Specify the position of cursor focus.
  vlSetVector3Macro(FocalPoint,float);
  vlGetVectorMacro(FocalPoint,float);

  // Description:
  // Turn on/off the wireframe bounding box.
  vlSetMacro(Outline,int);
  vlGetMacro(Outline,int);
  vlBooleanMacro(Outline,int);

  // Description:
  // Turn on/off the wireframe axes.
  // 
  vlSetMacro(Axes,int);
  vlGetMacro(Axes,int);
  vlBooleanMacro(Axes,int);

  // Description:
  // Turn on/off the wireframe x-shadows.
  vlSetMacro(XShadows,int);
  vlGetMacro(XShadows,int);
  vlBooleanMacro(XShadows,int);

  // Description:
  // Turn on/off the wireframe y-shadows.
  vlSetMacro(YShadows,int);
  vlGetMacro(YShadows,int);
  vlBooleanMacro(YShadows,int);

  // Description:
  // Turn on/off the wireframe z-shadows.
  vlSetMacro(ZShadows,int);
  vlGetMacro(ZShadows,int);
  vlBooleanMacro(ZShadows,int);

  // Description:
  // Turn on/off cursor wrapping. If the cursor focus moves outside the
  // specified bounds, the cursor will either be restrained against the
  // nearest "wall" (Wrap=off), or it will wrap around (Wrap=on).
  vlSetMacro(Wrap,int);
  vlGetMacro(Wrap,int);
  vlBooleanMacro(Wrap,int);

protected:
  void Execute();

  float ModelBounds[6];
  float FocalPoint[3];
  int Outline;
  int Axes;
  int XShadows;
  int YShadows;
  int ZShadows;
  int Wrap;
};

#endif


