/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPointsGeometryFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkStructuredPointsGeometryFilter - obsolete class
// .SECTION Description
// vtkStructuredPointsGeometryFilter has been renamed to
// vtkImageDataGeometryFilter

#ifndef __vtkStructuredPointsGeometryFilter_h
#define __vtkStructuredPointsGeometryFilter_h

#include "vtkImageDataGeometryFilter.h"

class VTK_GRAPHICS_EXPORT vtkStructuredPointsGeometryFilter : public vtkImageDataGeometryFilter
{
public:
  vtkTypeMacro(vtkStructuredPointsGeometryFilter,vtkImageDataGeometryFilter);
  
  // Description:
  // Construct with initial extent of all the data
  static vtkStructuredPointsGeometryFilter *New();

protected:
  vtkStructuredPointsGeometryFilter();
  ~vtkStructuredPointsGeometryFilter() {};

private:
  vtkStructuredPointsGeometryFilter(const vtkStructuredPointsGeometryFilter&); // Not implemented
  void operator=(const vtkStructuredPointsGeometryFilter&); // Not implemented
};

#endif
