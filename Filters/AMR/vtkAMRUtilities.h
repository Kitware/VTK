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
// .NAME vtkAMRUtilities -- Support for distributed/serial AMR operations
//
// .SECTION Description
//  A concrete instance of vtkObject that employs a singleton design
//  pattern and implements functionality for AMR specific operations.
//
// .SECTION See Also
//  vtkOverlappingAMR, vtkAMRBox

#ifndef VTKAMRUTILITIES_H_
#define VTKAMRUTILITIES_H_

#include "vtkFiltersAMRModule.h" // For export macro
#include "vtkObject.h"
#include <vector> // For C++ vector

// Forward declarations
class vtkAMRBox;
class vtkFieldData;
class vtkMultiProcessController;
class vtkOverlappingAMR;
class vtkUniformGridAMR;
class vtkUniformGrid;
class vtkAMRInformation;

class VTKFILTERSAMR_EXPORT vtkAMRUtilities : public vtkObject
{
public:
  // Standard Routines
  vtkTypeMacro(vtkAMRUtilities,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent );

  // Description:
  // This method detects and strips partially overlapping cells from a
  // given AMR dataset. If ghost layers are detected, they are removed and
  // new grid instances are created to represent the stripped
  // data-set otherwise, each block is shallow-copied.
  //
  // .SECTION Assumptions
  // 1) The ghosted AMR data must have complete metadata information.
  static void StripGhostLayers(
      vtkOverlappingAMR *ghostedAMRData,
      vtkOverlappingAMR *strippedAMRData,
      vtkMultiProcessController *myController=NULL);

  // Description:
  // A quick test of whether partially overlapping ghost cells exist. This test
  // starts from the highest-res boxes and checks if they have partially
  // overlapping cells. The code returns with true once partially overlapping
  // cells are detected. Otherwise, false is returned.
  static bool HasPartiallyOverlappingGhostCells(vtkOverlappingAMR *amr);

  // Description:
  // Compute map from block indices to process ids
  static void DistributeProcessInformation(vtkOverlappingAMR* amr, vtkMultiProcessController *myController, std::vector<int>& ProcessMap);

  // Description:
  // Blank cells in overlapping AMR
  static void BlankCells(vtkOverlappingAMR* amr,  vtkMultiProcessController *myController);

protected:
  vtkAMRUtilities() {}
  ~vtkAMRUtilities() {}

  // Description:
  // Given the real-extent w.r.t. the ghosted grid, this method copies the
  // field data (point/cell) data on the stripped grid.
  static void CopyFieldsWithinRealExtent(
      int realExtent[6],
      vtkUniformGrid *ghostedGrid,
      vtkUniformGrid *strippedGrid);

  // Description:
  // Copies the fields from the given source to the given target.
  static void CopyFieldData(
      vtkFieldData *target, vtkIdType targetIdx,
      vtkFieldData *source, vtkIdType sourceIdx );

  // Description:
  // Strips ghost layers from the given grid according to the given ghost
  // vector which encodes the number of cells to remote from each of the
  // 6 sides {imin,imax,jmin,jmax,kmin,kmax}. For example, a ghost vector
  // of {0,2,0,2,0,0} would indicate that there exist 2 ghost cells on the
  // imax and jmax side.
  static vtkUniformGrid* StripGhostLayersFromGrid(
      vtkUniformGrid* grid, int ghost[6]);

private:
  vtkAMRUtilities(const vtkAMRUtilities&); // Not implemented
  void operator=(const vtkAMRUtilities&); // Not implemented
};

#endif /* VTKAMRUTILITIES_H_ */
