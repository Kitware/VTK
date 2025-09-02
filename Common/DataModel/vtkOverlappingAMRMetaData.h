// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOverlappingAMRMetaData
 * @brief   Meta data that describes the structure of an overlapping AMR data set
 *
 * vtkOverlappingAMRMetaData encapsulates the following meta information for a generic AMR data set
 * - a list of vtkAMRBox objects
 * - RefinementRatio between AMR levels
 * - GridSpacing for each level
 * - BlockBounds for each block
 * - parent child information, if requested
 *
 * For many methods, Refinement/Spacing will be used preferentially if available
 * then BlockBounds as a fallback if available. See each method documentation.
 *
 * @sa vtkUniformGridAMR, vtkOverlappingAMR, vtkAMRBox, vtkOverlappingAMRMetaData,
 * vtkNonOverlappingAMR
 */

#ifndef vtkOverlappingAMRMetaData_h
#define vtkOverlappingAMRMetaData_h

#include "vtkAMRBox.h" //for storing AMR Boxes
#include "vtkAMRMetaData.h"
#include "vtkBoundingBox.h"           //for storing AMR Bounding boxes
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkSmartPointer.h"          //for ivars

VTK_ABI_NAMESPACE_BEGIN
class vtkIntArray;
class vtkDoubleArray;
class VTKCOMMONDATAMODEL_EXPORT vtkOverlappingAMRMetaData : public vtkAMRMetaData
{
public:
  static vtkOverlappingAMRMetaData* New();
  vtkTypeMacro(vtkOverlappingAMRMetaData, vtkAMRMetaData);

  void PrintSelf(ostream& os, vtkIndent indent) override;
  bool operator==(const vtkOverlappingAMRMetaData& other) const;
  bool operator!=(const vtkOverlappingAMRMetaData& other) const { return !(*this == other); }

  // VTK_DEPRECATED_IN_9_6_0
  // Remove once the deprecated Initialize method in parent class is removed
  using vtkAMRMetaData::Initialize;

  /**
   * Initialize the meta information
   * blocksPerLevels is the number of blocks for each level
   */
  void Initialize(const std::vector<unsigned int>& blocksPerLevel) override;

  ///@{
  /**
   * Get the AMR block origin as a 3 sized array
   * The origin is essentially the minimum of all the grids.
   */
  void GetOrigin(double origin[3]);
  [[nodiscard]] double* GetOrigin();
  void SetOrigin(const double* origin);
  ///@}

  /**
   * Returns the bounds of the entire domain as a 6 sized array
   */
  [[nodiscard]] const double* GetBounds();

  /**
   * Returns the bounding box of a given box
   * Rely on the BlockBounds if available or compute them on the fly
   * using Spacing and Refinement if not.
   */
  void GetBounds(unsigned int level, unsigned int id, double* bb);

  /**
   * Set the Bounds of a specific AMR block
   * This is usually called when using vtkRectilinearGrid in an OverlappingAMR
   * and once set for all blocks, you can avoid using Spacing and RefinementRatio.
   */
  void SetBounds(unsigned int level, unsigned int id, double* bb);

  /**
   * Recover the origin of the grid at (level,id).
   * Return true if sucessful, false otherwise.
   * Use Spacing if available, fallback on BlockBounds
   */
  bool GetOrigin(unsigned int level, unsigned int id, double* origin);

  /**
   * Return the spacing at the given level
   * Should only be called if HasSpacing() is true.
   */
  void GetSpacing(unsigned int level, double spacing[3]);

  /**
   * Return if a specific level has spacing
   */
  [[nodiscard]] bool HasSpacing(unsigned int level) const;

  /**
   * Return if all levels have spacing
   */
  [[nodiscard]] bool HasSpacing() const;

  /**
   * Return true if a specific block bounds have been set
   */
  [[nodiscard]] bool HasBlockBounds(unsigned int index) const;

  /**
   * Return true if block bounds have been set on all blocks
   */
  [[nodiscard]] bool HasBlockBounds() const;

  ///@{
  /**
   * Methods to set and get the AMR box at a given position
   */
  void SetAMRBox(unsigned int level, unsigned int id, const vtkAMRBox& box);
  [[nodiscard]] const vtkAMRBox& GetAMRBox(unsigned int level, unsigned int id) const;
  bool GetAMRBox(unsigned int level, unsigned int id, vtkAMRBox& box) const;
  ///@}

  /**
   * Fill the provided box coarsened to the previous level
   * Requite spacing/refinement ratio.
   * Return true on sucess, false otherwise.
   */
  bool GetCoarsenedAMRBox(unsigned int level, unsigned int id, vtkAMRBox& box);

  ///@{
  /**
   * Get/Set the SourceIndex of a block. Typically, this is a file-type specific index
   * that can be used by a reader to load a particular file block
   */
  [[nodiscard]] int GetAMRBlockSourceIndex(int index);
  void SetAMRBlockSourceIndex(int index, int sourceId);
  ///@}

  /**
   * This method computes the refinement ratio at each level.
   * At each level, l, the refinement ratio r_l is computed by
   * r_l = D_{l} / D_{l+1}, where D_{l+1} and D_{l} are the grid
   * spacings at the next and current level respectively.
   * .SECTION Assumptions
   * 1) Within each level, the refinement ratios are the same for all blocks.
   * 2) The refinement ratio is uniform along each dimension of the block.
   * Require HasSpacing() == true.
   * Return true on success, false otherwise.
   */
  bool GenerateRefinementRatio();

  /**
   * Returns whether refinement ratio has been set (either by calling
   * GenerateRefinementRatio() or by calling SetRefinementRatio()
   * If this is true, then HasSpacing is true too.
   */
  [[nodiscard]] bool HasRefinementRatio() const;

  /**
   * Set the refinement ratio at a level. This method should be
   * called for all levels, if called at all.
   */
  void SetRefinementRatio(unsigned int level, int ratio);

  /**
   * Returns the refinement of a given level.
   * Make sure to call SetRefinementRatio or GenerateRefinementRatio first
   * and that HasRefinementRatio is true
   */
  [[nodiscard]] int GetRefinementRatio(unsigned int level) const;

  /**
   * Set the spacing at a given level
   */
  void SetSpacing(unsigned int level, const double* h);

  /**
   * Return whether parent child information has been generated
   */
  [[nodiscard]] bool HasChildrenInformation() const;

  /**
   * Generate parent/children information if needed then return a pointer to Parents of a block.
   * The first entry is the number of parents the block has followed by its parent ids in level-1.
   * If none exits it returns nullptr.
   */
  unsigned int* GetParents(unsigned int level, unsigned int index, unsigned int& numParents);

  /**
   * Generate parent/children information if needed then return a pointer to Children of a block.
   * The first entry is the number of children the block has followed by its children ids in
   * level+1. If none exits it returns nullptr.
   */
  unsigned int* GetChildren(unsigned int level, unsigned int index, unsigned int& numChildren);

  /**
   * Generate if needed and prints the parents and children of a requested block (Debug Routine)
   */
  void PrintParentChildInfo(unsigned int level, unsigned int index);

  /**
   * Generate the parent/child relationships - needed to be called
   * before GetParents or GetChildren can be used!
   * Will GenerateRefinementRatio if not generated yet and BlockBounds are not set
   *
   * Use Spacing/RefinementRatio if set, fallback on BlockBounds and skip block if not available.
   */
  void GenerateParentChildInformation();

  /**
   * Checks whether the meta data is internally consistent.
   */
  [[nodiscard]] bool CheckValidity();

  /**
   * Given a point q, find whether q is bounded by the data set at
   * (level,index).  If it is, set cellIdx to the cell index and return
   * true; otherwise return false
   * Require Spacing.
   */
  bool FindCell(double q[3], unsigned int level, unsigned int index, int& cellIdx);

  /**
   * find the grid that contains the point q at the specified level
   */
  bool FindGrid(double q[3], int level, unsigned int& gridId);

  /**
   * Given a point q, find the highest level grid that contains it.
   */
  bool FindGrid(double q[3], unsigned int& level, unsigned int& gridId);

  /**
   * Get children at a specific level
   * HasChildrenInformation is NOT checked by this method.
   */
  [[nodiscard]] std::vector<std::vector<unsigned int>>& GetChildrenAtLevel(unsigned int i)
  {
    return this->AllChildren[i];
  }

  ///@{
  /**
   * Returns the grid type, VTK_DATA_SET by default.
   */
  [[nodiscard]] vtkGetMacro(GridType, unsigned int);
  vtkSetMacro(GridType, unsigned int);
  ///@}

  /**
   * Check it is an vtkOverlappingAMRMetaData and
   * copy internal fields from other into this
   */
  void DeepCopy(vtkAMRMetaData* other) override;

protected:
  vtkOverlappingAMRMetaData();
  ~vtkOverlappingAMRMetaData() override;

private:
  vtkOverlappingAMRMetaData(const vtkOverlappingAMRMetaData&) = delete;
  void operator=(const vtkOverlappingAMRMetaData&) = delete;

  bool HasValidOrigin() const;
  bool HasValidBounds() const;
  void UpdateBounds(int level, int id);
  void AllocateBoxes(unsigned int n);
  void AllocateBlockBounds(unsigned int n);
  void CalculateParentChildRelationShip(unsigned int level,
    std::vector<std::vector<unsigned int>>& children,
    std::vector<std::vector<unsigned int>>& parents);

  //-------------------------------------------------------------------------
  // Essential information that determines an AMR structure. Must be copied
  //-------------------------------------------------------------------------
  unsigned int GridType = VTK_DATA_SET;

  // the origin of the whole data set
  double Origin[3] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MAX, VTK_DOUBLE_MAX };

  // vtkAMRBoxes, one per block
  std::vector<vtkAMRBox> Boxes;

  // BoundingBoxes, one per block if used
  std::vector<vtkBoundingBox> BlockBounds;

  // Typically, this maps to a file block index used by the reader
  vtkSmartPointer<vtkIntArray> SourceIndex;

  // The grid spacing for all levels
  vtkNew<vtkDoubleArray> Spacing;

  // the bounds of the entire domain
  double Bounds[6] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN, VTK_DOUBLE_MAX, VTK_DOUBLE_MIN,
    VTK_DOUBLE_MAX, VTK_DOUBLE_MIN };

  //-------------------------------------------------------------------------
  // Auxiliary information that be computed
  //-------------------------------------------------------------------------

  // refinement ratio between two adjacent levels
  vtkNew<vtkIntArray> Refinement;

  // parent child information
  std::vector<std::vector<std::vector<unsigned int>>> AllChildren;
  std::vector<std::vector<std::vector<unsigned int>>> AllParents;
};

VTK_ABI_NAMESPACE_END
#endif
