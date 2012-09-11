/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeDataIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCompositeDataIterator - superclass for composite data iterators
// .SECTION Description
// vtkCompositeDataIterator provides an interface for accessing datasets
// in a collection (vtkCompositeDataIterator).
#ifndef __vtkCompositeDataIterator_h
#define __vtkCompositeDataIterator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

class vtkCompositeDataSet;
class vtkCompositeDataSetInternals;
class vtkCompositeDataSetIndex;
class vtkDataObject;
class vtkInformation;

class VTKCOMMONDATAMODEL_EXPORT vtkCompositeDataIterator : public vtkObject
{
public:
  vtkTypeMacro(vtkCompositeDataIterator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the composite dataset this iterator is iterating over.
  // Must be set before traversal begins.
  virtual void SetDataSet(vtkCompositeDataSet* ds);
  vtkGetObjectMacro(DataSet, vtkCompositeDataSet);

  // Description:
  // Begin iterating over the composite dataset structure.
  virtual void InitTraversal();

  // Description:
  // Begin iterating over the composite dataset structure in reverse order.
  virtual void InitReverseTraversal();

  // Description:
  // Move the iterator to the beginning of the collection.
  virtual void GoToFirstItem() = 0;

  // Description:
  // Move the iterator to the next item in the collection.
  virtual void GoToNextItem() =0;

  // Description:
  // Test whether the iterator is finished with the traversal.
  // Returns 1 for yes, and 0 for no.
  // It is safe to call any of the GetCurrent...() methods only when
  // IsDoneWithTraversal() returns 0.
  virtual int IsDoneWithTraversal() =0;

  // Description:
  // Returns the current item. Valid only when IsDoneWithTraversal() returns 0.
  virtual vtkDataObject* GetCurrentDataObject() = 0;

  // Description:
  // Returns the meta-data associated with the current item. This will allocate
  // a new vtkInformation object is none is already present. Use
  // HasCurrentMetaData to avoid unnecessary creation of vtkInformation objects.
  virtual vtkInformation* GetCurrentMetaData() =0;

  // Description:
  // Returns if the a meta-data information object is present for the current
  // item. Return 1 on success, 0 otherwise.
  virtual int HasCurrentMetaData() =0;

  // Description:
  // If SkipEmptyNodes is true, then NULL datasets will be skipped. Default is
  // true.
  vtkSetMacro(SkipEmptyNodes, int);
  vtkGetMacro(SkipEmptyNodes, int);
  vtkBooleanMacro(SkipEmptyNodes, int);

  // Description:
  // Flat index is an index to identify the data in a composite data structure
  virtual unsigned int GetCurrentFlatIndex()=0;

  // Description:
  // Returns if the iteration is in reverse order.
  vtkGetMacro(Reverse, int);

//BTX
protected:
  vtkCompositeDataIterator();
  virtual ~vtkCompositeDataIterator();
  int SkipEmptyNodes;
  int Reverse;
  vtkCompositeDataSet* DataSet;

private:
  vtkCompositeDataIterator(const vtkCompositeDataIterator&); // Not implemented.
  void operator=(const vtkCompositeDataIterator&); // Not implemented.
//ETX
};

#endif


