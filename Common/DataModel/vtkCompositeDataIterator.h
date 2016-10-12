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
/**
 * @class   vtkCompositeDataIterator
 * @brief   superclass for composite data iterators
 *
 * vtkCompositeDataIterator provides an interface for accessing datasets
 * in a collection (vtkCompositeDataIterator).
*/

#ifndef vtkCompositeDataIterator_h
#define vtkCompositeDataIterator_h

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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Set the composite dataset this iterator is iterating over.
   * Must be set before traversal begins.
   */
  virtual void SetDataSet(vtkCompositeDataSet* ds);
  vtkGetObjectMacro(DataSet, vtkCompositeDataSet);
  //@}

  /**
   * Begin iterating over the composite dataset structure.
   */
  virtual void InitTraversal();

  /**
   * Begin iterating over the composite dataset structure in reverse order.
   */
  virtual void InitReverseTraversal();

  /**
   * Move the iterator to the beginning of the collection.
   */
  virtual void GoToFirstItem() = 0;

  /**
   * Move the iterator to the next item in the collection.
   */
  virtual void GoToNextItem() =0;

  /**
   * Test whether the iterator is finished with the traversal.
   * Returns 1 for yes, and 0 for no.
   * It is safe to call any of the GetCurrent...() methods only when
   * IsDoneWithTraversal() returns 0.
   */
  virtual int IsDoneWithTraversal() =0;

  /**
   * Returns the current item. Valid only when IsDoneWithTraversal() returns 0.
   */
  virtual vtkDataObject* GetCurrentDataObject() = 0;

  /**
   * Returns the meta-data associated with the current item. This will allocate
   * a new vtkInformation object is none is already present. Use
   * HasCurrentMetaData to avoid unnecessary creation of vtkInformation objects.
   */
  virtual vtkInformation* GetCurrentMetaData() =0;

  /**
   * Returns if the a meta-data information object is present for the current
   * item. Return 1 on success, 0 otherwise.
   */
  virtual int HasCurrentMetaData() =0;

  //@{
  /**
   * If SkipEmptyNodes is true, then NULL datasets will be skipped. Default is
   * true.
   */
  vtkSetMacro(SkipEmptyNodes, int);
  vtkGetMacro(SkipEmptyNodes, int);
  vtkBooleanMacro(SkipEmptyNodes, int);
  //@}

  /**
   * Flat index is an index to identify the data in a composite data structure
   */
  virtual unsigned int GetCurrentFlatIndex()=0;

  //@{
  /**
   * Returns if the iteration is in reverse order.
   */
  vtkGetMacro(Reverse, int);
  //@}

protected:
  vtkCompositeDataIterator();
  ~vtkCompositeDataIterator() VTK_OVERRIDE;
  int SkipEmptyNodes;
  int Reverse;
  vtkCompositeDataSet* DataSet;

private:
  vtkCompositeDataIterator(const vtkCompositeDataIterator&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCompositeDataIterator&) VTK_DELETE_FUNCTION;

};

#endif


