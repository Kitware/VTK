/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCursor3D.h
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
// .NAME vtkCursor3D - generate a 3D cursor representation
// .SECTION Description
// vtkCursor3D is an object that generates a 3D representation of a cursor.
// The cursor consists of a wireframe bounding box, three intersecting 
// axes lines that meet at the cursor focus, and "shadows" or projections
// of the axes against the sides of the bounding box. Each of these
// components can be turned on/off.
//
// This filter generates two output datasets. The first (Output) is just the 
// geometric representation of the cursor. The second (Focus) is a single
// point at the focal point.

#ifndef __vtkCursor3D_h
#define __vtkCursor3D_h

#include "vtkPolyDataSource.h"

class VTK_GRAPHICS_EXPORT vtkCursor3D : public vtkPolyDataSource 
{
public:
  vtkTypeRevisionMacro(vtkCursor3D,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with model bounds = (-1,1,-1,1,-1,1), focal point = (0,0,0),
  // all parts of cursor visible, and wrapping off.
  static vtkCursor3D *New();

  // Description:
  // Set / get the boundary of the 3D cursor.
  void SetModelBounds(float xmin, float xmax, float ymin, float ymax, 
                      float zmin, float zmax);
  void SetModelBounds(float bounds[6]);
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

  // Description:
  // Get the focus for this filter.
  vtkPolyData *GetFocus() {return (vtkPolyData *)this->Focus;};

  // Description:
  // Turn every part of the 3D cursor on or off.
  void AllOn();
  void AllOff();

protected:
  vtkCursor3D();
  ~vtkCursor3D();

  void Execute();

  vtkPolyData *Focus;
  float ModelBounds[6];
  float FocalPoint[3];
  int Outline;
  int Axes;
  int XShadows;
  int YShadows;
  int ZShadows;
  int Wrap;
private:
  vtkCursor3D(const vtkCursor3D&);  // Not implemented.
  void operator=(const vtkCursor3D&);  // Not implemented.
};

#endif


