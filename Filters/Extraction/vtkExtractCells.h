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

/**
 * @class   vtkExtractCells
 * @brief   subset a vtkDataSet to create a vtkUnstructuredGrid
 *
 *
 *    Given a vtkDataSet and a list of cell ids, create a vtkUnstructuredGrid
 *    composed of these cells.  If the cell list is empty when vtkExtractCells
 *    executes, it will set up the ugrid, point and cell arrays, with no points,
 *    cells or data.
 */

#ifndef vtkExtractCells_h
#define vtkExtractCells_h

#include "vtkFiltersExtractionModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

class vtkIdList;
class vtkExtractCellsSTLCloak;

class VTKFILTERSEXTRACTION_EXPORT vtkExtractCells : public vtkUnstructuredGridAlgorithm
{
public:
  //@{
  /**
   * Standard methods for construction, type info, and printing.
   */
  vtkTypeMacro(vtkExtractCells, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkExtractCells* New();
  //@}

  /**
   * Set the list of cell IDs that the output vtkUnstructuredGrid will be
   * composed of.  Replaces any other cell ID list supplied so far.  (Set to
   * nullptr to free memory used by cell list.)  The cell ids should be >=0.
   */
  void SetCellList(vtkIdList* l);

  /**
   * Add the supplied list of cell IDs to those that will be included in the
   * output vtkUnstructuredGrid. The cell ids should be >=0.
   */
  void AddCellList(vtkIdList* l);

  /**
   * Add this range of cell IDs to those that will be included in the output
   * vtkUnstructuredGrid. Note that (from < to), and (from >= 0).
   */
  void AddCellRange(vtkIdType from, vtkIdType to);

  //@{
  /**
   * Another way to provide ids using a pointer to vtkIdType array.
   */
  void SetCellIds(const vtkIdType* ptr, vtkIdType numValues);
  void AddCellIds(const vtkIdType* ptr, vtkIdType numValues);
  //@}

  //@{
  /**
   * If all cells are being extracted, this filter can use fast path to speed up
   * the extraction. In that case, one can set this flag to true. When set to
   * true, cell ids added via the various methods are simply ignored.
   * Defaults to false.
   */
  vtkSetMacro(ExtractAllCells, bool);
  vtkGetMacro(ExtractAllCells, bool);
  vtkBooleanMacro(ExtractAllCells, bool);
  //@}

  //@{
  /**
   * If the cell ids specified are already sorted and unique, then set this to
   * true to avoid the filter from doing time-consuming sorts and uniquification
   * operations. Defaults to false.
   */
  vtkSetMacro(AssumeSortedAndUniqueIds, bool);
  vtkGetMacro(AssumeSortedAndUniqueIds, bool);
  vtkBooleanMacro(AssumeSortedAndUniqueIds, bool);
  //@}
protected:
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  vtkExtractCells();
  ~vtkExtractCells() override;

  void Copy(vtkDataSet* input, vtkUnstructuredGrid* output);
  vtkIdType ReMapPointIds(vtkDataSet* grid);

  void CopyCellsDataSet(vtkDataSet* input, vtkUnstructuredGrid* output);
  void CopyCellsUnstructuredGrid(vtkDataSet* input, vtkUnstructuredGrid* output);

  vtkExtractCellsSTLCloak* CellList = nullptr;
  vtkIdType SubSetUGridCellArraySize = 0;
  vtkIdType SubSetUGridFacesArraySize = 0;
  bool InputIsUgrid = false;
  bool ExtractAllCells = false;
  bool AssumeSortedAndUniqueIds = false;

private:
  vtkExtractCells(const vtkExtractCells&) = delete;
  void operator=(const vtkExtractCells&) = delete;
};

#endif
