/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiBlockDataVisitor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiBlockDataVisitor - visitor to apply a command to all datasets
// .SECTION Description
// vtkMultiBlockDataVisitor is a concrete implementation of 
// vtkCompositeDataIterator. It iterates over the collection
// and applies the command to each. It will recursively
// iterate over items in the composite datasets in the collection.

#ifndef __vtkMultiBlockDataVisitor_h
#define __vtkMultiBlockDataVisitor_h

#include "vtkCompositeDataVisitor.h"

class vtkCompositeDataIterator;
class vtkDataSet;
class vtkMultiBlockDataIterator;

class VTK_COMMON_EXPORT vtkMultiBlockDataVisitor : public vtkCompositeDataVisitor
{
public:
  static vtkMultiBlockDataVisitor *New();

  vtkTypeRevisionMacro(vtkMultiBlockDataVisitor,vtkCompositeDataVisitor);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the iterator used to access the items in the input.
  void SetDataIterator(vtkMultiBlockDataIterator* it);
  vtkGetObjectMacro(DataIterator, vtkMultiBlockDataIterator);

  // Description:
  // Apply the command on each object in the collection.
  virtual void Execute();

protected:
  vtkMultiBlockDataVisitor(); 
  virtual ~vtkMultiBlockDataVisitor(); 

  vtkMultiBlockDataIterator* DataIterator;

  void ExecuteDataSet(vtkDataSet* ds);
  void ExecuteCompositeDataSet(vtkCompositeDataIterator* iter);

private:
  vtkMultiBlockDataVisitor(const vtkMultiBlockDataVisitor&);  // Not implemented.
  void operator=(const vtkMultiBlockDataVisitor&);  // Not implemented.
};

#endif

