/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCursor3D.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
  vtkTypeMacro(vtkCursor3D,vtkPolyDataSource);
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


