/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalDataGroupFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHierarchicalDataGroupFilter - collects multiple inputs into one hierarchical dataset
// .SECTION Description
// Legacy class. Use vtkMultiGroupDataGroupFilter instead.
//
// .SECTION See Also
// vtkMultiGroupDataGroupFilter

#ifndef __vtkHierarchicalDataGroupFilter_h
#define __vtkHierarchicalDataGroupFilter_h

#include "vtkMultiGroupDataGroupFilter.h"

class VTK_GRAPHICS_EXPORT vtkHierarchicalDataGroupFilter : public vtkMultiGroupDataGroupFilter 
{
public:
  vtkTypeRevisionMacro(vtkHierarchicalDataGroupFilter,vtkMultiGroupDataGroupFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with PointIds and CellIds on; and ids being generated
  // as scalars.
  static vtkHierarchicalDataGroupFilter *New();

protected:
  vtkHierarchicalDataGroupFilter();
  ~vtkHierarchicalDataGroupFilter();

private:
  vtkHierarchicalDataGroupFilter(const vtkHierarchicalDataGroupFilter&);  // Not implemented.
  void operator=(const vtkHierarchicalDataGroupFilter&);  // Not implemented.
};

#endif


