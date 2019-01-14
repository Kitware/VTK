/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiBlockDataSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

class VTKCOMMONDATAMODEL_EXPORT vtkMultiBlockDataSet : public vtkDataObjectTree
{
public:
  static vtkMultiBlockDataSet* New();
  vtkTypeMacro(vtkMultiBlockDataSet, vtkDataObjectTree);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return class name of data type (see vtkType.h for
   * definitions).
   */
  int GetDataObjectType() override {return VTK_MULTIBLOCK_DATA_SET;}

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
   */
  void SetBlock(unsigned int blockno, vtkDataObject* block);

  /**
   * Remove the given block from the dataset.
   */
  void RemoveBlock(unsigned int blockno);

  /**
   * Returns true if meta-data is available for a given block.
   */
  int HasMetaData(unsigned int blockno)
    { return this->Superclass::HasChildMetaData(blockno); }

  /**
   * Returns the meta-data for the block. If none is already present, a new
   * vtkInformation object will be allocated. Use HasMetaData to avoid
   * allocating vtkInformation objects.
   */
  vtkInformation* GetMetaData(unsigned int blockno)
    { return this->Superclass::GetChildMetaData(blockno); }

  //@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  static vtkMultiBlockDataSet* GetData(vtkInformation* info);
  static vtkMultiBlockDataSet* GetData(vtkInformationVector* v, int i=0);
  //@}

  /**
   * Unhiding superclass method.
   */
  vtkInformation* GetMetaData(vtkCompositeDataIterator* iter) override
    { return this->Superclass::GetMetaData(iter); }

  /**
   * Unhiding superclass method.
   */
  int HasMetaData(vtkCompositeDataIterator* iter) override
    { return this->Superclass::HasMetaData(iter); }

protected:
  vtkMultiBlockDataSet();
  ~vtkMultiBlockDataSet() override;

private:
  vtkMultiBlockDataSet(const vtkMultiBlockDataSet&) = delete;
  void operator=(const vtkMultiBlockDataSet&) = delete;

};

#endif


