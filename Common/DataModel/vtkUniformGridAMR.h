// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkUniformGridAMR
 * @brief   a multi-resolution dataset based on vtkUniformGrid
 *
 * vtkUniformGridAMR (AMR stands for Adaptive Mesh Refinement)
 * is a container for vtkUniformGrid. Each grid is added as a block of a given level.
 *
 * The structure of the container is described in a vtkAMRMetaData object.
 *
 * @sa
 * vtkOverlappingAMR, vtkNonOverlappingAMR, vtkOverlappingAMRMetaData, vtkUniformGridAMRDataIterator
 */

#ifndef vtkUniformGridAMR_h
#define vtkUniformGridAMR_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCompositeDataSet.h"
#include "vtkDeprecation.h"  // for VTK_DEPRECATED_IN_9_6_0
#include "vtkNew.h"          // for vtkNew
#include "vtkSmartPointer.h" // for vtkSmartPointer

VTK_ABI_NAMESPACE_BEGIN
class vtkCompositeDataIterator;
class vtkUniformGrid;
class vtkAMRMetaData;
class vtkOverlappingAMRMetaData; // VTK_DEPRECATED_IN_9_6_0
class vtkAMRDataInternals;

class VTKCOMMONDATAMODEL_EXPORT vtkUniformGridAMR : public vtkCompositeDataSet
{
public:
  static vtkUniformGridAMR* New();
  vtkTypeMacro(vtkUniformGridAMR, vtkCompositeDataSet);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return a new iterator (the iterator has to be deleted by the user).
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
   * Initialize the AMR with the specified blocksPerLevel
   */
  virtual void Initialize(const std::vector<unsigned int>& blocksPerLevel);

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
   * Get number of levels.
   */
  [[nodiscard]] unsigned int GetNumberOfLevels() const;

  /**
   * Get the number of blocks for all levels including nullptr blocks, or 0 if AMRMetaData is
   * invalid.
   */
  [[nodiscard]] unsigned int GetNumberOfBlocks() const;

  /**
   * Deprecated, forward to GetNumberOfBlocks
   */
  VTK_DEPRECATED_IN_9_6_0("Use GetNumberOfBlocks instead")
  virtual unsigned int GetTotalNumberOfBlocks() { return this->GetNumberOfBlocks(); }

  /**
   * Get the number of block at the given level, including nullptr blocks, or 0 if AMRMetaData
   * is invalid.
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

  /**
   * Overriding superclass method.
   */
  void SetDataSet(vtkCompositeDataIterator* iter, vtkDataObject* dataObj) override;

  /**
   * At the passed in level, set grid as the idx'th block at that level. idx must be less
   * than the number of data sets at that level
   */
  virtual void SetDataSet(unsigned int level, unsigned int idx, vtkUniformGrid* grid);

  // Needed because, otherwise vtkCompositeData::GetDataSet(unsigned int flatIndex) is hidden.
  using Superclass::GetDataSet;

  /**
   * Get the data set pointed to by iter
   */
  vtkDataObject* GetDataSet(vtkCompositeDataIterator* iter) override;

  /**
   * Get the data set using the (level, index) pair.
   */
  vtkUniformGrid* GetDataSet(unsigned int level, unsigned int idx);

  /**
   * Returns the absolute block index from a level and a relative block index
   * or -1 if it doesn't exist.
   * Forward to the internal vtkAMRMetaData.
   */
  [[nodiscard]] int GetAbsoluteBlockIndex(unsigned int level, unsigned int index) const;

  /**
   * Returns the absolute block index from a level and a relative block index
   * or -1 if it doesn't exist.
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
  VTK_DEPRECATED_IN_9_6_0("This function is deprecated and should not be used")
  virtual vtkAMRDataInternals* GetAMRData() { return this->AMRData; }
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
  VTK_DEPRECATED_IN_9_6_0(
    "This function is deprecated and should not be inherited, use SetAMRMetaData() instead")
  virtual void SetAMRInfo(vtkOverlappingAMRMetaData*){};
  ///@}

private:
  vtkUniformGridAMR(const vtkUniformGridAMR&) = delete;
  void operator=(const vtkUniformGridAMR&) = delete;

  friend class vtkUniformGridAMRDataIterator;

  double Bounds[6] = {
    VTK_DOUBLE_MAX,
    VTK_DOUBLE_MIN,
    VTK_DOUBLE_MAX,
    VTK_DOUBLE_MIN,
    VTK_DOUBLE_MAX,
    VTK_DOUBLE_MIN,
  };
  vtkNew<vtkAMRDataInternals> AMRData;
  vtkSmartPointer<vtkAMRMetaData> AMRMetaData;
};

VTK_ABI_NAMESPACE_END
#endif
