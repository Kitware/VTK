// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAMRDataObject
 * @brief   a multi-resolution dataset based on vtkCartesianGrid
 *
 * vtkAMRDataObject (AMR stands for Adaptive Mesh Refinement)
 * is a container for vtkCartesianGrid. Each grid is added as a block of a given level.
 *
 * Supplemental information are stored in the AMRMetaData.
 *
 * The AMR is stored as a vtkPartitionedDataSetCollection, where each AMR Level is a
 * vtkPartitionedDataSet
 * eg:
 * - root
 *  - level 0 (pds)
 *   - level 0, index 0
 *  - level 1 (pds)
 *   - level 1, index 0
 *   - level 1, index 1
 *  - level 2 (pds)
 *   - level 2, index 0
 *   - level 2, index 1
 *   - level 2, index 2
 *   - level 2, index 3
 *
 * The AMR metadata is used to reconstruct the level and index of the AMR when needed.
 *
 * vtkAMRDataObject support both vtkRectilinearGrid and vtkImageData (vtkUniformGrid)
 * when using AddDataSet which can be mixed.
 *
 * vtkAMRDataObject is an interface class and user should instanciate either vtkNonOverlappingAMR
 * or vtkOverlappingAMR depending on their usecases.
 *
 * Contrary to vtkNonOverlappingAMR, vtkOverlappingAMR does not support mixed types.
 *
 * @sa
 * vtkOverlappingAMR, vtkNonOverlappingAMR, vtkOverlappingAMRMetaData, vtkAMRIterator,
 * vtkPartitioneDataSetCollection, vtkPartitionedDataSet
 */

#ifndef vtkAMRDataObject_h
#define vtkAMRDataObject_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNew.h"                   // for vtkNew
#include "vtkPartitionedDataSetCollection.h"
#include "vtkSmartPointer.h" // for vtkSmartPointer

VTK_ABI_NAMESPACE_BEGIN
class vtkAMRMetaData;
class vtkCartesianGrid;
class vtkCompositeDataIterator;
class vtkDataSet;
class vtkImageData;
class vtkRectilinearGrid;
class vtkUniformGrid;
class VTKCOMMONDATAMODEL_EXPORT vtkAMRDataObject : public vtkPartitionedDataSetCollection
{
public:
  static vtkAMRDataObject* New();
  vtkTypeMacro(vtkAMRDataObject, vtkPartitionedDataSetCollection);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return a new vtkAMRIterator (the iterator has to be deleted by the user).
   */
  VTK_NEWINSTANCE vtkCompositeDataIterator* NewIterator() override;

  /**
   * Return class name of data type (see vtkType.h for definitions).
   */
  int GetDataObjectType() VTK_FUTURE_CONST override { return VTK_AMR_DATA_OBJECT; }

  /**
   * Restore data object to initial state.
   */
  void Initialize() override;

  /**
   * Initialize the AMRMetaData and the AMR with the specified blocksPerLevel
   */
  virtual void Initialize(const std::vector<unsigned int>& blocksPerLevel);

  /**
   * Initialize AMR using the provided metadata by reconstructing the blocksPerLevel
   */
  virtual void Initialize(vtkAMRMetaData* metadata);

  ///@{
  /**
   * Set/Get the data description of this uniform grid instance,
   * e.g. VTK_STRUCTURED_XYZ_GRID
   */
  void SetGridDescription(int gridDescription);
  int GetGridDescription();
  ///@}

  /**
   * Get number of levels. Forward to the internal AMRMetaData.
   * Return 0 if metadata is invalid.
   */
  [[nodiscard]] unsigned int GetNumberOfLevels() const;

  /**
   * Get the number of blocks for all levels including nullptr blocks.
   * Forward to the internal AMRMetaData.
   * Returns 0 if AMRMetaData is invalid.
   */
  [[nodiscard]] unsigned int GetNumberOfBlocks() const;

  /**
   * Get the number of block at the given level plus this AMR current level
   * Returns 0 if AMRMetaData is invalid.
   */
  [[nodiscard]] unsigned int GetNumberOfBlocks(unsigned int level) const;

  ///@{
  /**
   * Get the (min/max) bounds of the AMR domain.
   */
  void GetBounds(double bounds[6]);
  virtual const double* GetBounds();
  void GetMin(double min[3]);
  void GetMax(double max[3]);
  ///@}

  using Superclass::SetDataSet;
  /**
   * At the passed in level, set grid as the idx'th block at that level. idx must be less
   * than the number of data sets at that level
   */
  virtual void SetDataSet(unsigned int level, unsigned int idx, vtkDataSet* grid);

  /**
   * Get the data set as a cartesian grid using the (level, index) pair.
   */
  vtkCartesianGrid* GetDataSetAsCartesianGrid(unsigned int level, unsigned int idx);

  /**
   * Get the data set as an image data using the (level, index) pair.
   */
  vtkImageData* GetDataSetAsImageData(unsigned int level, unsigned int idx);

  /**
   * Get the data set as a rectilinear grid using the (level, index) pair.
   */
  vtkRectilinearGrid* GetDataSetAsRectilinearGrid(unsigned int level, unsigned int idx);

  /**
   * Returns the absolute block index for given level plus this AMR current level
   * and a relative block index or -1 if it doesn't exist or AMRMetaData is invalid.
   * Forward to the internal vtkAMRMetaData.
   */
  [[nodiscard]] int GetAbsoluteBlockIndex(unsigned int level, unsigned int index) const;

  /**
   * Returns the an index pair (level, relative index) given a absolute block index
   * Forward to the internal vtkAMRMetaData.
   */
  void ComputeIndexPair(unsigned int index, unsigned int& level, unsigned int& id);

  ///@{
  /**
   * ShallowCopy.
   */
  void CompositeShallowCopy(vtkCompositeDataSet* src) override;
  void ShallowCopy(vtkDataObject* src) override;
  ///@}

  /**
   * DeepCopy.
   */
  void DeepCopy(vtkDataObject* src) override;

  /**
   * CopyStructure.
   */
  void CopyStructure(vtkCompositeDataSet* src) override;

  ///@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  static vtkAMRDataObject* GetData(vtkInformation* info);
  static vtkAMRDataObject* GetData(vtkInformationVector* v, int i = 0);
  ///@}

  ///@{
  /**
   * Get/Set the AMR meta data
   */
  [[nodiscard]] vtkAMRMetaData* GetAMRMetaData();
  void SetAMRMetaData(vtkAMRMetaData* metadata);
  ///@}

  ///@{
  /**
   * Overrides that call SetDataSet and GetDataSetAsCartesianGrid under the hood.
   */
  void SetPartition(unsigned int idx, unsigned int partition, vtkDataObject* object) override;
  vtkDataSet* GetPartition(unsigned int idx, unsigned int partition) override;
  ///@}

protected:
  vtkAMRDataObject();
  ~vtkAMRDataObject() override;

  /**
   * Create and set a new vtkAMRMetaData as AMRMetaData
   */
  virtual void InstantiateMetaData();

private:
  vtkAMRDataObject(const vtkAMRDataObject&) = delete;
  void operator=(const vtkAMRDataObject&) = delete;

  void InitializeInternal();

  double Bounds[6] = {
    VTK_DOUBLE_MAX,
    VTK_DOUBLE_MIN,
    VTK_DOUBLE_MAX,
    VTK_DOUBLE_MIN,
    VTK_DOUBLE_MAX,
    VTK_DOUBLE_MIN,
  };
  vtkSmartPointer<vtkAMRMetaData> AMRMetaData;
};

VTK_ABI_NAMESPACE_END
#endif
