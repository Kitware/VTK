// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkUniformGridAMR
 * @brief   a multi-resolution dataset based on vtkUniformGrid
 *
 * vtkUniformGridAMR (AMR stands for Adaptive Mesh Refinement)
 * is a container for vtkUniformGrid. Each grid is added as a block of a given level.
 *
 * The structure of the container is described in a vtkOverlappingAMRMetaData object.
 *
 * @sa
 * vtkOverlappingAMR, vtkNonOverlappingAMR, vtkOverlappingAMRMetaData, vtkUniformGridAMRDataIterator
 */

#ifndef vtkUniformGridAMR_h
#define vtkUniformGridAMR_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCompositeDataSet.h"
#include "vtkNew.h" // for vtkNew

VTK_ABI_NAMESPACE_BEGIN
class vtkCompositeDataIterator;
class vtkUniformGrid;
class vtkOverlappingAMRMetaData;
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
   * Initialize the AMR with a specified number of levels and the blocks per level.
   */
  virtual void Initialize(int numLevels, const int* blocksPerLevel);

  /**
   * Set/Get the data description of this uniform grid instance,
   * e.g. VTK_XYZ_GRID
   */
  void SetGridDescription(int gridDescription);
  int GetGridDescription();

  /**
   * Get number of levels.
   */
  unsigned int GetNumberOfLevels();

  /**
   * Get the total number of blocks, including nullptr blocks
   */
  virtual unsigned int GetTotalNumberOfBlocks();

  /**
   * Get the number of datasets at the given level, including null blocks
   */
  unsigned int GetNumberOfDataSets(unsigned int level);

  ///@{
  /**
   * Get the (min/max) bounds of the AMR domain.
   */
  void GetBounds(double bounds[6]);
  const double* GetBounds();
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
   * Retrieves the composite index associated with the data at the given
   * (level,index) pair.
   */
  int GetCompositeIndex(unsigned int level, unsigned int index);

  /**
   * Given the compositeIdx (as set by SetCompositeIdx) this method returns the
   * corresponding level and dataset index within the level.
   */
  void GetLevelAndIndex(unsigned int compositeIdx, unsigned int& level, unsigned int& idx);

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

protected:
  vtkUniformGridAMR();
  ~vtkUniformGridAMR() override;

  double Bounds[6];

  ///@{
  /**
   * Get/Set the meta AMR meta data
   * Deprecated, do not use.
   */
  VTK_DEPRECATED_IN_9_6_0("This function is deprecated and should not be used")
  virtual vtkAMRDataInternals* GetAMRData() { return this->AMRData; }
  VTK_DEPRECATED_IN_9_6_0("This function is deprecated and has no effect")
  virtual void SetAMRData(vtkAMRDataInternals*){};
  ///@}

  ///@{
  /**
   * Get/Set the meta AMR meta info
   */
  vtkGetObjectMacro(AMRInfo, vtkOverlappingAMRMetaData);
  virtual void SetAMRInfo(vtkOverlappingAMRMetaData*);
  ///@}

  vtkOverlappingAMRMetaData* AMRInfo;

private:
  vtkUniformGridAMR(const vtkUniformGridAMR&) = delete;
  void operator=(const vtkUniformGridAMR&) = delete;

  friend class vtkUniformGridAMRDataIterator;

  vtkNew<vtkAMRDataInternals> AMRData;
};

VTK_ABI_NAMESPACE_END
#endif
