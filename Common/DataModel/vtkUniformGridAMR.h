// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkUniformGridAMR
 * @brief   a multi-resolution dataset based on vtkUniformGrid
 *
 * vtkUniformGridAMR (AMR stands for Adaptive Mesh Refinement)
 * is a container for vtkUniformGrid. Each grid is added as a block of a given level.
 *
 * Supplemental information are stored in the AMRMetaData.
 *
 * The AMR is stored as a vtkPartitionedDataSetCollection, where each AMR Level is a
 vtkPartitionedDataSet
 * eg:
 * - root
 *  - level 0 (pds)
     - level 0, index 0
    - level 1 (pds)
 *   - level 1, index 0
 *   - level 1, index 1
    - level 2 (pds)
 *   - level 2, index 0
 *   - level 2, index 1
 *   - level 2, index 2
 *   - level 2, index 3
 *
 * The AMR metadata is used to reconstruct the level and index of the AMR when needed.
 *
 * @sa
 * vtkOverlappingAMR, vtkNonOverlappingAMR, vtkOverlappingAMRMetaData, vtkUniformGridAMRIterator,
 * vtkPartitioneDataSetCollection, vtkPartitionedDataSet
 */

#ifndef vtkUniformGridAMR_h
#define vtkUniformGridAMR_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDeprecation.h"           // for VTK_DEPRECATED_IN_9_6_0
#include "vtkNew.h"                   // for vtkNew
#include "vtkPartitionedDataSetCollection.h"
#include "vtkSmartPointer.h" // for vtkSmartPointer

VTK_ABI_NAMESPACE_BEGIN
class vtkAMRDataInternals; // VTK_DEPRECATED_IN_9_6_0
class vtkAMRMetaData;
class vtkCompositeDataIterator;
class vtkOverlappingAMRMetaData; // VTK_DEPRECATED_IN_9_6_0
class vtkUniformGrid;

class VTKCOMMONDATAMODEL_EXPORT vtkUniformGridAMR : public vtkPartitionedDataSetCollection
{
public:
  static vtkUniformGridAMR* New();
  vtkTypeMacro(vtkUniformGridAMR, vtkPartitionedDataSetCollection);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return a new vtkUniformGridAMRIterator (the iterator has to be deleted by the user).
   */
  VTK_NEWINSTANCE vtkCompositeDataIterator* NewIterator() override;

  /**
   * Return class name of data type (see vtkType.h for definitions).
   */
  int GetDataObjectType() VTK_FUTURE_CONST override { return VTK_UNIFORM_GRID_AMR; }

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

  /**
   * Initialize the AMR with a specified number of levels and the blocks per level.
   */
  VTK_DEPRECATED_IN_9_6_0("Use Initialize(const std::vector<unsigned int>&) instead")
  virtual void Initialize(int numLevels, const int* blocksPerLevel);

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
   * Deprecated, forward to GetNumberOfBlocks
   */
  VTK_DEPRECATED_IN_9_6_0("Use GetNumberOfBlocks instead")
  virtual unsigned int GetTotalNumberOfBlocks() { return this->GetNumberOfBlocks(); }

  /**
   * Get the number of block at the given level plus this AMR current level
   * Returns 0 if AMRMetaData is invalid.
   */
  [[nodiscard]] unsigned int GetNumberOfBlocks(unsigned int level) const;

  /**
   * Deprecated, forward to GetNumberOfBlocks(level)
   */
  VTK_DEPRECATED_IN_9_6_0("Use GetNumberOfBlocks(level) instead")
  unsigned int GetNumberOfDataSets(unsigned int level) { return this->GetNumberOfBlocks(level); }

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
  virtual void SetDataSet(unsigned int level, unsigned int idx, vtkUniformGrid* grid);

  using Superclass::GetDataSet;
  /**
   * Get the data set using the (level, index) pair.
   */
  vtkUniformGrid* GetDataSet(unsigned int level, unsigned int idx);

  /**
   * Returns the absolute block index for given level plus this AMR current level
   * and a relative block index or -1 if it doesn't exist or AMRMetaData is invalid.
   * Forward to the internal vtkAMRMetaData.
   */
  [[nodiscard]] int GetAbsoluteBlockIndex(unsigned int level, unsigned int index) const;

  /**
   * Forward to the internal GetAbsoluteBlockIndex
   * Deprecated, use GetAbsoluteBlockIndex instead.
   */
  VTK_DEPRECATED_IN_9_6_0("This function is deprecated, use GetAbsoluteBlockIndex() instead")
  int GetCompositeIndex(unsigned int level, unsigned int index)
  {
    return this->GetAbsoluteBlockIndex(level, index);
  }

  /**
   * Returns the an index pair (level, relative index) given a absolute block index
   * Forward to the internal vtkAMRMetaData.
   */
  void ComputeIndexPair(unsigned int index, unsigned int& level, unsigned int& id);

  /**
   * Returns the an index pair (level, relative index) given a absolute block index
   * Forward to the ComputeIndexPair.
   * Deprecated, using ComputeIndexPair.
   */
  VTK_DEPRECATED_IN_9_6_0("This function is deprecated, use ComputeIndexPair() instead")
  void GetLevelAndIndex(unsigned int compositeIdx, unsigned int& level, unsigned int& idx)
  {
    this->ComputeIndexPair(compositeIdx, level, idx);
  }

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
  static vtkUniformGridAMR* GetData(vtkInformation* info);
  static vtkUniformGridAMR* GetData(vtkInformationVector* v, int i = 0);
  ///@}

  ///@{
  /**
   * Get/Set the AMR meta data
   */
  [[nodiscard]] vtkAMRMetaData* GetAMRMetaData();
  void SetAMRMetaData(vtkAMRMetaData* metadata);
  ///@}

protected:
  vtkUniformGridAMR();
  ~vtkUniformGridAMR() override;

  /**
   * Create and set a new vtkAMRMetaData as AMRMetaData
   */
  virtual void InstantiateMetaData();

  ///@{
  /**
   * Get/Set the meta AMR data
   * Deprecated, do not use.
   */
  VTK_DEPRECATED_IN_9_6_0("This function is deprecated and should not be used, returns nullptr")
  virtual vtkAMRDataInternals* GetAMRData() { return nullptr; }
  VTK_DEPRECATED_IN_9_6_0("This function is deprecated and has no effect")
  virtual void SetAMRData(vtkAMRDataInternals*){};
  ///@}

  ///@{
  /**
   * Noop and deprecated, use GetAMRData/SetAMRMetaData instead
   */
  VTK_DEPRECATED_IN_9_6_0(
    "This function is deprecated and should not be inherited, use GetAMRMetaData() instead")
  virtual vtkOverlappingAMRMetaData* GetAMRInfo() { return nullptr; };
  VTK_DEPRECATED_IN_9_6_0("This function is deprecated and should not be inherited, use "
                          "SetAMRMetaData() or Initialize(vtkAMRMetaData*) instead")
  virtual void SetAMRInfo(vtkOverlappingAMRMetaData*){};
  ///@}

private:
  vtkUniformGridAMR(const vtkUniformGridAMR&) = delete;
  void operator=(const vtkUniformGridAMR&) = delete;

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
