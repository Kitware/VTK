/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Cursor3D.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkCursor3D - generate a 3D cursor representation
// .SECTION Description
// vtkCursor3D is an object that generates a 3D representation of a cursor.
// The cursor consists of a wireframe bounding box, three intersecting 
// axes lines that meet at the cursor focus, and "shadows" or projections
// of the axes against the sides of the bounding box. Each of these
// components can be turned on/off.

#ifndef __vtkCursor3D_h
#define __vtkCursor3D_h

#include "PolySrc.hh"

class vtkCursor3D : public vtkPolySource 
{
public:
  vtkCursor3D();
  char *GetClassName() {return "vtkCursor3D";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetModelBounds(float *bounds);
  void SetModelBounds(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax);
  vtkGetVectorMacro(ModelBounds,float,6);

  // Description:
  // Specify the position of cursor focus.
  vtkSetVector3Macro(FocalPoint,float);
  vtkGetVectorMacro(FocalPoint,float,3);

  // Description:
  // Turn on/off the wireframe bounding box.
  vtkSetMacro(Outline,int);
  vtkGetMacro(Outline,int);
  vtkBooleanMacro(Outline,int);

  // Description:
  // Turn on/off the wireframe axes.
  // 
  vtkSetMacro(Axes,int);
  vtkGetMacro(Axes,int);
  vtkBooleanMacro(Axes,int);

  // Description:
  // Turn on/off the wireframe x-shadows.
  vtkSetMacro(XShadows,int);
  vtkGetMacro(XShadows,int);
  vtkBooleanMacro(XShadows,int);

  // Description:
  // Turn on/off the wireframe y-shadows.
  vtkSetMacro(YShadows,int);
  vtkGetMacro(YShadows,int);
  vtkBooleanMacro(YShadows,int);

  // Description:
  // Turn on/off the wireframe z-shadows.
  vtkSetMacro(ZShadows,int);
  vtkGetMacro(ZShadows,int);
  vtkBooleanMacro(ZShadows,int);

  // Description:
  // Turn on/off cursor wrapping. If the cursor focus moves outside the
  // specified bounds, the cursor will either be restrained against the
  // nearest "wall" (Wrap=off), or it will wrap around (Wrap=on).
  vtkSetMacro(Wrap,int);
  vtkGetMacro(Wrap,int);
  vtkBooleanMacro(Wrap,int);

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


