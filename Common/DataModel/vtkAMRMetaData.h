// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAMRMetaData
 * @brief   Meta data that describes the structure of a generic AMR data set
 *
 * vtkAMRMetaData encapsulates the following meta information for a generic AMR data set
 * - The file block index for each block
 * - the Grid description
 *
 * @sa vtkUniformGridAMR, vtkOverlappingAMR, vtkAMRBox, vtkOverlappingAMRMetaData,
 * vtkNonOverlappingAMR
 */

#ifndef vtkAMRMetaData_h
#define vtkAMRMetaData_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDeprecation.h"           // For VTK_DEPRECATED_IN_9_8_0
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

  ///@{
  /**
   * returns the grid description used for all blocks
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
   * Returns number of blocks for all levels
   */
  [[nodiscard]] unsigned int GetNumberOfBlocks() const;

  /**
   * Returns the absolute block index from a level and a relative block index
   * or -1 if level is invalid
   */
  [[nodiscard]] int GetAbsoluteBlockIndex(unsigned int level, unsigned int relativeBlockIdx) const;

  /**
   * Returns the an index pair (level, relative index) given a absolute block index
   */
  void ComputeIndexPair(unsigned int index, unsigned int& level, unsigned int& id);

  /**
   * Returns internal vector of blocks.
   */
  VTK_DEPRECATED_IN_9_8_0("Use vtkAMRMetaData methods instead")
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
