/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDistributedDataFilter.h

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

// .NAME vtkDistributedDataFilter - Distribute data among processors
//
// .SECTION Description
// This filter redistributes data among processors in a parallel
// application into spatially contiguous vtkUnstructuredGrids.
// The execution model anticipated is that all processes read in
// part of a large vtkDataSet. Each process sets the input of
// filter to be that DataSet. When executed, this filter builds
// in parallel a k-d tree, decomposing the space occupied by the
// distributed DataSet into spatial regions.  It assigns each
// spatial region to a processor.  The data is then redistributed
// and the output is a single vtkUnstructuredGrid containing the
// cells in the process' assigned regions.
//
// This filter is sometimes called "D3" for "distributed data decomposition".
//
// Enhancement: You can set the k-d tree decomposition, rather than
// have D3 compute it.  This allows you to divide a dataset using
// the decomposition computed for another dataset.  Obtain a description
// of the k-d tree cuts this way:
//
//    vtkBSPCuts *cuts = D3Object1->GetKdtree()->GetCuts()
//
// And set it this way:
//
//    D3Object2->GetKdtree()->SetCuts(cuts) 
//
//    It is desirable to have a field array of global node IDs
//    for two reasons:
//
//    1. When merging together sub grids that were distributed
//    across processors, global node IDs can be used to remove
//    duplicate points and significantly reduce the size of the
//    resulting output grid.  If no such array is available,
//    D3 will use a tolerance to merge points, which is much
//    slower.
//
//    2. If ghost cells have been requested, D3 requires a
//    global node ID array in order to request and transfer
//    ghost cells in parallel among the processors.  If there
//    is no global node ID array, D3 will in parallel create
//    a global node ID array, and the time to do this can be
//    significant.
//    
//    If you know the name of a global node ID array in the input
//    dataset, set that name with this method.  If you leave
//    it unset, D3 will search the input data set for certain
//    common names of global node ID arrays.  If none is found,
//    and ghost cells have been requested, D3 will create a
//    temporary global node ID array before aquiring ghost cells.
//   It is also desirable to have global element IDs.  However,
//   if they don't exist D3 can create them relatively quickly.
//   Set the name of the global element ID array if you have it.
//   If it is not set, D3 will search for it using common names.
//   If still not found, D3 will create a temporary array of
//   global element IDs.
//
// .SECTION Caveats
// The Execute() method must be called by all processes in the
// parallel application, or it will hang.  If you are not certain
// that your pipeline will execute identically on all processors,
// you may want to use this filter in an explicit execution mode.
//
// .SECTION See Also
// vtkKdTree vtkPKdTree vtkBSPCuts

#ifndef __vtkDistributedDataFilter_h
#define __vtkDistributedDataFilter_h

#include "vtkUnstructuredGridAlgorithm.h"

class vtkUnstructuredGrid;
class vtkPKdTree;
class vtkMultiProcessController;
class vtkDataArray;
class vtkIdTypeArray;
class vtkIntArray;
class vtkFloatArray;
class vtkIdList;
class vtkUnstructuredGrid;
class vtkModelMetadata;
class vtkDistributedDataFilterSTLCloak;

class VTK_PARALLEL_EXPORT vtkDistributedDataFilter: public vtkUnstructuredGridAlgorithm
{
  vtkTypeRevisionMacro(vtkDistributedDataFilter, 
    vtkUnstructuredGridAlgorithm);

public:
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkDistributedDataFilter *New();

  // Description:
  //   Set/Get the communicator object
  void SetController(vtkMultiProcessController *c);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  // Description:
  //    When this filter executes, it creates a vtkPKdTree (K-d tree)
  //    data structure in parallel which divides the total distributed 
  //    data set into spatial regions.  The K-d tree object also creates 
  //    tables describing which processes have data for which 
  //    regions.  Only then does this filter redistribute 
  //    the data according to the region assignment scheme.  By default, 
  //    the K-d tree structure and it's associated tables are deleted
  //    after the filter executes.  If you anticipate changing only the
  //    region assignment scheme (input is unchanged) and explicitly
  //    re-executing, then RetainKdTreeOn, and the K-d tree structure and
  //    tables will be saved.  Then, when you re-execute, this filter will
  //    skip the k-d tree build phase and go straight to redistributing
  //    the data according to region assignment.  See vtkPKdTree for
  //    more information about region assignment.

  vtkBooleanMacro(RetainKdtree, int);
  vtkGetMacro(RetainKdtree, int);
  vtkSetMacro(RetainKdtree, int);

  // Description:
  //   Get a pointer to the parallel k-d tree object.  Required for changing
  //   default behavior for region assignment, changing default depth of tree,
  //   or other tree building default parameters.  See vtkPKdTree and 
  //   vtkKdTree for more information about these options.

  vtkPKdTree *GetKdtree();

  // Description:
  //   Each cell in the data set is associated with one of the
  //   spatial regions of the k-d tree decomposition.  In particular,
  //   the cell belongs to the region that it's centroid lies in.
  //   When the new vtkUnstructuredGrid is created, by default it
  //   is composed of the cells associated with the region(s)
  //   assigned to this process.  If you also want it to contain
  //   cells that intersect these regions, but have their centroid
  //   elsewhere, then set this variable on.  By default it is off.

  vtkBooleanMacro(IncludeAllIntersectingCells, int);
  vtkGetMacro(IncludeAllIntersectingCells, int);
  vtkSetMacro(IncludeAllIntersectingCells, int);

  // Description:
  //   Set this variable if you want the cells of the output
  //   vtkUnstructuredGrid to be clipped to the spatial region
  //   boundaries.  By default this is off.

  vtkBooleanMacro(ClipCells, int);
  vtkGetMacro(ClipCells, int);
  vtkSetMacro(ClipCells, int);

//BTX
  enum BoundaryModes {
    ASSIGN_TO_ONE_REGION=0,
    ASSIGN_TO_ALL_INTERSECTING_REGIONS=1,
    SPLIT_BOUNDARY_CELLS=2
  };
//ETX

  // Description:
  // Handling of ClipCells and IncludeAllIntersectingCells.
  void SetBoundaryMode(int mode);
  void SetBoundaryModeToAssignToOneRegion()
    { this->SetBoundaryMode(vtkDistributedDataFilter::ASSIGN_TO_ONE_REGION); }
  void SetBoundaryModeToAssignToAllIntersectingRegions()
    { this->SetBoundaryMode(
      vtkDistributedDataFilter::ASSIGN_TO_ALL_INTERSECTING_REGIONS);
    }
  void SetBoundaryModeToSplitBoundaryCells()
    { this->SetBoundaryMode(vtkDistributedDataFilter::SPLIT_BOUNDARY_CELLS); }
  int GetBoundaryMode();
  
  // Description:
  //   Ensure previous filters don't send up ghost cells
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  // Description:
  //  This class does a great deal of all-to-all communication
  //  when exchanging portions of data sets and building new sub 
  //  grids.
  //  By default it will do fast communication.  It can instead
  //  use communication routines that use the least possible
  //  amount of memory, but these are slower.  Set this option
  //  ON to choose these latter routines.

  vtkBooleanMacro(UseMinimalMemory, int);
  vtkGetMacro(UseMinimalMemory, int);
  vtkSetMacro(UseMinimalMemory, int);


  // Description:
  //  Turn on collection of timing data

  vtkBooleanMacro(Timing, int);
  vtkSetMacro(Timing, int);
  vtkGetMacro(Timing, int);

  // Description:
  // Consider the MTime of the KdTree.
  unsigned long GetMTime();

protected:
  vtkDistributedDataFilter();
  ~vtkDistributedDataFilter();

  // Description:
  //  Another way to set ClipCells and IncludeAllIntersectingCells.
  //  AssignBoundaryCellsToOneRegion turns off both ClipCells and
  //  IncludeAllIntersectingCells.  Each cell will be included in
  //  exactly one process' output unstructured grid.  

  void AssignBoundaryCellsToOneRegionOn();
  void AssignBoundaryCellsToOneRegionOff();
  void SetAssignBoundaryCellsToOneRegion(int val);

  // Description:
  //  Another way to set ClipCells and IncludeAllIntersectingCells.
  //  AssignBoundaryCellsToAllIntersectingRegions turns off ClipCells 
  //  turns on IncludeAllIntersectingCells.  A cell will be included
  //  in the output unstructured grid built for every region that it
  //  intersects.  If a cell intersects two process' spatial regions,
  //  both processes will have that cell in their output grid.

  void AssignBoundaryCellsToAllIntersectingRegionsOn();
  void AssignBoundaryCellsToAllIntersectingRegionsOff();
  void SetAssignBoundaryCellsToAllIntersectingRegions(int val);

  // Description:
  //  Another way to set ClipCells and IncludeAllIntersectingCells.
  //  DivideBoundaryCells turns on both ClipCells and
  //  IncludeAllIntersectingCells.  A cell that straddles a processor
  //  boundary will be split along the boundary, with each process
  //  getting the portion of the cell that lies in it's spatial region.

  void DivideBoundaryCellsOn();
  void DivideBoundaryCellsOff();
  void SetDivideBoundaryCells(int val);

  // Description:
  //   Build a vtkUnstructuredGrid for a spatial region from the 
  //   data distributed across processes.  Execute() must be called
  //   by all processes, or it will hang.

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  void SingleProcessExecute(vtkDataSet *input, vtkUnstructuredGrid *output);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);

private:

//BTX
  enum{
      DeleteNo = 0,
      DeleteYes = 1
      };

  enum{
      DuplicateCellsNo = 0,
      DuplicateCellsYes = 1
      };

  enum{
      GhostCellsNo = 0,
      GhostCellsYes = 1
      };

  enum{
      UnsetGhostLevel = 99
      };
//ETX
  int PartitionDataAndAssignToProcesses(vtkDataSet *set);
  vtkUnstructuredGrid *RedistributeDataSet(vtkDataSet *set, vtkDataSet *input);
  int ClipGridCells(vtkUnstructuredGrid *grid);
  vtkUnstructuredGrid * AcquireGhostCells(vtkUnstructuredGrid *grid);

  void ComputeMyRegionBounds();

  int CheckFieldArrayTypes(vtkDataSet *set);

  vtkDataSet *TestFixTooFewInputFiles(vtkDataSet *input);

  vtkUnstructuredGrid *MPIRedistribute(vtkDataSet *in, vtkDataSet *input);

  vtkIdList **GetCellIdsForProcess(int proc, int *nlists);

  void SetUpPairWiseExchange();
  void FreeIntArrays(vtkIdTypeArray **ar);
  vtkIdTypeArray *ExchangeCounts(vtkIdType myCount, int tag);
  vtkIdTypeArray **ExchangeIntArrays(vtkIdTypeArray **arIn, 
                                  int deleteSendArrays, int tag);
  vtkFloatArray **ExchangeFloatArrays(vtkFloatArray **myArray, 
                                      int deleteSendArrays, int tag);
  vtkUnstructuredGrid *ExchangeMergeSubGrids(vtkIdList **cellIds, int deleteCellIds,
                         vtkDataSet *myGrid, int deleteMyGrid,
                         int filterOutDuplicateCells, int ghostCellFlag, int tag);
  vtkUnstructuredGrid *ExchangeMergeSubGrids(vtkIdList ***cellIds, int *numLists, 
                   int deleteCellIds,
                   vtkDataSet *myGrid, int deleteMyGrid,
                   int filterOutDuplicateCells, int ghostCellFlag, int tag);
  vtkIdTypeArray *ExchangeCountsLean(vtkIdType myCount, int tag);
  vtkIdTypeArray **ExchangeIntArraysLean(vtkIdTypeArray **arIn, 
                                         int deleteSendArrays, int tag);
  vtkFloatArray **ExchangeFloatArraysLean(vtkFloatArray **myArray, 
                                      int deleteSendArrays, int tag);
  vtkUnstructuredGrid *ExchangeMergeSubGridsLean(
                   vtkIdList ***cellIds, int *numLists, 
                   int deleteCellIds,
                   vtkDataSet *myGrid, int deleteMyGrid,
                   int filterOutDuplicateCells, int ghostCellFlag, int tag);
  vtkIdTypeArray *ExchangeCountsFast(vtkIdType myCount, int tag);
  vtkIdTypeArray **ExchangeIntArraysFast(vtkIdTypeArray **arIn, 
                                         int deleteSendArrays, int tag);
  vtkFloatArray **ExchangeFloatArraysFast(vtkFloatArray **myArray, 
                                      int deleteSendArrays, int tag);
  vtkUnstructuredGrid *ExchangeMergeSubGridsFast(
                   vtkIdList ***cellIds, int *numLists, 
                   int deleteCellIds,
                   vtkDataSet *myGrid, int deleteMyGrid,
                   int filterOutDuplicateCells, int ghostCellFlag, int tag);


  char *MarshallDataSet(vtkUnstructuredGrid *extractedGrid, int &size);
  vtkUnstructuredGrid *UnMarshallDataSet(char *buf, int size);

  void ClipCellsToSpatialRegion(vtkUnstructuredGrid *grid);

  void ClipWithVtkClipDataSet(vtkUnstructuredGrid *grid, double *bounds,
           vtkUnstructuredGrid **outside, vtkUnstructuredGrid **inside);
  void ClipWithBoxClipDataSet(vtkUnstructuredGrid *grid, double *bounds,
           vtkUnstructuredGrid **outside, vtkUnstructuredGrid **inside);

  vtkIdTypeArray *GetGlobalNodeIdArray(vtkDataSet *set);
  vtkIdType *GetGlobalNodeIds(vtkDataSet *set);
  vtkIdTypeArray *GetGlobalElementIdArray(vtkDataSet *set);
  vtkIdType *GetGlobalElementIds(vtkDataSet *set);

  int AssignGlobalNodeIds(vtkUnstructuredGrid *grid);
  int AssignGlobalElementIds(vtkDataSet *in);

  vtkIdTypeArray **FindGlobalPointIds(vtkFloatArray **ptarray,
    vtkIdTypeArray *ids, vtkUnstructuredGrid *grid, int &numUniqueMissingPoints);

  int InMySpatialRegion(float x, float y, float z);
  int InMySpatialRegion(double x, double y, double z);
  int StrictlyInsideMyBounds(float x, float y, float z);
  int StrictlyInsideMyBounds(double x, double y, double z);

  vtkIdTypeArray **GetGhostPointIds(int ghostLevel, vtkUnstructuredGrid *grid,
                                    int AddCellsIAlreadyHave);
  vtkIdTypeArray **MakeProcessLists(vtkIdTypeArray **pointIds,
                                 vtkDistributedDataFilterSTLCloak *procs);
  vtkUnstructuredGrid *AddGhostCellsUniqueCellAssignment(
                           vtkUnstructuredGrid *myGrid,
                           vtkDistributedDataFilterSTLCloak *globalToLocalMap);
  vtkUnstructuredGrid *AddGhostCellsDuplicateCellAssignment(
                           vtkUnstructuredGrid *myGrid,
                           vtkDistributedDataFilterSTLCloak *globalToLocalMap);
  vtkIdList **BuildRequestedGrids( vtkIdTypeArray **globalPtIds,
                        vtkUnstructuredGrid *grid,
                        vtkDistributedDataFilterSTLCloak *ptIdMap);
  vtkUnstructuredGrid *SetMergeGhostGrid(
                       vtkUnstructuredGrid *ghostCellGrid,
                       vtkUnstructuredGrid *incomingGhostCells,
                       int ghostLevel, vtkDistributedDataFilterSTLCloak *idMap);

  vtkUnstructuredGrid *ExtractCells(vtkIdList *list, 
                  int deleteCellLists, vtkDataSet *in, vtkModelMetadata *mmd);
  vtkUnstructuredGrid *ExtractCells(vtkIdList **lists, int nlists, 
                  int deleteCellLists, vtkDataSet *in, vtkModelMetadata *mmd);
  vtkUnstructuredGrid *ExtractZeroCellGrid(vtkDataSet *in,
                  vtkModelMetadata *mmd);

  void AddMetadata(vtkUnstructuredGrid *grid, vtkModelMetadata *mmd);

  static int GlobalPointIdIsUsed(vtkUnstructuredGrid *grid,
               int ptId, vtkDistributedDataFilterSTLCloak *globalToLocal);

  static int LocalPointIdIsUsed(vtkUnstructuredGrid *grid, int ptId);
  static int FindId(vtkIdTypeArray *ids, vtkIdType gid, vtkIdType startLoc);
  static vtkIdTypeArray *AddPointAndCells(vtkIdType gid, 
                                       vtkIdType localId, 
                                       vtkUnstructuredGrid *grid, 
                                       vtkIdType *gidCells, 
                                       vtkIdTypeArray *ids);

  static void AddConstantUnsignedCharPointArray(vtkUnstructuredGrid *grid, 
                                 const char *arrayName, unsigned char val);
  static void AddConstantUnsignedCharCellArray(vtkUnstructuredGrid *grid, 
                                 const char *arrayName, unsigned char val);
  static void RemoveRemoteCellsFromList(vtkIdList *cellList, 
                                        vtkIdType *gidCells, 
                                        int *remoteCells, 
                                        int nRemoteCells);
  static vtkUnstructuredGrid *MergeGrids(vtkDataSet **sets, int nsets,
                                         int deleteDataSets,
                                         int useGlobalNodeIds, float pointMergeTolerance,
                                         int useGlobalCellIds);

  static void FreeIdLists(vtkIdList**lists, int nlists);
  static vtkIdType GetIdListSize(vtkIdList**lists, int nlists);
  static int HasMetadata(vtkDataSet *s);

  vtkPKdTree *Kdtree;
  vtkMultiProcessController *Controller;

  int NumProcesses;
  int MyId;

  int *Target;
  int *Source;

  int NumConvexSubRegions;
  double *ConvexSubRegionBounds;

  int GhostLevel;

  int RetainKdtree;
  int IncludeAllIntersectingCells;
  int ClipCells;
  int AssignBoundaryCellsToOneRegion;
  int AssignBoundaryCellsToAllIntersectingRegions;
  int DivideBoundaryCells;

  int Timing;

  int NextProgressStep;
  double ProgressIncrement;

  int UseMinimalMemory;

  vtkDistributedDataFilter(const vtkDistributedDataFilter&); // Not implemented
  void operator=(const vtkDistributedDataFilter&); // Not implemented
};
#endif
