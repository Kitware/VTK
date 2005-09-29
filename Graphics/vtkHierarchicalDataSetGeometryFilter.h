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
// .NAME vtkHierarchicalDataSetGeometryFilter - extract geometry from hierarchical data
// .SECTION Description
// Legacy class. Use vtkMultiGroupDataGeometryFilter instead.
//
// .SECTION See Also
// vtkMultiGroupDataGeometryFilter

#ifndef __vtkHierarchicalDataSetGeometryFilter_h
#define __vtkHierarchicalDataSetGeometryFilter_h

#include "vtkMultiGroupDataGeometryFilter.h"

class vtkPolyData;

class VTK_GRAPHICS_EXPORT vtkHierarchicalDataSetGeometryFilter : public vtkMultiGroupDataGeometryFilter
{
public:
  static vtkHierarchicalDataSetGeometryFilter *New();
  vtkTypeRevisionMacro(vtkHierarchicalDataSetGeometryFilter,vtkMultiGroupDataGeometryFilter);
  void PrintSelf(ostream& os, vtkIndent indent);


protected:
  vtkHierarchicalDataSetGeometryFilter();
  ~vtkHierarchicalDataSetGeometryFilter();

private:
  vtkHierarchicalDataSetGeometryFilter(const vtkHierarchicalDataSetGeometryFilter&);  // Not implemented.
  void operator=(const vtkHierarchicalDataSetGeometryFilter&);  // Not implemented.
};

#endif


