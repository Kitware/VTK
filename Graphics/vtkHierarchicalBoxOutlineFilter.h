/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalBoxOutlineFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHierarchicalBoxOutlineFilter - reate wireframe outline for hierarchical datasets
// .SECTION Description
// vtkHierarchicalBoxOutlineFilter creates an outline for each vtkUniformGrid
// in a vtkHierarchicalBoxDataSet

// .SECTION See Also
// vtkOutlineFilter

#ifndef __vtkHierarchicalBoxOutlineFilter_h
#define __vtkHierarchicalBoxOutlineFilter_h

#include "vtkHierarchicalBoxToPolyDataFilter.h"

class vtkDataObject;

class VTK_GRAPHICS_EXPORT vtkHierarchicalBoxOutlineFilter : public vtkHierarchicalBoxToPolyDataFilter
{
public:
  static vtkHierarchicalBoxOutlineFilter *New();

  vtkTypeRevisionMacro(vtkHierarchicalBoxOutlineFilter,
                       vtkHierarchicalBoxToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkHierarchicalBoxOutlineFilter();
  ~vtkHierarchicalBoxOutlineFilter();

  virtual void ExecuteData(vtkDataObject*);

private:
  vtkHierarchicalBoxOutlineFilter(const vtkHierarchicalBoxOutlineFilter&);  // Not implemented.
  void operator=(const vtkHierarchicalBoxOutlineFilter&);  // Not implemented.
};


#endif



