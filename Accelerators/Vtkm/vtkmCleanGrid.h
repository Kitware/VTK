//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================
/**
 * @class   vtkmCleanGrid
 * @brief   removes redundant or unused cells and/or points
 *
 * vtkmCleanGrid is a filter that takes vtkDataSet data as input and
 * generates vtkUnstructuredGrid as output. vtkmCleanGrid will convert all cells
 * to an explicit representation, and if enabled, will remove unused points.
 *
*/

#ifndef vtkmCleanGrid_h
#define vtkmCleanGrid_h

#include "vtkUnstructuredGridAlgorithm.h"
#include "vtkAcceleratorsVTKmModule.h" //required for correct implementation

class vtkDataSet;
class vtkUnstructuredGrid;

class VTKACCELERATORSVTKM_EXPORT vtkmCleanGrid : public vtkUnstructuredGridAlgorithm
{
public:
  vtkTypeMacro(vtkmCleanGrid, vtkUnstructuredGridAlgorithm)

  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkmCleanGrid* New();

  //@{
  /**
   * Get/Set if the points from the input that are unused in the output should
   * be removed. This will take extra time but the result dataset may use
   * less memory. Off by default.
   */
  vtkSetMacro(CompactPoints, bool);
  vtkGetMacro(CompactPoints, bool);
  vtkBooleanMacro(CompactPoints, bool);
  //@}

protected:
  vtkmCleanGrid();
  ~vtkmCleanGrid();

  int FillInputPortInformation(int, vtkInformation *) VTK_OVERRIDE;
  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) VTK_OVERRIDE;

  bool CompactPoints;

private:
  vtkmCleanGrid(const vtkmCleanGrid&) VTK_DELETE_FUNCTION;
  void operator=(const vtkmCleanGrid&) VTK_DELETE_FUNCTION;
};

#endif // vtkmCleanGrid_h
// VTK-HeaderTest-Exclude: vtkmCleanGrid.h
