// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkUniformGridAMRDataIterator
 * @brief   subclass of vtkCompositeDataIterator
 * with API to get current level and dataset index.
 *
 */

#ifndef vtkUniformGridAMRDataIterator_h
#define vtkUniformGridAMRDataIterator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCompositeDataIterator.h"
#include "vtkSmartPointer.h" //for member variable Information

VTK_ABI_NAMESPACE_BEGIN
class vtkInformation;
class vtkAMRInformation;
class vtkAMRDataInternals;
class vtkUniformGridAMR;
class AMRIndexIterator;

class VTKCOMMONDATAMODEL_EXPORT vtkUniformGridAMRDataIterator : public vtkCompositeDataIterator
{
public:
  static vtkUniformGridAMRDataIterator* New();
  vtkTypeMacro(vtkUniformGridAMRDataIterator, vtkCompositeDataIterator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns the meta-data associated with the current item.
   * Note that this points to a single instance of vtkInformation object
   * allocated by the iterator and will be changed as soon as GoToNextItem is
   * called.
   */
  vtkInformation* GetCurrentMetaData() override;

  vtkTypeBool HasCurrentMetaData() override { return 1; }

  /**
   * Returns the current item. Valid only when IsDoneWithTraversal() returns 0.
   */
  vtkDataObject* GetCurrentDataObject() override;

  /**
   * Flat index is an index obtained by traversing the tree in preorder.
   * This can be used to uniquely identify nodes in the tree.
   * Not valid if IsDoneWithTraversal() returns true.
   */
  unsigned int GetCurrentFlatIndex() override;

  /**
   * Returns the level for the current dataset.
   */
  virtual unsigned int GetCurrentLevel();

  /**
   * Returns the dataset index for the current data object. Valid only if the
   * current data is a leaf node i.e. no a composite dataset.
   */
  virtual unsigned int GetCurrentIndex();

  /**
   * Move the iterator to the beginning of the collection.
   */
  void GoToFirstItem() override;

  /**
   * Move the iterator to the next item in the collection.
   */
  void GoToNextItem() override;

  /**
   * Test whether the iterator is finished with the traversal.
   * Returns 1 for yes, and 0 for no.
   * It is safe to call any of the GetCurrent...() methods only when
   * IsDoneWithTraversal() returns 0.
   */
  int IsDoneWithTraversal() override;

protected:
  vtkUniformGridAMRDataIterator();
  ~vtkUniformGridAMRDataIterator() override;
  vtkSmartPointer<AMRIndexIterator> Iter;

private:
  vtkUniformGridAMRDataIterator(const vtkUniformGridAMRDataIterator&) = delete;
  void operator=(const vtkUniformGridAMRDataIterator&) = delete;

  vtkSmartPointer<vtkInformation> Information;
  vtkSmartPointer<vtkUniformGridAMR> AMR;
  vtkAMRInformation* AMRInfo;
  vtkAMRDataInternals* AMRData;

  void GetCurrentIndexPair(unsigned int& level, unsigned int& id);
};

VTK_ABI_NAMESPACE_END
#endif
