/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransformPolyDataFilter.h
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
// .NAME vtkTransformPolyDataFilter - transform points and associated normals and vectors for polygonal dataset
// .SECTION Description
// vtkTransformPolyDataFilter is a filter to transform point
// coordinates and associated point and cell normals and
// vectors. Other point and cell data is passed through the filter
// unchanged. This filter is specialized for polygonal data. See
// vtkTransformFilter for more general data.
//
// An alternative method of transformation is to use vtkActor's methods
// to scale, rotate, and translate objects. The difference between the
// two methods is that vtkActor's transformation simply effects where
// objects are rendered (via the graphics pipeline), whereas
// vtkTransformPolyDataFilter actually modifies point coordinates in the 
// visualization pipeline. This is necessary for some objects 
// (e.g., vtkProbeFilter) that require point coordinates as input.

// .SECTION See Also
// vtkTransform vtkTransformFilter vtkActor

#ifndef __vtkTransformPolyDataFilter_h
#define __vtkTransformPolyDataFilter_h

#include "vtkPolyDataToPolyDataFilter.h"
#include "vtkTransform.h"

class VTK_GRAPHICS_EXPORT vtkTransformPolyDataFilter : public vtkPolyDataToPolyDataFilter
{
public:
  static vtkTransformPolyDataFilter *New();
  vtkTypeRevisionMacro(vtkTransformPolyDataFilter,vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return the MTime also considering the transform.
  unsigned long GetMTime();

  // Description:
  // Specify the transform object used to transform points.
  vtkSetObjectMacro(Transform,vtkAbstractTransform);
  vtkGetObjectMacro(Transform,vtkAbstractTransform);

protected:
  vtkTransformPolyDataFilter();
  ~vtkTransformPolyDataFilter();

  void Execute();
  vtkAbstractTransform *Transform;
private:
  vtkTransformPolyDataFilter(const vtkTransformPolyDataFilter&);  // Not implemented.
  void operator=(const vtkTransformPolyDataFilter&);  // Not implemented.
};

#endif


