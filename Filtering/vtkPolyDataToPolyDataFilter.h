/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataToPolyDataFilter.h
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
// .NAME vtkPolyDataToPolyDataFilter - abstract filter class
// .SECTION Description
// vtkPolyDataToPolyDataFilter is an abstract filter class whose subclasses
// take as input polygonal data and generate polygonal data on output.

// .SECTION See Also
// vtkCleanPolyData vtkDecimate vtkFeatureEdges vtkFeatureVertices
// vtkMaskPolyData vtkPolyDataNormals vtkSmoothPolyDataFilter vtkStripper
// vtkTransformPolyDataFilter vtkTriangleFilter vtkTubeFilter
// vtkLinearExtrusionFilter vtkRibbonFilter vtkRotationalExtrusionFilter
// vtkShrinkPolyData

#ifndef __vtkPolyDataToPolyDataFilter_h
#define __vtkPolyDataToPolyDataFilter_h

#include "vtkPolyDataSource.h"
#include "vtkPolyData.h"

class VTK_FILTERING_EXPORT vtkPolyDataToPolyDataFilter : public vtkPolyDataSource
{
public:
  vtkTypeRevisionMacro(vtkPolyDataToPolyDataFilter,vtkPolyDataSource);

  // Description:
  // Set / get the input data or filter.
  virtual void SetInput(vtkPolyData *input);
  vtkPolyData *GetInput();
  
protected:  
   vtkPolyDataToPolyDataFilter();
  ~vtkPolyDataToPolyDataFilter() {};

private:
  vtkPolyDataToPolyDataFilter(const vtkPolyDataToPolyDataFilter&);  // Not implemented.
  void operator=(const vtkPolyDataToPolyDataFilter&);  // Not implemented.
};

#endif


