/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKitwareCutter.h
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
// .NAME vtkKitwareCutter - Cut vtkDataSet with user-specified implicit function
// .SECTION Description
// vtkKitwareCutter is a filter to cut through data using any subclass of 
// vtkImplicitFunction. That is, a polygonal surface is created
// corresponding to the implicit function F(x,y,z) = value(s), where
// you can specify one or more values used to cut with.
//
// In VTK, cutting means reducing a cell of dimension N to a cut surface
// of dimension N-1. For example, a tetrahedron when cut by a plane (i.e.,
// vtkPlane implicit function) will generate triangles. (Clipping takes
// a N dimensional cell and creates N dimension primitives.)
//
// vtkKitwareCutter is generally used to "slice-through" a dataset, generating
// a surface that can be visualized. It is also possible to use vtkKitwareCutter
// to do a form of volume rendering. vtkKitwareCutter does this by generating
// multiple cut surfaces (usually planes) which are ordered (and rendered)
// from back-to-front. The surfaces are set translucent to give a 
// volumetric rendering effect.
//
// vtkKitwareCutter uses the synchronized templates algorithm to do contouring.

// .SECTION See Also
// vtkImplicitFunction vtkClipPolyData

#ifndef __vtkKitwareCutter_h
#define __vtkKitwareCutter_h

#include "vtkCutter.h"

class VTK_PATENTED_EXPORT vtkKitwareCutter : public vtkCutter
{
public:
  static vtkKitwareCutter *New();
  vtkTypeRevisionMacro(vtkKitwareCutter, vtkCutter);
  void PrintSelf(ostream &os, vtkIndent indent);
  
protected:
  vtkKitwareCutter();
  ~vtkKitwareCutter();
  
  void Execute();

  void StructuredPointsCutter();
  void StructuredGridCutter();
  void RectilinearGridCutter();
  
private:
  vtkKitwareCutter(const vtkKitwareCutter&);  // Not implemented.
  void operator=(const vtkKitwareCutter&);  // Not implemented.
};

#endif
