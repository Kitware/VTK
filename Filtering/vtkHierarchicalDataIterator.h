/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalDataIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHierarchicalDataIterator - iterator to access datasets in a vtkHierarchicalDataIterator
// .SECTION Description
// Legacy class. Use vtkMultiGroupDataIterator instead.
//  
// .SECTION See Also
// vtkMultiGroupDataIterator

#ifndef __vtkHierarchicalDataIterator_h
#define __vtkHierarchicalDataIterator_h

#include "vtkMultiGroupDataIterator.h"

class vtkHierarchicalDataSet;
class vtkHierarchicalDataIteratorInternal;

class VTK_FILTERING_EXPORT vtkHierarchicalDataIterator : public vtkMultiGroupDataIterator
{
public:
  static vtkHierarchicalDataIterator *New();

  vtkTypeRevisionMacro(vtkHierarchicalDataIterator,vtkMultiGroupDataIterator);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the data object to iterator over.
  vtkHierarchicalDataSet* GetDataSet();

protected:
  vtkHierarchicalDataIterator(); 
  virtual ~vtkHierarchicalDataIterator(); 

private:
  vtkHierarchicalDataIterator(const vtkHierarchicalDataIterator&);  // Not implemented.
  void operator=(const vtkHierarchicalDataIterator&);  // Not implemented.
};

#endif

