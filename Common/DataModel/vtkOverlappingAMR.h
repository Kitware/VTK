// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOverlappingAMR
 * @brief   a multi-resolution dataset based on vtkUniformGrid allowing overlaps
 *
 * vtkOverlappingAMR groups vtkUniformGrid into level of different refinement
 * (AMR stands for Adaptive Mesh Refinement). See SetDataSet to add a new grid.
 *
 * The grids of a level are expected to have the same spacing and refinement ratio.
 * The refinement ratio represent the spacing factor between a level and the
 * previous one. This class does not ensure the link between spacing and refinement
 * ratio: please set them carefully.
 *
 * Associated to each grid, a vtkAMRBox object describes the main information
 * of the grid: origin, extent, spacing. When creating a vtkOverlappingAMR,
 * you should call SetAMRBox for each block of each level.
 *
 * In a distributed environement, the structure should be shared across all rank:
 * the vtkAMRInformation and vtkAMRBox should be duplicated as needed.
 *
 * @sa
 * vtkAMRInformation, vtkNonOverlappingAMR, vtkUniformGridAMR, vtkAMRBox
 */

#ifndef vtkOverlappingAMR_h
#define vtkOverlappingAMR_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkUniformGridAMR.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkAMRBox;
class vtkCompositeDataIterator;
class vtkUniformGrid;
class vtkAMRInformation;
class vtkInformationIdTypeKey;

class VTKCOMMONDATAMODEL_EXPORT vtkOverlappingAMR : public vtkUniformGridAMR
{
public:
  static vtkOverlappingAMR* New();

  /**
   * Return class name of data type (see vtkType.h for definitions).
   */
  int GetDataObjectType() VTK_FUTURE_CONST override { return VTK_OVERLAPPING_AMR; }

  vtkTypeMacro(vtkOverlappingAMR, vtkUniformGridAMR);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return a new iterator (the iterator has to be deleted by the user).
   */
  VTK_NEWINSTANCE vtkCompositeDataIterator* NewIterator() override;

  ///@{
  /**
   * Get/Set the global origin of the amr data set
   */
  void SetOrigin(const double origin[3]);
  double* GetOrigin();
  ///@}

  ///@{
  /**
   * Get/Set the grid spacing at a given level.
   * Note that is expected (but not enforeced) that spacing evolves according
   * to RefinementRatio factor from previous level to current.
   * In pseudo code, you should ensure the following:
   * ```
   * GetSpacing(lvl, spacing)
   * SetSpacing(lvl + 1, spacing / RefinementRatio)
   * ```
   *
   * See SetRefinementRatio.
   */
  void SetSpacing(unsigned int level, const double spacing[3]);
  void GetSpacing(unsigned int level, double spacing[3]);
  ///@}

  ///@{
  /**
   * Set/Get the AMRBox for a given block
   */
  void SetAMRBox(unsigned int level, unsigned int id, const vtkAMRBox& box);
  const vtkAMRBox& GetAMRBox(unsigned int level, unsigned int id);
  ///@}

  /**
   * Returns the bounding information of a data set.
   */
  void GetBounds(unsigned int level, unsigned int id, double bb[6]);

  /**
   * Returns the origin of an AMR block
   */
  void GetOrigin(unsigned int level, unsigned int id, double origin[3]);

  static vtkInformationIdTypeKey* NUMBER_OF_BLANKED_POINTS();

  /**
   * Retrieve an instance of this class from an information object.
   */
  static vtkOverlappingAMR* GetData(vtkInformation* info)
  {
    return vtkOverlappingAMR::SafeDownCast(Superclass::GetData(info));
  }
  static vtkOverlappingAMR* GetData(vtkInformationVector* v, int i = 0)
  {
    return vtkOverlappingAMR::SafeDownCast(Superclass::GetData(v, i));
  }

  /**
   * Sets the refinement of a given level. The spacing at level
   * level+1 is defined as spacing(level+1) = spacing(level)/refRatio(level).
   * Note that currently, this is not enforced by this class however
   * some algorithms might not function properly if the spacing in
   * the blocks (vtkUniformGrid) does not match the one described
   * by the refinement ratio.
   */
  void SetRefinementRatio(unsigned int level, int refRatio);

  /**
   * Returns the refinement of a given level.
   */
  int GetRefinementRatio(unsigned int level);

  ///@{
  /**
   * Set/Get the source id of a block. The source id is produced by an
   * AMR source, e.g. a file reader might set this to be a file block id
   */
  void SetAMRBlockSourceIndex(unsigned int level, unsigned int id, int sourceId);
  int GetAMRBlockSourceIndex(unsigned int level, unsigned int id);
  ///@}

  /**
   * Returns the refinement ratio for the position pointed by the iterator.
   */
  int GetRefinementRatio(vtkCompositeDataIterator* iter);

  /**
   * Return whether parent child information has been generated
   */
  bool HasChildrenInformation();

  /**
   * Generate the parent/child relationships - needed to be called
   * before GetParents or GetChildren can be used!
   */
  void GenerateParentChildInformation();

  /**
   * Return a pointer to Parents of a block.  The first entry is the number
   * of parents the block has followed by its parent ids in level-1.
   * If none exits it returns nullptr.
   */
  unsigned int* GetParents(unsigned int level, unsigned int index, unsigned int& numParents);

  /**
   * Return a pointer to Children of a block.  The first entry is the number
   * of children the block has followed by its children ids in level+1.
   * If none exits it returns nullptr.
   */
  unsigned int* GetChildren(unsigned int level, unsigned int index, unsigned int& numChildren);

  /**
   * Prints the parents and children of a requested block (Debug Routine)
   */
  void PrintParentChildInfo(unsigned int level, unsigned int index);

  // Unhide superclass method
  void GetBounds(double b[6]) { Superclass::GetBounds(b); }

  /**
   * Given a point q, find the highest level grid that contains it.
   */
  bool FindGrid(double q[3], unsigned int& level, unsigned int& gridId);

  /**
   * Get/Set the internal representation of amr meta meta data
   */
  vtkAMRInformation* GetAMRInfo() override { return Superclass::GetAMRInfo(); }
  void SetAMRInfo(vtkAMRInformation* info) override { Superclass::SetAMRInfo(info); }

  ///@{
  /**
   * Check whether the data set is internally consistent, e.g.
   * whether the meta data and actual data blocks match.
   * Incorrectness will be reported as error messages
   */
  void Audit();

protected:
  vtkOverlappingAMR();
  ~vtkOverlappingAMR() override;
  ///@}

private:
  vtkOverlappingAMR(const vtkOverlappingAMR&) = delete;
  void operator=(const vtkOverlappingAMR&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
