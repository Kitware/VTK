/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalDataSetGeometryFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHierarchicalDataSetGeometryFilter
 * @brief   extract geometry from hierarchical data
 *
 * Legacy class. Use vtkCompositeDataGeometryFilter instead.
 *
 * @sa
 * vtkCompositeDataGeometryFilter
*/

#ifndef vtkHierarchicalDataSetGeometryFilter_h
#define vtkHierarchicalDataSetGeometryFilter_h

#include "vtkFiltersGeometryModule.h" // For export macro
#include "vtkCompositeDataGeometryFilter.h"

class vtkPolyData;

class VTKFILTERSGEOMETRY_EXPORT vtkHierarchicalDataSetGeometryFilter :
  public vtkCompositeDataGeometryFilter
{
public:
  static vtkHierarchicalDataSetGeometryFilter *New();
  vtkTypeMacro(vtkHierarchicalDataSetGeometryFilter,
    vtkCompositeDataGeometryFilter);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;


protected:
  vtkHierarchicalDataSetGeometryFilter();
  ~vtkHierarchicalDataSetGeometryFilter() VTK_OVERRIDE;

private:
  vtkHierarchicalDataSetGeometryFilter(const vtkHierarchicalDataSetGeometryFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkHierarchicalDataSetGeometryFilter&) VTK_DELETE_FUNCTION;
};

#endif


