/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergeCells.h

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
 * @class   vtkMergeCells
 * @brief   merges any number of vtkDataSets back into a single
 *   vtkUnstructuredGrid
 *
 *
 *    Designed to work with distributed vtkDataSets, this class will take
 *    vtkDataSets and merge them back into a single vtkUnstructuredGrid.
 *
 *    The vtkPoints object of the unstructured grid will have data type
 *    VTK_FLOAT, regardless of the data type of the points of the
 *    input vtkDataSets.  If this is a problem, someone must let me know.
 *
 *    It is assumed the different DataSets have the same field arrays.  If
 *    the name of a global point ID array is provided, this class will
 *    refrain from including duplicate points in the merged Ugrid.  This
 *    class differs from vtkAppendFilter in these ways: (1) it uses less
 *    memory than that class (which uses memory equal to twice the size
 *    of the final Ugrid) but requires that you know the size of the
 *    final Ugrid in advance (2) this class assumes the individual DataSets have
 *    the same field arrays, while vtkAppendFilter intersects the field
 *    arrays (3) this class knows duplicate points may be appearing in
 *    the DataSets and can filter those out, (4) this class is not a filter.
*/

#ifndef vtkMergeCells_h
#define vtkMergeCells_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkObject.h"
#include "vtkDataSetAttributes.h" // Needed for FieldList

class vtkDataSet;
class vtkUnstructuredGrid;
class vtkPointData;
class vtkCellData;
class vtkMergeCellsSTLCloak;

class VTKFILTERSGENERAL_EXPORT vtkMergeCells : public vtkObject
{
public:
  vtkTypeMacro(vtkMergeCells, vtkObject);
  void PrintSelf(ostream &os, vtkIndent indent) override;

  static vtkMergeCells *New();

  /**
   * Set the vtkUnstructuredGrid object that will become the
   * union of the DataSets specified in MergeDataSet calls.
   * vtkMergeCells assumes this grid is empty at first.
   */

  virtual void SetUnstructuredGrid(vtkUnstructuredGrid*);
  vtkGetObjectMacro(UnstructuredGrid, vtkUnstructuredGrid);

  /**
   * Specify the total number of cells in the final vtkUnstructuredGrid.
   * Make this call before any call to MergeDataSet().
   */

  vtkSetMacro(TotalNumberOfCells, vtkIdType);
  vtkGetMacro(TotalNumberOfCells, vtkIdType);

  /**
   * Specify the total number of points in the final vtkUnstructuredGrid
   * Make this call before any call to MergeDataSet().  This is an
   * upper bound, since some points may be duplicates.
   */

  vtkSetMacro(TotalNumberOfPoints, vtkIdType);
  vtkGetMacro(TotalNumberOfPoints, vtkIdType);

  /**
   * vtkMergeCells attempts eliminate duplicate points when merging
   * data sets.  This is done most efficiently if a global point ID
   * field array is available.  Set the name of the point array if you
   * have one.
   */

  vtkSetMacro(UseGlobalIds, int);
  vtkGetMacro(UseGlobalIds, int);

  /**
   * vtkMergeCells attempts eliminate duplicate points when merging
   * data sets.  If no global point ID field array name is provided,
   * it will use a point locator to find duplicate points.  You can
   * set a tolerance for that locator here.  The default tolerance
   * is 10e-4.
   */

  vtkSetClampMacro(PointMergeTolerance, float, 0.0, VTK_FLOAT_MAX);
  vtkGetMacro(PointMergeTolerance, float);

  /**
   * vtkMergeCells will detect and filter out duplicate cells if you
   * provide it the name of a global cell ID array.
   */

  vtkSetMacro(UseGlobalCellIds, int);
  vtkGetMacro(UseGlobalCellIds, int);

  /**
   * vtkMergeCells attempts eliminate duplicate points when merging
   * data sets.  If for some reason you don't want it to do this,
   * than MergeDuplicatePointsOff().
   */

  vtkSetMacro(MergeDuplicatePoints, vtkTypeBool);
  vtkGetMacro(MergeDuplicatePoints, vtkTypeBool);
  vtkBooleanMacro(MergeDuplicatePoints, vtkTypeBool);

  /**
   * We need to know the number of different data sets that will
   * be merged into one so we can pre-allocate some arrays.
   * This can be an upper bound, not necessarily exact.
   */

  vtkSetMacro(TotalNumberOfDataSets, int);
  vtkGetMacro(TotalNumberOfDataSets, int);

  /**
   * Provide a DataSet to be merged in to the final UnstructuredGrid.
   * This call returns after the merge has completed.  Be sure to call
   * SetTotalNumberOfCells, SetTotalNumberOfPoints, and SetTotalNumberOfDataSets
   * before making this call.  Return 0 if OK, -1 if error.
   */

  int MergeDataSet(vtkDataSet *set);

  /**
   * Call Finish() after merging last DataSet to free unneeded memory and to
   * make sure the ugrid's GetNumberOfPoints() reflects the actual
   * number of points set, not the number allocated.
   */

  void Finish();

protected:

  vtkMergeCells();
  ~vtkMergeCells() override;

private:

  void FreeLists();
  void StartUGrid(vtkDataSet *set);
  vtkIdType *MapPointsToIdsUsingGlobalIds(vtkDataSet *set);
  vtkIdType *MapPointsToIdsUsingLocator(vtkDataSet *set);
  vtkIdType AddNewCellsUnstructuredGrid(vtkDataSet *set, vtkIdType *idMap);
  vtkIdType AddNewCellsDataSet(vtkDataSet *set, vtkIdType *idMap);

  vtkIdType GlobalCellIdAccessGetId(vtkIdType idx);
  int GlobalCellIdAccessStart(vtkDataSet *set);
  vtkIdType GlobalNodeIdAccessGetId(vtkIdType idx);
  int GlobalNodeIdAccessStart(vtkDataSet *set);

  int TotalNumberOfDataSets;

  vtkIdType TotalNumberOfCells;
  vtkIdType TotalNumberOfPoints;

  vtkIdType NumberOfCells;     // so far
  vtkIdType NumberOfPoints;

  int UseGlobalIds;       // point, or node, IDs
  int GlobalIdArrayType;
  void* GlobalIdArray;

  int UseGlobalCellIds;   // cell IDs
  int GlobalCellIdArrayType;
  void* GlobalCellIdArray;

  float PointMergeTolerance;
  vtkTypeBool MergeDuplicatePoints;

  char InputIsUGrid;
  char InputIsPointSet;

  vtkMergeCellsSTLCloak *GlobalIdMap;
  vtkMergeCellsSTLCloak *GlobalCellIdMap;

  vtkDataSetAttributes::FieldList *ptList;
  vtkDataSetAttributes::FieldList *cellList;

  vtkUnstructuredGrid *UnstructuredGrid;

  int nextGrid;

  vtkMergeCells(const vtkMergeCells&) = delete;
  void operator=(const vtkMergeCells&) = delete;
};
#endif
