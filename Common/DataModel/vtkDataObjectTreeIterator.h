/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObjectTreeIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDataObjectTreeIterator - superclass for composite data iterators
// .SECTION Description
// vtkDataObjectTreeIterator provides an interface for accessing datasets
// in a collection (vtkDataObjectTreeIterator).
#ifndef __vtkDataObjectTreeIterator_h
#define __vtkDataObjectTreeIterator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCompositeDataIterator.h"
#include "vtkSmartPointer.h" //to store data sets

class vtkDataObjectTree;
class vtkDataObjectTreeInternals;
class vtkDataObjectTreeIndex;
class vtkDataObject;
class vtkInformation;

class VTKCOMMONDATAMODEL_EXPORT vtkDataObjectTreeIterator : public vtkCompositeDataIterator
{
public:
  static vtkDataObjectTreeIterator* New();
  vtkTypeMacro(vtkDataObjectTreeIterator, vtkCompositeDataIterator);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Move the iterator to the beginning of the collection.
  virtual void GoToFirstItem();

  // Description:
  // Move the iterator to the next item in the collection.
  virtual void GoToNextItem();

  // Description:
  // Test whether the iterator is finished with the traversal.
  // Returns 1 for yes, and 0 for no.
  // It is safe to call any of the GetCurrent...() methods only when
  // IsDoneWithTraversal() returns 0.
  virtual int IsDoneWithTraversal();

  // Description:
  // Returns the current item. Valid only when IsDoneWithTraversal() returns 0.
  virtual vtkDataObject* GetCurrentDataObject();

  // Description:
  // Returns the meta-data associated with the current item.
  // Note that, depending on iterator implementation, the returned information
  // is not necessarily stored on the current object. So modifying the information
  // is forbidden.
  virtual vtkInformation* GetCurrentMetaData();

  // Description:
  // Returns if the a meta-data information object is present for the current
  // item. Return 1 on success, 0 otherwise.
  virtual int HasCurrentMetaData();

  // Description:
  // Flat index is an index obtained by traversing the tree in preorder.
  // This can be used to uniquely identify nodes in the tree.
  // Not valid if IsDoneWithTraversal() returns true.
  virtual unsigned int GetCurrentFlatIndex();

//BTX
protected:
  vtkDataObjectTreeIterator();
  virtual ~vtkDataObjectTreeIterator();

  // Takes the current location to the next dataset. This traverses the tree in
  // preorder fashion.
  // If the current location is a composite dataset, next is its 1st child dataset.
  // If the current is not a composite dataset, then next is the next dataset.
  // This method gives no guarantees  whether the current dataset will be
  // non-null or leaf.
  void NextInternal();

  // Description:
  // Returns the index for the current data object.
  vtkDataObjectTreeIndex GetCurrentIndex();

  // Needs access to GetCurrentIndex().
  friend class vtkDataObjectTree;
  friend class vtkMultiDataSetInternal;

  unsigned int CurrentFlatIndex;

private:
  vtkDataObjectTreeIterator(const vtkDataObjectTreeIterator&); // Not implemented.
  void operator=(const vtkDataObjectTreeIterator&); // Not implemented.

  class vtkInternals;
  vtkInternals* Internals;
  friend class vtkInternals;

  // Description:
  // Helper method used by vtkInternals to get access to the internals of
  // vtkDataObjectTree.
  vtkDataObjectTreeInternals* GetInternals(vtkDataObjectTree*);

  // Cannot be called when this->IsDoneWithTraversal() return 1.
  void UpdateLocation();
//ETX
};

#endif
