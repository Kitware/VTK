/*=========================================================================
  Program:   Visualization Toolkit
  Module:    vtkHierarchicalBoxDataSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHierarchicalBoxDataSet - Backwards compatibility class
//
// .SECTION Description
// An empty class for backwards compatiblity
//
// .SECTION See Also
// vtkUniformGridAM vtkOverlappingAMR vtkNonOverlappingAMR

#include "vtkOverlappingAMR.h"

class VTK_FILTERING_EXPORT vtkHierarchicalBoxDataSet:
  public vtkOverlappingAMR
{
public:
  static vtkHierarchicalBoxDataSet *New();
  vtkTypeMacro(vtkHierarchicalBoxDataSet,vtkOverlappingAMR);
  void PrintSelf(ostream &os, vtkIndent indent);

protected:
  vtkHierarchicalBoxDataSet();
  virtual ~vtkHierarchicalBoxDataSet();

private:
  vtkHierarchicalBoxDataSet(const vtkHierarchicalBoxDataSet&); // Not implemented
  void operator=(const vtkHierarchicalBoxDataSet&); // Not implemented
};
