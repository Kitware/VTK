// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDataObjectTree
 * @brief   provides implementation for most abstract
 * methods in the superclass vtkCompositeDataSet
 *
 * vtkDataObjectTree is represents a collection
 * of datasets (including other composite datasets). It
 * provides an interface to access the datasets through iterators.
 * vtkDataObjectTree provides methods that are used by subclasses to store the
 * datasets.
 * vtkDataObjectTree provides the datastructure for a full tree
 * representation. Subclasses provide the semantics for it and control how
 * this tree is built.
 *
 * @sa
 * vtkDataObjectTreeIterator
 */

#ifndef vtkDataObjectTree_h
#define vtkDataObjectTree_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCompositeDataSet.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkCompositeDataIterator;
class vtkDataObjectTreeIterator;
class vtkDataObjectTreeInternals;
class vtkInformation;
class vtkInformationStringKey;
class vtkDataObject;

class VTKCOMMONDATAMODEL_EXPORT VTK_MARSHALAUTO vtkDataObjectTree : public vtkCompositeDataSet
{
public:
  vtkTypeMacro(vtkDataObjectTree, vtkCompositeDataSet);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return a new iterator (the iterator has to be deleted by user).
   */
  VTK_NEWINSTANCE virtual vtkDataObjectTreeIterator* NewTreeIterator();

  /**
   * Return a new iterator (the iterator has to be deleted by user).

   * Use NewTreeIterator when you have a pointer to a vtkDataObjectTree
   * and NewIterator when you have a pointer to a vtkCompositeDataSet;
   * NewIterator is inherited and calls NewTreeIterator internally.
   */
  VTK_NEWINSTANCE vtkCompositeDataIterator* NewIterator() override;

  /**
   * Copies the tree structure from the input. All pointers to non-composite
   * data objects are initialized to nullptr. This also shallow copies the meta data
   * associated with all the nodes.
   */
  void CopyStructure(vtkCompositeDataSet* input) override;

  /**
   * Sets the data set at the location pointed by the iterator.
   * The iterator does not need to be iterating over this dataset itself. It can
   * be any composite datasite with similar structure (achieved by using
   * CopyStructure).
   */
  void SetDataSet(vtkCompositeDataIterator* iter, vtkDataObject* dataObj) override;

  /**
   * Sets the data at the location provided by a vtkDataObjectTreeIterator
   */
  void SetDataSetFrom(vtkDataObjectTreeIterator* iter, vtkDataObject* dataObj);

  // Needed because, otherwise vtkCompositeData::GetDataSet(unsigned int flatIndex) is hidden.
  using Superclass::GetDataSet;
  /**
   * Returns the dataset located at the position pointed by the iterator.
   * The iterator does not need to be iterating over this dataset itself. It can
   * be an iterator for composite dataset with similar structure (achieved by
   * using CopyStructure).
   */
  vtkDataObject* GetDataSet(vtkCompositeDataIterator* iter) override;

  /**
   * Returns the meta-data associated with the position pointed by the iterator.
   * This will create a new vtkInformation object if none already exists. Use
   * HasMetaData to avoid creating the vtkInformation object unnecessarily.
   * The iterator does not need to be iterating over this dataset itself. It can
   * be an iterator for composite dataset with similar structure (achieved by
   * using CopyStructure).
   */
  virtual vtkInformation* GetMetaData(vtkCompositeDataIterator* iter);

  /**
   * Returns if any meta-data associated with the position pointed by the iterator.
   * The iterator does not need to be iterating over this dataset itself. It can
   * be an iterator for composite dataset with similar structure (achieved by
   * using CopyStructure).
   */
  virtual vtkTypeBool HasMetaData(vtkCompositeDataIterator* iter);

  /**
   * Return the actual size of the data in kibibytes (1024 bytes). This number
   * is valid only after the pipeline has updated.
   */
  unsigned long GetActualMemorySize() override;

  /**
   * Restore data object to initial state,
   */
  void Initialize() override;

  ///@{
  /**
   * CompositeShallow, Shallow and Deep copy.
   */
  void CompositeShallowCopy(vtkCompositeDataSet* src) override;
  void ShallowCopy(vtkDataObject* src) override;
  void DeepCopy(vtkDataObject* src) override;
  ///@}

  /**
   * Returns the total number of points of all blocks. This will
   * iterate over all blocks and call GetNumberOfPoints() so it
   * might be expansive.
   */
  vtkIdType GetNumberOfPoints() override;

  /**
   * Returns the total number of cells of all blocks. This will
   * iterate over all blocks and call GetNumberOfPoints() so it
   * might be expensive.
   */
  vtkIdType GetNumberOfCells() override;

  /**
   * Get the number of children.
   */
  unsigned int GetNumberOfChildren();

  /**
   * Returns a child dataset at a given index.
   */
  vtkDataObject* GetChild(unsigned int index);

  /**
   * Returns the meta-data at a given index. If the index is valid, however, no
   * information object is set, then a new one will created and returned.
   * To avoid unnecessary creation, use HasMetaData().
   */
  vtkInformation* GetChildMetaData(unsigned int index);

  /**
   * Returns if meta-data information is available for the given child index.
   * Returns 1 is present, 0 otherwise.
   */
  vtkTypeBool HasChildMetaData(unsigned int index);

  ///@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  static vtkDataObjectTree* GetData(vtkInformation* info);
  static vtkDataObjectTree* GetData(vtkInformationVector* v, int i = 0);
  ///@}

  /**
   * Overridden to return `VTK_DATA_OBJECT_TREE`.
   */
  int GetDataObjectType() VTK_FUTURE_CONST override { return VTK_DATA_OBJECT_TREE; }

protected:
  vtkDataObjectTree();
  ~vtkDataObjectTree() override;

  /**
   * Set the number of children.
   */
  void SetNumberOfChildren(unsigned int num);

  /**
   * Set child dataset at a given index. The number of children is adjusted to
   * to be greater than the index specified.
   */
  void SetChild(unsigned int index, vtkDataObject*);

  /**
   * Remove the child at a given index.
   */
  void RemoveChild(unsigned int index);

  /**
   * Sets the meta-data at a given index.
   */
  void SetChildMetaData(unsigned int index, vtkInformation* info);

  /**
   * When copying structure from another vtkDataObjectTree, this method gets
   * called for create a new non-leaf for the `other` node. Subclasses can
   * override this to create a different type of vtkDataObjectTree subclass, if
   * appropriate. Default implementation, simply calls `NewInstance` on other;
   */
  virtual vtkDataObjectTree* CreateForCopyStructure(vtkDataObjectTree* other);

  // The internal datastructure. Subclasses need not access this directly.
  vtkDataObjectTreeInternals* Internals;

  friend class vtkDataObjectTreeIterator;

private:
  vtkDataObjectTree(const vtkDataObjectTree&) = delete;
  void operator=(const vtkDataObjectTree&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
