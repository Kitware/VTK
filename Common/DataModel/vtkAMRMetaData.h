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
#include "vtkObject.h"
#include "vtkSmartPointer.h" // For vtkSmartPointer

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

  /**
   * Initialize the meta information
   * numLevels is the number of levels
   * blocksPerLevel[i] is the number of blocks at level i
   */
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
   * Returns the number of datasets at the given level
   */
  [[nodiscard]] unsigned int GetNumberOfDataSets(unsigned int level) const;

  /**
   * Returns total number of datasets
   */
  [[nodiscard]] unsigned int GetTotalNumberOfBlocks();

  /**
   * Returns the single index from a pair of indices
   */
  [[nodiscard]] int GetIndex(unsigned int level, unsigned int id) const;

  /**
   * Returns the an index pair given a single index
   */
  void ComputeIndexPair(unsigned int index, unsigned int& level, unsigned int& id);

  /**
   * Returns internal vector of blocks.
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
  int GridDescription = -1; // example: vtkStructuredData::VTK_XYZ_GRID

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
