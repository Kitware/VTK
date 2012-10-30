/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractCells.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

// .NAME vtkExtractCells - subset a vtkDataSet to create a vtkUnstructuredGrid
//
// .SECTION Description
//    Given a vtkDataSet and a list of cell Ids, create a vtkUnstructuredGrid
//    composed of these cells.  If the cell list is empty when vtkExtractCells
//    executes, it will set up the ugrid, point and cell arrays, with no points,
//    cells or data.

#ifndef __vtkExtractCells_h
#define __vtkExtractCells_h

#include "vtkFiltersExtractionModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

class vtkIdList;
class vtkExtractCellsSTLCloak;

class VTKFILTERSEXTRACTION_EXPORT vtkExtractCells : public vtkUnstructuredGridAlgorithm
{
public:
  vtkTypeMacro(vtkExtractCells, vtkUnstructuredGridAlgorithm);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  static vtkExtractCells *New();

  // Description:
  // Set the list of cell IDs that the output vtkUnstructuredGrid
  // will be composed of.  Replaces any other cell ID list supplied
  // so far.  (Set to NULL to free memory used by cell list.)

  void SetCellList(vtkIdList *l);

  // Description:
  // Add the supplied list of cell IDs to those that will be included
  // in the output vtkUnstructuredGrid.

  void AddCellList(vtkIdList *l);

  // Description:
  // Add this range of cell IDs to those that will be included
  // in the output vtkUnstructuredGrid.

  void AddCellRange(vtkIdType from, vtkIdType to);

protected:

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);

  vtkExtractCells();
  ~vtkExtractCells();

private:

  void Copy(vtkDataSet *input, vtkUnstructuredGrid *output);
  static vtkIdType findInSortedList(vtkIdList *idList, vtkIdType id);
  vtkIdList *reMapPointIds(vtkDataSet *grid);

  void CopyCellsDataSet(vtkIdList *ptMap, vtkDataSet *input,
                        vtkUnstructuredGrid *output);
  void CopyCellsUnstructuredGrid(vtkIdList *ptMap, vtkDataSet *input,
                                 vtkUnstructuredGrid *output);

  vtkExtractCellsSTLCloak *CellList;

  int SubSetUGridCellArraySize;
  char InputIsUgrid;

  vtkExtractCells(const vtkExtractCells&); // Not implemented
  void operator=(const vtkExtractCells&); // Not implemented
};

#endif
