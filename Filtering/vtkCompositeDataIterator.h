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
// .NAME vtkCompositeDataIterator - abstract superclass for composite data iterators
// .SECTION Description
// vtkCompositeDataIterator provides an interface for accessing datasets
// in a collection (vtkCompositeDataIterator). Sub-classes provide the
// actual implementation.

#ifndef __vtkCompositeDataIterator_h
#define __vtkCompositeDataIterator_h

#include "vtkObject.h"

class vtkDataObject;
class vtkInformation;

class VTK_FILTERING_EXPORT vtkCompositeDataIterator : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkCompositeDataIterator,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Move the iterator to the beginning of the collection.
  void InitTraversal() { this->GoToFirstItem(); }

  // Description:
  // Move the iterator to the beginning of the collection.
  virtual void GoToFirstItem() = 0;

  // Description:
  // Move the iterator to the next item in the collection.
  virtual void GoToNextItem() = 0;

  // Description:
  // Test whether the iterator is currently pointing to a valid
  // item. Returns 1 for yes, 0 for no.
  virtual int IsDoneWithTraversal() = 0;

  // Description:
  // Get the information object associated with the current
  // data object.
  virtual vtkInformation* GetCurrentInformationObject() = 0;

  // Description:
  // Get the current item. Valid only when IsDoneWithTraversal()
  // returns 1.
  virtual vtkDataObject* GetCurrentDataObject() = 0;

  // Description:
  // If VisitOnlyLeaves is true, the iterator will only visit nodes
  // (sub-datasets) that are not composite. If it encounters a composite
  // data set, it will automatically traverse that composite dataset until
  // it finds non-composite datasets. With this options, it is possible to
  // visit all non-composite datasets in tree of composite datasets
  // (composite of composite of composite for example :-) ) If
  // VisitOnlyLeaves is false, GetCurrentDataObject() may return
  // vtkCompositeDataSet. By default, VisitOnlyLeaves is 1.
  vtkSetMacro(VisitOnlyLeaves, int);
  vtkGetMacro(VisitOnlyLeaves, int);
  vtkBooleanMacro(VisitOnlyLeaves, int);

protected:
  vtkCompositeDataIterator(); 
  virtual ~vtkCompositeDataIterator(); 

  int VisitOnlyLeaves;
private:
  vtkCompositeDataIterator(const vtkCompositeDataIterator&);  // Not implemented.
  void operator=(const vtkCompositeDataIterator&);  // Not implemented.
};

#endif

