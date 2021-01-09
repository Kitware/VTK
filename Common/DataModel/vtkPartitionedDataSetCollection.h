/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPartitionedDataSetCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPartitionedDataSetCollection
 * @brief   Composite dataset that groups datasets as a collection.
 *
 * vtkPartitionedDataSetCollection is a vtkCompositeDataSet that stores
 * a collection of non-null vtkPartitionedDataSets. These items can represent
 * different concepts depending on the context. For example, they can
 * represent region of different materials in a simulation or parts in
 * an assembly. It is not requires that items have anything in common.
 * For example, they can have completely different point or cell arrays.
 */

#ifndef vtkPartitionedDataSetCollection_h
#define vtkPartitionedDataSetCollection_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataObjectTree.h"

class vtkPartitionedDataSet;
class vtkDataAssembly;
class vtkDataSet;

class VTKCOMMONDATAMODEL_EXPORT vtkPartitionedDataSetCollection : public vtkDataObjectTree
{
public:
  static vtkPartitionedDataSetCollection* New();
  vtkTypeMacro(vtkPartitionedDataSetCollection, vtkDataObjectTree);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return class name of data type (see vtkType.h for
   * definitions).
   */
  int GetDataObjectType() override { return VTK_PARTITIONED_DATA_SET_COLLECTION; }

  /**
   * Set the number of blocks. This will cause allocation if the new number of
   * blocks is greater than the current size. All new blocks are initialized to
   * with empty `vtkPartitionedDataSetCollection` instances.
   */
  void SetNumberOfPartitionedDataSets(unsigned int numDataSets);

  /**
   * Returns the number of blocks.
   */
  unsigned int GetNumberOfPartitionedDataSets();

  /**
   * Returns the block at the given index. It is recommended that one uses the
   * iterators to iterate over composite datasets rather than using this API.
   */
  vtkPartitionedDataSet* GetPartitionedDataSet(unsigned int idx);

  /**
   * Sets the data object as the given block. The total number of blocks will
   * be resized to fit the requested block no.
   *
   * @remark `dataset` cannot be nullptr.
   */
  void SetPartitionedDataSet(unsigned int idx, vtkPartitionedDataSet* dataset);

  /**
   * Remove the given block from the dataset.
   */
  void RemovePartitionedDataSet(unsigned int idx);

  //@{
  /**
   * API to get/set partitions using a tuple index.
   */
  void SetPartition(unsigned int idx, unsigned int partition, vtkDataObject* object);
  vtkDataSet* GetPartition(unsigned int idx, unsigned int partition);
  vtkDataObject* GetPartitionAsDataObject(unsigned int idx, unsigned int partition);
  //@}

  /**
   * Returns the number of partitions in a partitioned dataset at the given index.
   */
  unsigned int GetNumberOfPartitions(unsigned int idx);

  /**
   * Set number of partitions at a given index. Note, this will call
   * `SetNumberOfPartitionedDataSets` if needed to grow the collection.
   */
  void SetNumberOfPartitions(unsigned int idx, unsigned int numPartitions);

  /**
   * Returns true if meta-data is available for a given block.
   */
  int HasMetaData(unsigned int idx) { return this->Superclass::HasChildMetaData(idx); }

  /**
   * Returns the meta-data for the block. If none is already present, a new
   * vtkInformation object will be allocated. Use HasMetaData to avoid
   * allocating vtkInformation objects.
   */
  vtkInformation* GetMetaData(unsigned int idx) { return this->Superclass::GetChildMetaData(idx); }

  //@{
  /**
   * DataAssembly provides a way to define hierarchical organization of
   * partitioned-datasets. These methods provide access to the data assembly
   * instances associated, if any.
   */
  vtkGetObjectMacro(DataAssembly, vtkDataAssembly);
  void SetDataAssembly(vtkDataAssembly* assembly);
  //@}

  //@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  static vtkPartitionedDataSetCollection* GetData(vtkInformation* info);
  static vtkPartitionedDataSetCollection* GetData(vtkInformationVector* v, int i = 0);
  //@}

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
  int HasMetaData(vtkCompositeDataIterator* iter) override
  {
    return this->Superclass::HasMetaData(iter);
  }

  /**
   * Overridden to include DataAssembly MTime.
   */
  vtkMTimeType GetMTime() override;

  //@{
  /**
   * Overridden to handle vtkDataAssembly.
   */
  void ShallowCopy(vtkDataObject* src) override;
  void DeepCopy(vtkDataObject* src) override;
  void CopyStructure(vtkCompositeDataSet* input) override;
  void Initialize() override;
  //@}
protected:
  vtkPartitionedDataSetCollection();
  ~vtkPartitionedDataSetCollection() override;

private:
  vtkPartitionedDataSetCollection(const vtkPartitionedDataSetCollection&) = delete;
  void operator=(const vtkPartitionedDataSetCollection&) = delete;

  vtkDataAssembly* DataAssembly;
};

#endif
