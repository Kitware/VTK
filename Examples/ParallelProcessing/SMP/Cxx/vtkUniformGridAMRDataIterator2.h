/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUniformGridAMRDataIterator2.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkUniformGridAMRDataIterator2 - subclass of vtkCompositeDataIterator
// with API to get current level and dataset index.
// .SECTION Description

#ifndef vtkUniformGridAMRDataIterator2_h
#define vtkUniformGridAMRDataIterator2_h

#include "vtkCompositeDataIterator.h"
#include "vtkSmartPointer.h" //for member variable Information

class vtkInformation;
class vtkAMRInformation;
class vtkAMRDataInternals2;
class vtkUniformGridAMR2;
class AMRIndexIterator;

class VTK_EXPORT vtkUniformGridAMRDataIterator2 :
  public vtkCompositeDataIterator
{
public:
  static vtkUniformGridAMRDataIterator2* New();
  vtkTypeMacro(vtkUniformGridAMRDataIterator2, vtkCompositeDataIterator);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the meta-data associated with the current item.
  // Note that this points to a single instance of vtkInformation object
  // allocated by the iterator and will be changed as soon as GoToNextItem is
  // called.
  virtual vtkInformation* GetCurrentMetaData();

  // Description:
  virtual int HasCurrentMetaData() { return 1;}

  // Description:
  // Returns the current item. Valid only when IsDoneWithTraversal() returns 0.
  virtual vtkDataObject* GetCurrentDataObject();

  // Description:
  // Flat index is an index obtained by traversing the tree in preorder.
  // This can be used to uniquely identify nodes in the tree.
  // Not valid if IsDoneWithTraversal() returns true.
  unsigned int GetCurrentFlatIndex();

  // Description:
  // Returns the level for the current dataset.
  virtual unsigned int GetCurrentLevel();

  // Description:
  // Returns the dataset index for the current data object. Valid only if the
  // current data is a leaf node i.e. no a composite dataset.
  virtual unsigned int GetCurrentIndex();

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

  virtual void CopyAndInit(vtkCompositeDataIterator* from, int deep=0);
  virtual unsigned int GetNumberOfBlocks();

//BTX
protected:
  vtkUniformGridAMRDataIterator2();
  ~vtkUniformGridAMRDataIterator2();
  vtkSmartPointer<AMRIndexIterator> Iter;
private:
  vtkUniformGridAMRDataIterator2(const vtkUniformGridAMRDataIterator2&); // Not implemented.
  void operator=(const vtkUniformGridAMRDataIterator2&); // Not implemented.

  vtkSmartPointer<vtkInformation> Information;
  vtkSmartPointer<vtkUniformGridAMR2> AMR;
  vtkAMRInformation* AMRInfo;
  vtkAMRDataInternals2* AMRData;

  void GetCurrentIndexPair(unsigned int& level, unsigned int& id);
//ETX
};

#endif
