/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUniformGridAMRDataIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkUniformGridAMRDataIterator - subclass of vtkCompositeDataIterator
// with API to get current level and dataset index.
// .SECTION Description

#ifndef vtkUniformGridAMRDataIterator_h
#define vtkUniformGridAMRDataIterator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCompositeDataIterator.h"
#include "vtkSmartPointer.h" //for member variable Information

class vtkInformation;
class vtkAMRInformation;
class vtkAMRDataInternals;
class vtkUniformGridAMR;
class AMRIndexIterator;

class VTKCOMMONDATAMODEL_EXPORT vtkUniformGridAMRDataIterator :
  public vtkCompositeDataIterator
{
public:
  static vtkUniformGridAMRDataIterator* New();
  vtkTypeMacro(vtkUniformGridAMRDataIterator, vtkCompositeDataIterator);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  // Description:
  // Returns the meta-data associated with the current item.
  // Note that this points to a single instance of vtkInformation object
  // allocated by the iterator and will be changed as soon as GoToNextItem is
  // called.
  vtkInformation* GetCurrentMetaData() VTK_OVERRIDE;

  // Description:
  int HasCurrentMetaData() VTK_OVERRIDE { return 1;}

  // Description:
  // Returns the current item. Valid only when IsDoneWithTraversal() returns 0.
  vtkDataObject* GetCurrentDataObject() VTK_OVERRIDE;

  // Description:
  // Flat index is an index obtained by traversing the tree in preorder.
  // This can be used to uniquely identify nodes in the tree.
  // Not valid if IsDoneWithTraversal() returns true.
  unsigned int GetCurrentFlatIndex() VTK_OVERRIDE;

  // Description:
  // Returns the level for the current dataset.
  virtual unsigned int GetCurrentLevel();

  // Description:
  // Returns the dataset index for the current data object. Valid only if the
  // current data is a leaf node i.e. no a composite dataset.
  virtual unsigned int GetCurrentIndex();

  // Description:
  // Move the iterator to the beginning of the collection.
  void GoToFirstItem() VTK_OVERRIDE;

  // Description:
  // Move the iterator to the next item in the collection.
  void GoToNextItem() VTK_OVERRIDE;

  // Description:
  // Test whether the iterator is finished with the traversal.
  // Returns 1 for yes, and 0 for no.
  // It is safe to call any of the GetCurrent...() methods only when
  // IsDoneWithTraversal() returns 0.
  int IsDoneWithTraversal() VTK_OVERRIDE;

protected:
  vtkUniformGridAMRDataIterator();
  ~vtkUniformGridAMRDataIterator() VTK_OVERRIDE;
  vtkSmartPointer<AMRIndexIterator> Iter;
private:
  vtkUniformGridAMRDataIterator(const vtkUniformGridAMRDataIterator&) VTK_DELETE_FUNCTION;
  void operator=(const vtkUniformGridAMRDataIterator&) VTK_DELETE_FUNCTION;

  vtkSmartPointer<vtkInformation> Information;
  vtkSmartPointer<vtkUniformGridAMR> AMR;
  vtkAMRInformation* AMRInfo;
  vtkAMRDataInternals* AMRData;

  void GetCurrentIndexPair(unsigned int& level, unsigned int& id);

};

#endif
