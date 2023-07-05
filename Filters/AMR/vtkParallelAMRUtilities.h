// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkParallelAMRUtilities
 *
 *
 *  A concrete instance of vtkObject that employs a singleton design
 *  pattern and implements functionality for AMR specific operations.
 *
 * @sa
 *  vtkOverlappingAMR, vtkAMRBox
 */

#ifndef vtkParallelAMRUtilities_h
#define vtkParallelAMRUtilities_h

#include "vtkAMRUtilities.h"
#include "vtkFiltersAMRModule.h" // For export macro
#include <vector>                // For C++ vector

// Forward declarations
VTK_ABI_NAMESPACE_BEGIN
class vtkMultiProcessController;
class vtkOverlappingAMR;

class VTKFILTERSAMR_EXPORT vtkParallelAMRUtilities : public vtkAMRUtilities
{
public:
  // Standard Routines
  vtkTypeMacro(vtkParallelAMRUtilities, vtkAMRUtilities);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * This method detects and strips partially overlapping cells from a
   * given AMR dataset. If ghost layers are detected, they are removed and
   * new grid instances are created to represent the stripped
   * data-set otherwise, each block is shallow-copied.

   * .SECTION Assumptions
   * 1) The ghosted AMR data must have complete metadata information.
   */
  static void StripGhostLayers(vtkOverlappingAMR* ghostedAMRData,
    vtkOverlappingAMR* strippedAMRData, vtkMultiProcessController* myController);

  /**
   * Compute map from block indices to process ids
   */
  static void DistributeProcessInformation(
    vtkOverlappingAMR* amr, vtkMultiProcessController* myController, std::vector<int>& ProcessMap);

  /**
   * Blank cells in overlapping AMR
   */
  static void BlankCells(vtkOverlappingAMR* amr, vtkMultiProcessController* myController);

private:
  vtkParallelAMRUtilities(const vtkParallelAMRUtilities&) = delete;
  void operator=(const vtkParallelAMRUtilities&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif /* vtkParallelAMRUtilities_h */
