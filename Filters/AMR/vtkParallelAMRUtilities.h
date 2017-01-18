/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRUtilities.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
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

#include "vtkFiltersAMRModule.h" // For export macro
#include "vtkAMRUtilities.h"
#include <vector> // For C++ vector

// Forward declarations
class vtkMultiProcessController;
class vtkOverlappingAMR;

class VTKFILTERSAMR_EXPORT vtkParallelAMRUtilities : public vtkAMRUtilities
{
public:
  // Standard Routines
  vtkTypeMacro(vtkParallelAMRUtilities,vtkAMRUtilities);
  void PrintSelf(ostream& os, vtkIndent indent ) VTK_OVERRIDE;

  /**
   * This method detects and strips partially overlapping cells from a
   * given AMR dataset. If ghost layers are detected, they are removed and
   * new grid instances are created to represent the stripped
   * data-set otherwise, each block is shallow-copied.

   * .SECTION Assumptions
   * 1) The ghosted AMR data must have complete metadata information.
   */
  static void StripGhostLayers(
      vtkOverlappingAMR *ghostedAMRData,
      vtkOverlappingAMR *strippedAMRData,
      vtkMultiProcessController *myController);

  /**
   * Compute map from block indices to process ids
   */
  static void DistributeProcessInformation(vtkOverlappingAMR* amr, vtkMultiProcessController *myController, std::vector<int>& ProcessMap);

  /**
   * Blank cells in overlapping AMR
   */
  static void BlankCells(vtkOverlappingAMR* amr,  vtkMultiProcessController *myController);

private:
  vtkParallelAMRUtilities(const vtkParallelAMRUtilities&) VTK_DELETE_FUNCTION;
  void operator=(const vtkParallelAMRUtilities&) VTK_DELETE_FUNCTION;
};

#endif /* vtkParallelAMRUtilities_h */
