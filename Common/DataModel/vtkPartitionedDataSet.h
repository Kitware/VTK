/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPartitionedDataSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPartitionedDataSet
 * @brief   composite dataset to encapsulates a dataset consisting of
 * partitions.
 *
 * A vtkPartitionedDataSet dataset groups multiple datasets together.
 * For example, say a simulation running in parallel on 16 processes
 * generated 16 datasets that when considering together form a whole
 * dataset. These are referred to as the partitions of the whole dataset.
 * Now imagine that we want to load a volume of 16 partitions in a
 * visualization cluster of 4 nodes. Each node could get 4 partitions,
 * not necessarily forming a whole rectangular region. In this case,
 * it is not possible to append the 4 partitions together into a vtkImageData.
 * We can then collect these 4 partitions together using a
 * vtkPartitionedDataSet.
 *
 * It is required that all non-empty partitions have the same arrays
 * and that they can be processed together as a whole by the same kind of
 * filter. However, it is not required that they are of the same type.
 * For example, it is possible to have structured datasets together with
 * unstructured datasets as long as they are compatible meshes (i.e. can
 * be processed together for the same kind of filter).
 */

#ifndef vtkPartitionedDataSet_h
#define vtkPartitionedDataSet_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataObjectTree.h"

class vtkDataSet;
class VTKCOMMONDATAMODEL_EXPORT vtkPartitionedDataSet : public vtkDataObjectTree
{
public:
  static vtkPartitionedDataSet* New();
  vtkTypeMacro(vtkPartitionedDataSet, vtkDataObjectTree);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return class name of data type (see vtkType.h for
   * definitions).
   */
  int GetDataObjectType() override {return VTK_PARTITIONED_DATA_SET;}

  /**
   * Set the number of partitionss. This will cause allocation if the new number of
   * partitions is greater than the current size. All new partitions are initialized to
   * null.
   */
  void SetNumberOfPartitions(unsigned int numPartitions);

  /**
   * Returns the number of partitions.
   */
  unsigned int GetNumberOfPartitions();

  //@{
  /**
   * Returns the partition at the given index.
   */
  vtkDataSet* GetPartition(unsigned int idx);
  vtkDataObject* GetPartitionAsDataObject(unsigned int idx);
  //@}

  /**
   * Sets the data object as the given partition. The total number of partitions will
   * be resized to fit the requested partition no.
   */
  void SetPartition(unsigned int idx, vtkDataObject* partition);


/**
 * Returns true if meta-data is available for a given partition.
 */
  int HasMetaData(unsigned int idx)
    { return this->Superclass::HasChildMetaData(idx); }

  /**
   * Returns the meta-data for the partition. If none is already present, a new
   * vtkInformation object will be allocated. Use HasMetaData to avoid
   * allocating vtkInformation objects.
   */
  vtkInformation* GetMetaData(unsigned int idx)
    { return this->Superclass::GetChildMetaData(idx); }

  //@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  static vtkPartitionedDataSet* GetData(vtkInformation* info);
  static vtkPartitionedDataSet* GetData(vtkInformationVector* v, int i=0);
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
  vtkPartitionedDataSet();
  ~vtkPartitionedDataSet() override;

private:
  vtkPartitionedDataSet(const vtkPartitionedDataSet&) = delete;
  void operator=(const vtkPartitionedDataSet&) = delete;

};

#endif
