/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergePoints2D.h
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
// .NAME vtkMergePoints2D - merge exactly coincident points
// .SECTION Description
// vtkMergePoints2D is a locator object to quickly locate points in 3D.
// The primary difference between vtkMergePoints2D and its superclass
// vtkPointLocator is that vtkMergePoints2D merges precisely coincident points
// and is therefore much faster.
// .SECTION See Also
// vtkCleanPolyData

#ifndef __vtkMergePoints2D_h
#define __vtkMergePoints2D_h

#include "vtkPointLocator2D.h"

class VTK_FILTERING_EXPORT vtkMergePoints2D : public vtkPointLocator2D
{
public:
  static vtkMergePoints2D *New();
  vtkTypeRevisionMacro(vtkMergePoints2D,vtkPointLocator2D);

  // Description:
  // Determine whether point given by x[] has been inserted into points list.
  // Return id of previously inserted point if this is true, otherwise return
  // -1.
  int IsInsertedPoint(float x[2]);

protected:
  vtkMergePoints2D() {};
  ~vtkMergePoints2D() {};

private:
  vtkMergePoints2D(const vtkMergePoints2D&);  // Not implemented.
  void operator=(const vtkMergePoints2D&);  // Not implemented.
};

#endif


