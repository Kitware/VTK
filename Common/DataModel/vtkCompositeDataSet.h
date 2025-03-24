// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCompositeDataSet
 * @brief   abstract superclass for composite
 * (multi-block or AMR) datasets
 *
 * vtkCompositeDataSet is an abstract class that represents a collection
 * of datasets (including other composite datasets). It
 * provides an interface to access the datasets through iterators.
 * vtkCompositeDataSet provides methods that are used by subclasses to store the
 * datasets.
 * vtkCompositeDataSet provides the datastructure for a full tree
 * representation. Subclasses provide the semantics for it and control how
 * this tree is built.
 *
 * @sa
 * vtkCompositeDataIterator
 */

#ifndef vtkCompositeDataSet_h
#define vtkCompositeDataSet_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataObject.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

#include <vector> // For GetDataSets

VTK_ABI_NAMESPACE_BEGIN
class vtkCompositeDataIterator;
class vtkCompositeDataSetInternals;
class vtkDataSet;
class vtkInformation;
class vtkInformationStringKey;
class vtkInformationIntegerKey;

class VTKCOMMONDATAMODEL_EXPORT VTK_MARSHALAUTO vtkCompositeDataSet : public vtkDataObject
{
public:
  vtkTypeMacro(vtkCompositeDataSet, vtkDataObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return a new iterator (the iterator has to be deleted by user).
   */
  virtual VTK_NEWINSTANCE vtkCompositeDataIterator* NewIterator() = 0;

  /**
   * Return class name of data type (see vtkType.h for
   * definitions).
   */
  int GetDataObjectType() VTK_FUTURE_CONST override { return VTK_COMPOSITE_DATA_SET; }

  /**
   * Copies the tree structure from the input. All pointers to non-composite
   * data objects are initialized to nullptr. This also shallow copies the meta data
   * associated with all the nodes.
   */
  virtual void CopyStructure(vtkCompositeDataSet* input);

  /**
   * Sets the data set at the location pointed by the iterator.
   * The iterator does not need to be iterating over this dataset itself. It can
   * be any composite dataset with similar structure (achieved by using
   * CopyStructure).
   */
  virtual void SetDataSet(vtkCompositeDataIterator* iter, vtkDataObject* dataObj) = 0;

  /**
   * Returns the dataset located at the position pointed by the iterator.
   * The iterator does not need to be iterating over this dataset itself. It can
   * be an iterator for composite dataset with similar structure (achieved by
   * using CopyStructure).
   */
  virtual vtkDataObject* GetDataSet(vtkCompositeDataIterator* iter) = 0;

  /**
   * Returns the dataset located at the position pointed by the flatIndex.
   * If no dataset has the same flat index, nullptr is returned.
   *
   * It should be noted that this function should be used ONLY when you already know the flat index.
   * It should NOT be used when you are iterating over the composite dataset (in that case, prefer
   * the vtkCompositeDataIterator).
   */
  virtual vtkDataObject* GetDataSet(unsigned int flatIndex);

  /**
   * Return the actual size of the data in kibibytes (1024 bytes). This number
   * is valid only after the pipeline has updated.
   */
  unsigned long GetActualMemorySize() override;

  ///@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  static vtkCompositeDataSet* GetData(vtkInformation* info);
  static vtkCompositeDataSet* GetData(vtkInformationVector* v, int i = 0);
  ///@}

  /**
   * Restore data object to initial state,
   */
  void Initialize() override;

  /**
   * The goal of the method is to copy the data up to the dataset pointers only.
   * The implementation is delegated to the differenent subclasses.
   * If you want to copy up to array pointers, @see vtkDataObject::ShallowCopy.
   *
   * This method just calls vtkDataObject::ShallowCopy.
   */
  virtual void CompositeShallowCopy(vtkCompositeDataSet* src);

  /**
   * Returns the total number of points of all blocks. This will
   * iterate over all blocks and call GetNumberOfPoints() so it
   * might be expensive.
   */
  virtual vtkIdType GetNumberOfPoints();

  /**
   * Returns the total number of cells of all blocks. This will
   * iterate over all blocks and call GetNumberOfPoints() so it
   * might be expensive.
   */
  virtual vtkIdType GetNumberOfCells();

  /**
   * Get the number of elements for a specific attribute type (POINT, CELL, etc.).
   */
  vtkIdType GetNumberOfElements(int type) override;

  /**
   * Return the geometric bounding box in the form (xmin,xmax, ymin,ymax,
   * zmin,zmax).  Note that if the composite dataset contains abstract types
   * (i.e., non vtkDataSet types) such as tables these will be ignored by the
   * method. In cases where no vtkDataSet is contained in the composite
   * dataset then the returned bounds will be undefined. THIS METHOD IS
   * THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND THE DATASET IS NOT
   * MODIFIED.
   */
  void GetBounds(double bounds[6]);

  /**
   * Key used to put node name in the meta-data associated with a node.
   */
  static vtkInformationStringKey* NAME();

  /**
   * Key used to indicate that the current process can load the data
   * in the node.  Used for parallel readers where the nodes are assigned
   * to the processes by the reader to indicate further down the pipeline
   * which nodes will be on which processes.
   * ***THIS IS AN EXPERIMENTAL KEY SUBJECT TO CHANGE WITHOUT NOTICE***
   */
  static vtkInformationIntegerKey* CURRENT_PROCESS_CAN_LOAD_BLOCK();

  /**
   * Extract datasets from the given data object. This method returns a vector
   * of DataSetT* from the `dobj`. If dobj is a DataSetT, the returned
   * vector will have just 1 DataSetT. If dobj is a vtkCompositeDataSet, then
   * we iterate over it and add all non-null leaf nodes to the returned vector.
   *
   * If `preserveNull` is true (defaults to false), then `nullptr` place holders
   * are added as placeholders when leaf node dataset type does not match the
   * requested or is nullptr to begin with.
   */
  template <class DataSetT = vtkDataSet>
  static std::vector<DataSetT*> GetDataSets(vtkDataObject* dobj, bool preserveNull = false);

  /**
   * Returns true for POINT or CELL, false otherwise
   */
  bool SupportsGhostArray(int type) override;

protected:
  vtkCompositeDataSet();
  ~vtkCompositeDataSet() override;

private:
  vtkCompositeDataSet(const vtkCompositeDataSet&) = delete;
  void operator=(const vtkCompositeDataSet&) = delete;
};

VTK_ABI_NAMESPACE_END
#include "vtkCompositeDataSet.txx" // for template implementations

#endif
