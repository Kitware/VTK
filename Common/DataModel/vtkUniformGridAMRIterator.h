// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkUniformGridAMRIterator
 * @brief   specialized vtkDataObjectTreeIterator for AMR
 *
 * A specialized version of vtkDataObjectTreeIterator for vtkUniformGridAMR
 * with API to get current level and dataset index.
 *
 * Iterates only on the leaves (uniform grids) of the AMR and ignore the composite structure.
 * Does not support VisitOnlyLeaves off and TraversSubTree off.
 */

#ifndef vtkUniformGridAMRIterator_h
#define vtkUniformGridAMRIterator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataObjectTreeIterator.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONDATAMODEL_EXPORT vtkUniformGridAMRIterator : public vtkDataObjectTreeIterator
{
public:
  static vtkUniformGridAMRIterator* New();
  vtkTypeMacro(vtkUniformGridAMRIterator, vtkDataObjectTreeIterator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Move the iterator to the beginning of the AMR,
   * the index 0 at level 0.
   */
  void GoToFirstItem() override;

  /**
   * Move the iterator to the next item in the AMR,
   * skip composite datasets, only stop on leaves.
   */
  void GoToNextItem() override;

  /**
   * Returns the level for the current dataset.
   * Not supported when using Reverse mode.
   */
  [[nodiscard]] unsigned int GetCurrentLevel() const;

  /**
   * Returns the dataset relative index for the current data object.
   * Not supported when using Reverse mode.
   */
  [[nodiscard]] unsigned int GetCurrentIndex() const;

  /**
   * Return the information about the current data object
   * If the AMR being iterated on is a vtkOverlappingAMR, then
   * the bounds of the current box will be available in the information
   * using the vtkDataObject::BOUNDING_BOX() key.
   * Please note this is not the AMRMetaData of the AMR.
   */
  vtkInformation* GetCurrentMetaData() override;

protected:
  vtkUniformGridAMRIterator();
  ~vtkUniformGridAMRIterator() override;

private:
  vtkUniformGridAMRIterator(const vtkUniformGridAMRIterator&) = delete;
  void operator=(const vtkUniformGridAMRIterator&) = delete;

  /**
   * Check VisitOnlyLeaves and TraverseSubtree have not beed changed then return true.
   * if they were, return false with an error.
   */
  [[nodiscard]] bool IsValid() VTK_FUTURE_CONST;

  /**
   * Check if current item is valid (not empty, not a composite dataset) and keep iterating if it is
   * not.
   */
  void CheckItemAndLoopIfNeeded();

  unsigned int CurrentLevel = 0;
  unsigned int CurrentIndex = 0;
};

VTK_ABI_NAMESPACE_END
#endif
