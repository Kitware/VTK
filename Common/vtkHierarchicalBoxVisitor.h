/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalBoxVisitor.h
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
// .NAME vtkHierarchicalBoxVisitor - visitor to apply a command to all datasets
// .SECTION Description
// vtkHierarchicalBoxVisitor is a concrete implementation of 
// vtkCompositeDataIterator. It iterates over the collection
// and applies the command to each. When calling Execute()
// on the command, vtkHierarchicalBoxVisitor passes an instance
// of vtkHierarchicalBoxApplyFilterCommand::LevelInformation as
// the optional argument. This is used by vtkHierarchicalBoxApplyFilterCommand.

// .SECTION See Also
// vtkHierarchicalBoxApplyFilterCommand

#ifndef __vtkHierarchicalBoxVisitor_h
#define __vtkHierarchicalBoxVisitor_h

#include "vtkCompositeDataVisitor.h"

class vtkHierarchicalBoxDataSet;

class VTK_COMMON_EXPORT vtkHierarchicalBoxVisitor : public vtkCompositeDataVisitor
{
public:
  static vtkHierarchicalBoxVisitor *New();

  vtkTypeRevisionMacro(vtkHierarchicalBoxVisitor,vtkCompositeDataVisitor);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Apply the command on each object in the collection.
  virtual void Execute();

  // Description:
  // Set the data object to iterator over.
  void SetDataSet(vtkHierarchicalBoxDataSet* dataset);
  vtkGetObjectMacro(DataSet, vtkHierarchicalBoxDataSet);

protected:
  vtkHierarchicalBoxVisitor(); 
  virtual ~vtkHierarchicalBoxVisitor(); 

  vtkHierarchicalBoxDataSet* DataSet;

private:
  vtkHierarchicalBoxVisitor(const vtkHierarchicalBoxVisitor&);  // Not implemented.
  void operator=(const vtkHierarchicalBoxVisitor&);  // Not implemented.
};

#endif

