// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMultiBlockDataSet
 * @brief   Composite dataset that organizes datasets into
 * blocks.
 *
 * vtkMultiBlockDataSet is a vtkCompositeDataSet that stores
 * a hierarchy of datasets. The dataset collection consists of
 * multiple blocks. Each block can itself be a vtkMultiBlockDataSet, thus
 * providing for a full tree structure.
 * Sub-blocks are usually used to distribute blocks across processors.
 * For example, a 1 block dataset can be distributed as following:
 * @verbatim
 * proc 0:
 * Block 0:
 *   * ds 0
 *   * (null)
 *
 * proc 1:
 * Block 0:
 *   * (null)
 *   * ds 1
 * @endverbatim
 */

#ifndef vtkMultiBlockDataSet_h
#define vtkMultiBlockDataSet_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataObjectTree.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALMANUAL

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONDATAMODEL_EXPORT VTK_MARSHALMANUAL vtkMultiBlockDataSet : public vtkDataObjectTree
{
public:
  static vtkMultiBlockDataSet* New();
  vtkTypeMacro(vtkMultiBlockDataSet, vtkDataObjectTree);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return class name of data type (see vtkType.h for
   * definitions).
   */
  int GetDataObjectType() VTK_FUTURE_CONST override { return VTK_MULTIBLOCK_DATA_SET; }

  /**
   * Set the number of blocks. This will cause allocation if the new number of
   * blocks is greater than the current size. All new blocks are initialized to
   * null.
   */
  void SetNumberOfBlocks(unsigned int numBlocks);

  /**
   * Returns the number of blocks.
   */
  unsigned int GetNumberOfBlocks();

  /**
   * Returns the block at the given index. It is recommended that one uses the
   * iterators to iterate over composite datasets rather than using this API.
   */
  vtkDataObject* GetBlock(unsigned int blockno);

  /**
   * Sets the data object as the given block. The total number of blocks will
   * be resized to fit the requested block no.
   *
   * @remark while most vtkDataObject subclasses, including vtkMultiBlockDataSet
   * as acceptable as a block, `vtkPartitionedDataSet`,
   * `vtkPartitionedDataSetCollection`, and `vtkUniformGridAMR`
   * are not valid.
   */
  void SetBlock(unsigned int blockno, vtkDataObject* block);

  /**
   * Remove the given block from the dataset.
   */
  void RemoveBlock(unsigned int blockno);

  /**
   * Returns true if meta-data is available for a given block.
   */
  vtkTypeBool HasMetaData(unsigned int blockno)
  {
    return this->Superclass::HasChildMetaData(blockno);
  }

  /**
   * Returns the meta-data for the block. If none is already present, a new
   * vtkInformation object will be allocated. Use HasMetaData to avoid
   * allocating vtkInformation objects.
   */
  vtkInformation* GetMetaData(unsigned int blockno)
  {
    return this->Superclass::GetChildMetaData(blockno);
  }

  ///@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  static vtkMultiBlockDataSet* GetData(vtkInformation* info);
  static vtkMultiBlockDataSet* GetData(vtkInformationVector* v, int i = 0);
  ///@}

  /**
   * Unhiding superclass method.
   */
  vtkInformation* GetMetaData(vtkCompositeDataIterator* iter) override
  {
    return this->Superclass::GetMetaData(iter);
  }

  /**
   * Unhiding superclass method.
   */
  vtkTypeBool HasMetaData(vtkCompositeDataIterator* iter) override
  {
    return this->Superclass::HasMetaData(iter);
  }

protected:
  vtkMultiBlockDataSet();
  ~vtkMultiBlockDataSet() override;

  /**
   * Overridden to create a vtkMultiPieceDataSet whenever a
   * vtkPartitionedDataSet is encountered. This is necessary since
   * vtkMultiBlockDataSet cannot contain vtPartitionedDataSets.
   */
  vtkDataObjectTree* CreateForCopyStructure(vtkDataObjectTree* other) override;

private:
  vtkMultiBlockDataSet(const vtkMultiBlockDataSet&) = delete;
  void operator=(const vtkMultiBlockDataSet&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
