// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAMRMetaData
 * @brief   Meta data that describes the structure of a generic AMR data set
 *
 * vtkAMRInformation encapsulates the following meta information for a generic AMR data set
 * - The file block index for each block
 * - the Grid description
 *
 * @sa vtkUniformGridAMR, vtkOverlappingAMR, vtkAMRBox, vtkOverlappingAMRMetaData,
 * vtkNonOverlappingAMR
 */

#ifndef vtkAMRMetaData_h
#define vtkAMRMetaData_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDeprecation.h"           // for VTK_DEPRECATED_IN_9_6_0
#include "vtkObject.h"
#include "vtkSmartPointer.h"   // For vtkSmartPointer
#include "vtkStructuredData.h" // For VTK_STRUCTURED_INVALID

VTK_ABI_NAMESPACE_BEGIN

class vtkUnsignedIntArray;
class VTKCOMMONDATAMODEL_EXPORT vtkAMRMetaData : public vtkObject
{
public:
  static vtkAMRMetaData* New();
  vtkTypeMacro(vtkAMRMetaData, vtkObject);

  /**
   * Print members
   */
  void PrintSelf(ostream& os, vtkIndent indent) override;
  bool operator==(const vtkAMRMetaData& other) const;
  bool operator!=(const vtkAMRMetaData& other) const { return !(*this == other); }

  /**
   * Initialize the meta information
   * blocksPerLevel is the number of blocks for each levels
   */
  virtual void Initialize(const std::vector<unsigned int>& blocksPerLevel);

  /**
   * Initialize the meta information
   * numLevels is the number of levels
   * blocksPerLevel[i] is the number of blocks at level i
   */
  VTK_DEPRECATED_IN_9_6_0("Use Initialize(const std::vector<unsigned int>&) instead")
  virtual void Initialize(int numLevels, const int* blocksPerLevel);

  ///@{
  /**
   * returns the value of vtkUniformGrid::GridDescription() of any block
   */
  [[nodiscard]] vtkGetMacro(GridDescription, int);
  void SetGridDescription(int description);
  ///@}

  /**
   * Return the number of levels
   */
  [[nodiscard]] unsigned int GetNumberOfLevels() const;

  /**
   * Returns the number of blocks at the given level or zero if level is higher or equal than
   * numLevels
   */
  [[nodiscard]] unsigned int GetNumberOfBlocks(unsigned int level) const;

  /**
   * Deprecated, forward to GetNumberOfBlocks(level)
   */
  VTK_DEPRECATED_IN_9_6_0("Use GetNumberOfBlocks(level) instead")
  unsigned int GetNumberOfDataSets(unsigned int level) { return this->GetNumberOfBlocks(level); }

  /**
   * Returns number of blocks for all levels
   */
  [[nodiscard]] unsigned int GetNumberOfBlocks() const;

  /**
   * Deprecated, forward to GetNumberOfBlocks
   */
  VTK_DEPRECATED_IN_9_6_0("Use GetNumberOfBlocks instead")
  virtual unsigned int GetTotalNumberOfBlocks() { return this->GetNumberOfBlocks(); }

  /**
   * Returns the absolute block index from a level and a relative block index
   * or -1 if level is invalid
   */
  [[nodiscard]] int GetAbsoluteBlockIndex(unsigned int level, unsigned int relativeBlockIdx) const;

  /**
   * Deprecated, forward to GetAbsoluteBlockIndex
   */
  VTK_DEPRECATED_IN_9_6_0("Use GetAbsoluteBlockIndex(level, id) instead")
  [[nodiscard]] int GetIndex(unsigned int level, unsigned int id) const
  {
    return this->GetAbsoluteBlockIndex(level, id);
  }

  /**
   * Returns the an index pair (level, relative index) given a absolute block index
   */
  void ComputeIndexPair(unsigned int index, unsigned int& level, unsigned int& id);

  /**
   * Returns internal vector of blocks.
   * XXX: Do not use, will be deprecated.
   */
  [[nodiscard]] const std::vector<int>& GetNumBlocks() const { return this->NumBlocks; }

  /**
   * Copy internal fields from other into this
   */
  virtual void DeepCopy(vtkAMRMetaData* other);

protected:
  vtkAMRMetaData();
  ~vtkAMRMetaData() override;

private:
  vtkAMRMetaData(const vtkAMRMetaData&) = delete;
  void operator=(const vtkAMRMetaData&) = delete;

  void GenerateBlockLevel();

  //-------------------------------------------------------------------------
  // Essential information that determines an AMR structure. Must be copied
  //-------------------------------------------------------------------------

  // The type of grid stored in this AMR
  int GridDescription = vtkStructuredData::VTK_STRUCTURED_INVALID;

  // NumBlocks[i] stores the total number of blocks from level 0 to level i-1
  std::vector<int> NumBlocks = { 0 };

  //-------------------------------------------------------------------------
  // Auxiliary information that be computed
  //-------------------------------------------------------------------------

  // only necessary if need to call ComputeIndexPair
  vtkSmartPointer<vtkUnsignedIntArray> BlockLevel;
};

VTK_ABI_NAMESPACE_END
#endif
