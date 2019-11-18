/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPDistributedDataFilter.h

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
 * @class   vtkPDistributedDataFilter
 * @brief   Distribute data among processors
 *
 *
 * This filter redistributes data among processors in a parallel
 * application into spatially contiguous vtkUnstructuredGrids.
 * The execution model anticipated is that all processes read in
 * part of a large vtkDataSet. Each process sets the input of
 * filter to be that DataSet. When executed, this filter builds
 * in parallel a k-d tree, decomposing the space occupied by the
 * distributed DataSet into spatial regions.  It assigns each
 * spatial region to a processor.  The data is then redistributed
 * and the output is a single vtkUnstructuredGrid containing the
 * cells in the process' assigned regions.
 *
 * This filter is sometimes called "D3" for "distributed data decomposition".
 *
 * Enhancement: You can set the k-d tree decomposition, rather than
 * have D3 compute it.  This allows you to divide a dataset using
 * the decomposition computed for another dataset.  Obtain a description
 * of the k-d tree cuts this way:
 *
 * @code{cpp}
 *    vtkBSPCuts *cuts = D3Object1->GetCuts()
 * @endcode
 *
 * And set it this way:
 *
 * @code{cpp}
 *     D3Object2->SetCuts(cuts)
 * @endcode
 *
 *
 * It is desirable to have a field array of global node IDs
 * for two reasons:
 *
 * 1. When merging together sub grids that were distributed
 *    across processors, global node IDs can be used to remove
 *    duplicate points and significantly reduce the size of the
 *    resulting output grid.  If no such array is available,
 *    D3 will use a tolerance to merge points, which is much
 *    slower.
 *
 * 2. If ghost cells have been requested, D3 requires a
 *    global node ID array in order to request and transfer
 *    ghost cells in parallel among the processors.  If there
 *    is no global node ID array, D3 will in parallel create
 *    a global node ID array, and the time to do this can be
 *    significant.
 *
 * D3 uses `vtkPointData::GetGlobalIds` to access global
 * node ids from the input. If none is found,
 * and ghost cells have been requested, D3 will create a
 * temporary global node ID array before acquiring ghost cells.
 *
 * It is also desirable to have global element IDs (vtkCellData::GetGlobalIds).
 * However, if they don't exist D3 can create them relatively quickly.
 *
 * @warning
 * The Execute() method must be called by all processes in the
 * parallel application, or it will hang.  If you are not certain
 * that your pipeline will execute identically on all processors,
 * you may want to use this filter in an explicit execution mode.
 *
 * @sa
 * vtkKdTree vtkPKdTree vtkBSPCuts
 */

#ifndef vtkPDistributedDataFilter_h
#define vtkPDistributedDataFilter_h

#include "vtkDistributedDataFilter.h"
#include "vtkFiltersParallelGeometryModule.h" // For export macro

class vtkBSPCuts;
class vtkDataArray;
class vtkFloatArray;
class vtkIdList;
class vtkIdTypeArray;
class vtkIntArray;
class vtkMultiProcessController;
class vtkPDistributedDataFilterSTLCloak;
class vtkPKdTree;
class vtkUnstructuredGrid;

class VTKFILTERSPARALLELGEOMETRY_EXPORT vtkPDistributedDataFilter : public vtkDistributedDataFilter
{
public:
  vtkTypeMacro(vtkPDistributedDataFilter, vtkDistributedDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPDistributedDataFilter* New();

protected:
  vtkPDistributedDataFilter();
  ~vtkPDistributedDataFilter() override;

  /**
   * Build a vtkUnstructuredGrid for a spatial region from the
   * data distributed across processes.  Execute() must be called
   * by all processes, or it will hang.
   */

  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  void SingleProcessExecute(vtkDataSet* input, vtkUnstructuredGrid* output);

  /**
   * Implementation for request data.
   */
  int RequestDataInternal(vtkDataSet* input, vtkUnstructuredGrid* output);

private:
  enum
  {
    DeleteNo = 0,
    DeleteYes = 1
  };

  enum
  {
    DuplicateCellsNo = 0,
    DuplicateCellsYes = 1
  };

  enum
  {
    GhostCellsNo = 0,
    GhostCellsYes = 1
  };

  enum
  {
    UnsetGhostLevel = 99
  };

  /**
   * ?
   */
  int PartitionDataAndAssignToProcesses(vtkDataSet* set);

  /**
   * ?
   */
  vtkUnstructuredGrid* RedistributeDataSet(
    vtkDataSet* set, vtkDataSet* input, int filterOutDuplicateCells);

  /**
   * ?
   */
  int ClipGridCells(vtkUnstructuredGrid* grid);

  /**
   * ?
   */
  vtkUnstructuredGrid* AcquireGhostCells(vtkUnstructuredGrid* grid);

  /**
   * ?
   */
  void ComputeMyRegionBounds();

  /**
   * ?
   */
  int CheckFieldArrayTypes(vtkDataSet* set);

  /**
   * If any processes have 0 cell input data sets, then
   * spread the input data sets around (quickly) before formal
   * redistribution.
   */
  vtkDataSet* TestFixTooFewInputFiles(vtkDataSet* input, int& duplicateCells);

  /**
   * ?
   */
  vtkUnstructuredGrid* MPIRedistribute(
    vtkDataSet* in, vtkDataSet* input, int filterOutDuplicateCells);

  /**
   * ?
   */
  vtkIdList** GetCellIdsForProcess(int proc, int* nlists);

  /**
   * Fills in the Source and Target arrays which contain a schedule to allow
   * each processor to talk to every other.
   */
  void SetUpPairWiseExchange();

  //@{
  /**
   * ?
   */
  void FreeIntArrays(vtkIdTypeArray** ar);
  static void FreeIdLists(vtkIdList** lists, int nlists);
  static vtkIdType GetIdListSize(vtkIdList** lists, int nlists);
  //@}

  //@{
  /**
   * This transfers counts (array sizes) between processes.
   */
  vtkIdTypeArray* ExchangeCounts(vtkIdType myCount, int tag);
  vtkIdTypeArray* ExchangeCountsLean(vtkIdType myCount, int tag);
  vtkIdTypeArray* ExchangeCountsFast(vtkIdType myCount, int tag);
  //@}

  //@{
  /**
   * This transfers id valued data arrays between processes.
   */
  vtkIdTypeArray** ExchangeIdArrays(vtkIdTypeArray** arIn, int deleteSendArrays, int tag);
  vtkIdTypeArray** ExchangeIdArraysLean(vtkIdTypeArray** arIn, int deleteSendArrays, int tag);
  vtkIdTypeArray** ExchangeIdArraysFast(vtkIdTypeArray** arIn, int deleteSendArrays, int tag);
  //@}

  //@{
  /**
   * This transfers float valued data arrays between processes.
   */
  vtkFloatArray** ExchangeFloatArrays(vtkFloatArray** myArray, int deleteSendArrays, int tag);
  vtkFloatArray** ExchangeFloatArraysLean(vtkFloatArray** myArray, int deleteSendArrays, int tag);
  vtkFloatArray** ExchangeFloatArraysFast(vtkFloatArray** myArray, int deleteSendArrays, int tag);
  //@}

  //@{
  /**
   * ?
   */
  vtkUnstructuredGrid* ExchangeMergeSubGrids(vtkIdList** cellIds, int deleteCellIds,
    vtkDataSet* myGrid, int deleteMyGrid, int filterOutDuplicateCells, int ghostCellFlag, int tag);
  vtkUnstructuredGrid* ExchangeMergeSubGrids(vtkIdList*** cellIds, int* numLists, int deleteCellIds,
    vtkDataSet* myGrid, int deleteMyGrid, int filterOutDuplicateCells, int ghostCellFlag, int tag);
  vtkUnstructuredGrid* ExchangeMergeSubGridsLean(vtkIdList*** cellIds, int* numLists,
    int deleteCellIds, vtkDataSet* myGrid, int deleteMyGrid, int filterOutDuplicateCells,
    int ghostCellFlag, int tag);
  vtkUnstructuredGrid* ExchangeMergeSubGridsFast(vtkIdList*** cellIds, int* numLists,
    int deleteCellIds, vtkDataSet* myGrid, int deleteMyGrid, int filterOutDuplicateCells,
    int ghostCellFlag, int tag);
  //@}

  //@{
  /**
   * ?
   */
  char* MarshallDataSet(vtkUnstructuredGrid* extractedGrid, vtkIdType& size);
  vtkUnstructuredGrid* UnMarshallDataSet(char* buf, vtkIdType size);
  //@}

  //@{
  /**
   * ?
   */
  void ClipCellsToSpatialRegion(vtkUnstructuredGrid* grid);
#if 0
  void ClipWithVtkClipDataSet(vtkUnstructuredGrid *grid, double *bounds,
           vtkUnstructuredGrid **outside, vtkUnstructuredGrid **inside);
#endif
  //@}

  void ClipWithBoxClipDataSet(vtkUnstructuredGrid* grid, double* bounds,
    vtkUnstructuredGrid** outside, vtkUnstructuredGrid** inside);

  //@{
  /**
   * Accessors to the "GLOBALID" point and cell arrays of the dataset.
   * Global ids are used by D3 to uniquely name all points and cells
   * so that after shuffling data between processors, redundant information
   * can be quickly eliminated.
   */
  vtkIdTypeArray* GetGlobalNodeIdArray(vtkDataSet* set);
  vtkIdType* GetGlobalNodeIds(vtkDataSet* set);
  vtkIdTypeArray* GetGlobalElementIdArray(vtkDataSet* set);
  vtkIdType* GetGlobalElementIds(vtkDataSet* set);
  int AssignGlobalNodeIds(vtkUnstructuredGrid* grid);
  int AssignGlobalElementIds(vtkDataSet* in);
  vtkIdTypeArray** FindGlobalPointIds(vtkFloatArray** ptarray, vtkIdTypeArray* ids,
    vtkUnstructuredGrid* grid, vtkIdType& numUniqueMissingPoints);
  //@}

  /**
   * ?
   */
  vtkIdTypeArray** MakeProcessLists(
    vtkIdTypeArray** pointIds, vtkPDistributedDataFilterSTLCloak* procs);

  /**
   * ?
   */
  vtkIdList** BuildRequestedGrids(vtkIdTypeArray** globalPtIds, vtkUnstructuredGrid* grid,
    vtkPDistributedDataFilterSTLCloak* ptIdMap);

  //@{
  /**
   * ?
   */
  int InMySpatialRegion(float x, float y, float z);
  int InMySpatialRegion(double x, double y, double z);
  int StrictlyInsideMyBounds(float x, float y, float z);
  int StrictlyInsideMyBounds(double x, double y, double z);
  //@}

  //@{
  /**
   * ?
   */
  vtkIdTypeArray** GetGhostPointIds(
    int ghostLevel, vtkUnstructuredGrid* grid, int AddCellsIAlreadyHave);
  vtkUnstructuredGrid* AddGhostCellsUniqueCellAssignment(
    vtkUnstructuredGrid* myGrid, vtkPDistributedDataFilterSTLCloak* globalToLocalMap);
  vtkUnstructuredGrid* AddGhostCellsDuplicateCellAssignment(
    vtkUnstructuredGrid* myGrid, vtkPDistributedDataFilterSTLCloak* globalToLocalMap);
  vtkUnstructuredGrid* SetMergeGhostGrid(vtkUnstructuredGrid* ghostCellGrid,
    vtkUnstructuredGrid* incomingGhostCells, int ghostLevel,
    vtkPDistributedDataFilterSTLCloak* idMap);
  //@}

  //@{
  /**
   * ?
   */
  vtkUnstructuredGrid* ExtractCells(vtkIdList* list, int deleteCellLists, vtkDataSet* in);
  vtkUnstructuredGrid* ExtractCells(
    vtkIdList** lists, int nlists, int deleteCellLists, vtkDataSet* in);
  vtkUnstructuredGrid* ExtractZeroCellGrid(vtkDataSet* in);
  //@}

  //@{
  /**
   * ?
   */
  static int GlobalPointIdIsUsed(
    vtkUnstructuredGrid* grid, int ptId, vtkPDistributedDataFilterSTLCloak* globalToLocal);
  static int LocalPointIdIsUsed(vtkUnstructuredGrid* grid, int ptId);
  static vtkIdType FindId(vtkIdTypeArray* ids, vtkIdType gid, vtkIdType startLoc);
  //@}

  /**
   * ?
   */
  static vtkIdTypeArray* AddPointAndCells(vtkIdType gid, vtkIdType localId,
    vtkUnstructuredGrid* grid, vtkIdType* gidCells, vtkIdTypeArray* ids);

  //@{
  /**
   * ?
   */
  static void AddConstantUnsignedCharPointArray(
    vtkUnstructuredGrid* grid, const char* arrayName, unsigned char val);
  static void AddConstantUnsignedCharCellArray(
    vtkUnstructuredGrid* grid, const char* arrayName, unsigned char val);
  //@}

  /**
   * ?
   */
  static void RemoveRemoteCellsFromList(
    vtkIdList* cellList, vtkIdType* gidCells, vtkIdType* remoteCells, vtkIdType nRemoteCells);

  /**
   * ?
   */
  static vtkUnstructuredGrid* MergeGrids(vtkDataSet** sets, int nsets, int deleteDataSets,
    int useGlobalNodeIds, float pointMergeTolerance, int useGlobalCellIds);

private:
  vtkPDistributedDataFilter(const vtkPDistributedDataFilter&) = delete;
  void operator=(const vtkPDistributedDataFilter&) = delete;
};
#endif
