/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStripper.h
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
// .NAME vtkStripper - create triangle strips and/or poly-lines
// .SECTION Description

// vtkStripper is a filter that generates triangle strips and/or poly-lines
// from input polygons, triangle strips, and lines. Input polygons are
// assembled into triangle strips only if they are triangles; other types of
// polygons are passed through to the output and not stripped. (Use
// vtkTriangleFilter to triangulate non-triangular polygons prior to running
// this filter if you need to strip all the data.) The filter will pass
// through (to the output) vertices if they are present in the input
// polydata.
//
// The ivar MaximumLength can be used to control the maximum
// allowable triangle strip and poly-line length.

// .SECTION Caveats
// If triangle strips or poly-lines exist in the input data they will
// be passed through to the output data. This filter will only construct
// triangle strips if triangle polygons are available; and will only 
// construct poly-lines if lines are available.

// .SECTION See Also
// vtkTriangleFilter

#ifndef __vtkStripper_h
#define __vtkStripper_h

#include "vtkPolyDataToPolyDataFilter.h"

class VTK_GRAPHICS_EXPORT vtkStripper : public vtkPolyDataToPolyDataFilter
{
public:
  vtkTypeRevisionMacro(vtkStripper,vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Construct object with MaximumLength set to 1000.
  static vtkStripper *New();

  // Description:
  // Specify the maximum number of triangles in a triangle strip,
  // and/or the maximum number of lines in a poly-line.
  vtkSetClampMacro(MaximumLength,int,4,100000);
  vtkGetMacro(MaximumLength,int);

protected:
  vtkStripper();
  ~vtkStripper() {}

  // Usual data generation method
  void Execute();

  int MaximumLength;

private:
  vtkStripper(const vtkStripper&);  // Not implemented.
  void operator=(const vtkStripper&);  // Not implemented.
};

#endif


