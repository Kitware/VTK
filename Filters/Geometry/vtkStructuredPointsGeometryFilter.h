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
/**
 * @class   vtkStructuredPointsGeometryFilter
 * @brief   obsolete class
 *
 * vtkStructuredPointsGeometryFilter has been renamed to
 * vtkImageDataGeometryFilter
*/

#ifndef vtkStructuredPointsGeometryFilter_h
#define vtkStructuredPointsGeometryFilter_h

#include "vtkFiltersGeometryModule.h" // For export macro
#include "vtkImageDataGeometryFilter.h"

class VTKFILTERSGEOMETRY_EXPORT vtkStructuredPointsGeometryFilter : public vtkImageDataGeometryFilter
{
public:
  vtkTypeMacro(vtkStructuredPointsGeometryFilter,vtkImageDataGeometryFilter);

  /**
   * Construct with initial extent of all the data
   */
  static vtkStructuredPointsGeometryFilter *New();

protected:
  vtkStructuredPointsGeometryFilter();
  ~vtkStructuredPointsGeometryFilter() VTK_OVERRIDE {}

private:
  vtkStructuredPointsGeometryFilter(const vtkStructuredPointsGeometryFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkStructuredPointsGeometryFilter&) VTK_DELETE_FUNCTION;
};

#endif
// VTK-HeaderTest-Exclude: vtkStructuredPointsGeometryFilter.h
