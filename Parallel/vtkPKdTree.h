/*=========================================================================

  Program:   ParaView
  Module:    vtkPKdTree.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

// .NAME vtkPKdTree - Build a k-d tree decomposition of a list of points.
//
// .SECTION Description
//      Build, in parallel, a k-d tree decomposition of one or more
//      vtkDataSets distributed across processors.  We assume each
//      process has read in one portion of a large distributed data set.
//      When done, each process has access to the k-d tree structure, 
//      can obtain information about which process contains 
//      data for each spatial region, and can depth sort the spatial
//      regions.
//
//      This class can also assign spatial regions to processors, based
//      on one of several region assignment schemes.  By default 
//      a contiguous, convex region is assigned to each process.  Several
//      queries return information about how many and what cells I have
//      that lie in a region assigned to another process.
//
// .SECTION See Also
//      vtkKdTree

#ifndef __vtkPKdTree_h
#define __vtkPKdTree_h

#include "vtkKdTree.h"

class vtkMultiProcessController;
class vtkCommunicator;
class vtkSubGroup;
class vtkIntArray;

class VTK_EXPORT vtkPKdTree : public vtkKdTree
{
public:
  vtkTypeRevisionMacro(vtkPKdTree, vtkKdTree);


  void PrintSelf(ostream& os, vtkIndent indent);
  void PrintTiming(ostream& os, vtkIndent indent);
  void PrintTables(ostream& os, vtkIndent indent);

  static vtkPKdTree *New();

  // Description:
  //   Build the spatial decomposition.  Call this explicitly
  //   after changing any parameters affecting the build of the
  //   tree.  It must be called by all processes in the parallel
  //   application, or it will hang.  

  void BuildLocator();

  // Description:
  //   Get the total number of cells distributed across the data
  //   files read by all processes.  You must have called BuildLocator
  //   before calling this method.

  int GetTotalNumberOfCells(){return this->TotalNumCells;}

  // Description:
  //   Create tables of counts of cells per process per region.
  //   These tables can be accessed with queries like
  //   "HasData", "GetProcessCellCountForRegion", and so on.
  //   You must have called BuildLocator() beforehand.  This
  //   method must be called by all processes or it will hang.
  //   Returns 1 on error, 0 when no error.

  int CreateProcessCellCountData();

  // Description:
  //   A convenience function which compiles the global 
  //   bounds of the data arrays across processes.  
  //   These bounds can be accessed with 
  //   "GetCellArrayGlobalRange" and "GetPointArrayGlobalRange".
  //   This method must be called by all processes or it will hang.
  //   Returns 1 on error, 0 when no error.

  int CreateGlobalDataArrayBounds();

  // Description:
  //   Set/Get the number of spatial regions you want to get close
  //   to without going over.  (The number of spatial regions is normally
  //   a power of two.)  Call this before BuildLocator().

  vtkGetMacro(NumRegionsOrLess, int);
  vtkSetMacro(NumRegionsOrLess, int);

  // Description:
  //   Set/Get the number of spatial regions you want to get close
  //   to while having at least this many regions.  (The number of 
  //   spatial regions is normally a power of two.)  Performance
  //   hint:  Request just enough regions so you have at least one
  //   spatial region per processor.  Further subdividing the
  //   space takes time.  Call this before BuildLocator().

  vtkGetMacro(NumRegionsOrMore, int);
  vtkSetMacro(NumRegionsOrMore, int);

  // Description:
  //   Set/Get the communicator object

  void SetController(vtkMultiProcessController *c);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  // Description:
  //   The PKdTree class can assign spatial regions to processors after
  //   building the k-d tree, using one of several partitioning criteria.
  //   These functions Set/Get whether this assignment is computed. 
  //   The default is "Off", no assignment is computed.   If "On", and
  //   no assignment scheme is specified, contiguous assignment will be
  //   computed.  Specifying an assignment scheme (with AssignRegions*())
  //   automatically turns on RegionAssignment.

  vtkGetMacro(RegionAssignment, int);

  static const int NoRegionAssignment;
  static const int ContiguousAssignment;
  static const int UserDefinedAssignment;
  static const int RoundRobinAssignment;

  // Description:
  //   Assign spatial regions to processes via a user defined map.
  //   The user-supplied map is indexed by region ID, and provides a
  //   process ID for each region. 

  int AssignRegions(int *map, int numRegions);

  // Description:
  //   Let the PKdTree class assign a process to each region in a
  //   round robin fashion.  If the k-d tree has not yet been
  //   built, the regions will be assigned after BuildLocator executes.

  int AssignRegionsRoundRobin();

  // Description:
  //    Let the PKdTree class assign a process to each region
  //    by assigning contiguous sets of spatial regions to each
  //    process.  The set of regions assigned to each process will
  //    always have a union that is a convex space (a box).
  //    If the k-d tree has not yet been built, the regions
  //    will be assigned after BuildLocator executes.

  int AssignRegionsContiguous();

  // Description:
  //    Writes the list of region IDs assigned to the specified
  //    process.  Regions IDs start at 0 and increase by 1 from there.
  //    Returns the number of regions in the list. 

  int GetRegionAssignmentList(int procId, vtkIntArray *list);

  // Description:
  //    The k-d tree spatial regions have been assigned to processes.
  //    Given a point on the boundary of one of the regions, this
  //    method creates a list of all processes whose region
  //    boundaries include that point.  This may be required when
  //    looking for processes that have cells adjacent to the cells
  //    of a given process.

  void GetAllProcessesBorderingOnPoint(float x, float y, float z, 
                          vtkIntArray *list);

  // Description:
  //    Returns the ID of the process assigned to the region.

  int GetProcessAssignedToRegion(int regionId);

  // Description:
  //   Returns 1 if the process has data for the given region,
  //   0 otherwise. 

  int HasData(int processId, int regionId);

  // Description:
  //   Returns the number of cells the specified process has in the
  //   specified region.  

  int GetProcessCellCountForRegion(int processId, int regionId);

  // Description:
  //   Returns the total number of processes that have data
  //   falling within this spatial region. 

  int GetTotalProcessesInRegion(int regionId);

  // Description:
  //   Adds the list of processes having data for the given
  //   region to the supplied list, returns the number of
  //   processes added.

  int GetProcessListForRegion(int regionId, vtkIntArray *processes);

  // Description:
  //   Writes the number of cells each process has for the region
  //   to the supplied list of length len.  Returns the number of
  //   cell counts written.  The order of the cell counts corresponds
  //   to the order of process IDs in the process list returned by
  //   GetProcessListForRegion.

  int GetProcessesCellCountForRegion(int regionId, int *count, int len);

  // Description:
  //   Returns the total number of spatial regions that a given
  //   process has data for. 

  int GetTotalRegionsForProcess(int processId);

  // Description:
  //   Adds the region IDs for which this process has data to
  //   the supplied vtkIntArray.  Retruns the number of regions.

  int GetRegionListForProcess(int processId, vtkIntArray *regions);

  // Description:
  //   Writes to the supplied integer array the number of cells this
  //   process has for each region.  Returns the number of
  //   cell counts written.  The order of the cell counts corresponds
  //   to the order of region IDs in the region list returned by
  //   GetRegionListForProcess.

  int GetRegionsCellCountForProcess(int ProcessId, int *count, int len);

  // Description:
  //   After regions have been assigned to processes, I may want to know
  //   which cells I have that are in the regions assigned to a particular
  //   process.
  //
  //   This method takes a process ID and two vtkIdLists.  It
  //   writes to the first list the IDs of the cells
  //   contained in the process' regions.  (That is, their cell
  //   centroid is contained in the region.)  To the second list it
  //   write the IDs of the cells which intersect the process' regions 
  //   but whose cell centroid lies elsewhere.
  //
  //   The total number of cell IDs written to both lists is returned.  
  //   Either list pointer passed in can be NULL, and it will be ignored. 
  //   If there are multiple data sets, you must specify which data set
  //   you wish cell IDs for.  
  //
  //   The caller should delete these two lists when done.  This method 
  //   uses the cell lists created in vtkKdTree::CreateCellLists().
  //   If the cell lists for the process' regions do not exist, this
  //   method will first build the cell lists for all regions by calling
  //   CreateCellLists().  You must remember to DeleteCellLists() when 
  //   done with all calls to this method, as cell lists can require a 
  //   great deal of memory.  

  vtkIdType GetCellListsForProcessRegions(int ProcessId, int set, 
            vtkIdList *inRegionCells, vtkIdList *onBoundaryCells);
  vtkIdType GetCellListsForProcessRegions(int ProcessId, vtkDataSet *set,
            vtkIdList *inRegionCells, vtkIdList *onBoundaryCells);
  vtkIdType GetCellListsForProcessRegions(int ProcessId, vtkIdList *inRegionCells,
                                    vtkIdList *onBoundaryCells);

  // Description:
  //    The internal process/region cell count tables may require a 
  //    large amount of memory.  This call frees this memory.


  // Description:
  //    Return a list of all processes in order from front to
  //    back, given a vtkCamera

  int DepthOrderAllProcesses(vtkCamera *camera, vtkIntArray *orderedList);

  // Description:
  //    An added feature of vtkPKdTree is that it will calculate the
  //    the global range of field arrays across all processes.  You
  //    call CreateGlobalDataArrayBounds() to do this calculation.
  //    Then the following methods return the ranges.
  //    Returns 1 on error, 0 otherwise.

  int GetCellArrayGlobalRange(int arrayIndex, float range[2]);
  int GetPointArrayGlobalRange(int arrayIndex, float range[2]);
  int GetCellArrayGlobalRange(int arrayIndex, double range[2]);
  int GetPointArrayGlobalRange(int arrayIndex, double range[2]);

  int GetCellArrayGlobalRange(const char *name, float range[2]);
  int GetPointArrayGlobalRange(const char *name, float range[2]);
  int GetCellArrayGlobalRange(const char *name, double range[2]);
  int GetPointArrayGlobalRange(const char *name, double range[2]);

  // Description:
  //   This is a useful function that probably belongs in some
  //   other class.  It takes a list and creates a new sorted 
  //   list of unique IDs.

  static int MakeSortedUnique(int *list, int len, int **newList);

protected:

  vtkPKdTree();
  ~vtkPKdTree();

  void SingleProcessBuildLocator();
  int MultiProcessBuildLocator();

private:

  int NumRegionsOrLess;
  int NumRegionsOrMore;
  int RegionAssignment;

  vtkMultiProcessController *Controller;

  vtkSubGroup *SubGroup;

  int NumProcesses;
  int MyId;

  // basic tables - each region is the responsibility of one process, but
  //                one process may be assigned many regions

  int *RegionAssignmentMap;        // indexed by region ID
  int RegionAssignmentMapLength;
  int **ProcessAssignmentMap;      // indexed by process ID
  int *NumRegionsAssigned;         // indexed by process ID

  int UpdateRegionAssignment();

  // basic tables reflecting the data that was read from disk
  // by each process

  char *DataLocationMap;              // by process, by region

  int *NumProcessesInRegion;          // indexed by region ID
  int **ProcessList;                  // indexed by region ID

  int *NumRegionsInProcess;           // indexed by process ID
  int **RegionList;                   // indexed by process ID

  int **CellCountList;                // indexed by region ID

  double *CellDataMin;           // global range for data arrays
  double *CellDataMax;
  double *PointDataMin;
  double *PointDataMax;
  char **CellDataName;
  char **PointDataName;
  int NumCellArrays;
  int NumPointArrays; 

  // distribution of indices for select operation

  int BuildGlobalIndexLists(int ncells);

  int *StartVal;
  int *EndVal;
  int *NumCells;
  int TotalNumCells;

  // local share of points to be partitioned, and local cache

  int WhoHas(int pos);
  int _whoHas(int L, int R, int pos);
  float *GetLocalVal(int pos);
  float *GetLocalValNext(int pos);
  void SetLocalVal(int pos, float *val);
  void ExchangeVals(int pos1, int pos2);
  void ExchangeLocalVals(int pos1, int pos2);

  float *PtArray;
  float *PtArray2;
  float *CurrentPtArray;
  float *NextPtArray;
  int PtArraySize;

  int *SelectBuffer;

  // Parallel build of k-d tree

  int AllCheckForFailure(int rc, const char *where, const char *how);
  void AllCheckParameters();
  double *VolumeBounds();
  int DivideTest(int L, int R, int level);
  int DivideRegion(vtkKdNode *kd, int L, int level, int tag);
  int BreadthFirstDivide(double *bounds);
  void enQueueNode(vtkKdNode *kd, int L, int level, int tag);

  int Select(int dim, int L, int R);
  void _select(int L, int R, int K, int dim);
  void DoTransfer(int from, int to, int fromIndex, int toIndex, int count);
  int PartitionAboutMyValue(int L, int R, int K, int dim);
  int PartitionAboutOtherValue(int L, int R, float T, int dim);
  int PartitionSubArray(int L, int R, int K, int dim, int p1, int p2);

  int CompleteTree();
#ifdef YIELDS_INCONSISTENT_REGION_BOUNDARIES
  void RetrieveData(vtkKdNode *kd, int *buf);
#else
  void ReduceData(vtkKdNode *kd, int *sources);
  void BroadcastData(vtkKdNode *kd);
#endif

  float *DataBounds(int L, int K, int R);
  void GetLocalMinMax(int L, int R, int me, float *min, float *max);

  static int FillOutTree(vtkKdNode *kd, int level);
  static int ComputeDepth(vtkKdNode *kd);
  static void PackData(vtkKdNode *kd, float *data);
  static void UnpackData(vtkKdNode *kd, float *data);
  static void CheckFixRegionBoundaries(vtkKdNode *tree);

  // list management

  int AllocateDoubleBuffer();
  void FreeDoubleBuffer();
  void SwitchDoubleBuffer();
  int AllocateSelectBuffer();
  void FreeSelectBuffer();

  void InitializeGlobalIndexLists();
  int AllocateAndZeroGlobalIndexLists();
  void FreeGlobalIndexLists();
  void InitializeRegionAssignmentLists();
  int AllocateAndZeroRegionAssignmentLists();
  void FreeRegionAssignmentLists();
  void InitializeProcessDataLists();
  int AllocateAndZeroProcessDataLists();
  void FreeProcessDataLists();
  void InitializeFieldArrayMinMax();
  int AllocateAndZeroFieldArrayMinMax();
  void FreeFieldArrayMinMax();

  void ReleaseTables();

  // Assigning regions to processors

  void AddProcessRegions(int procId, vtkKdNode *kd);
  void BuildRegionListsForProcesses();

  // Gather process/region data totals

  int *CollectLocalRegionProcessData();
  int BuildRegionProcessTables();
  int BuildFieldArrayMinMax();
  void AddEntry(int *list, int len, int id);
  static int BinarySearch(int *list, int len, int which);

  static int FindLocalArrayIndex(const char *n, const char **names, int len);


  vtkPKdTree(const vtkPKdTree&); // Not implemented
  void operator=(const vtkPKdTree&); // Not implemented
};

//BTX
class VTK_EXPORT vtkSubGroup{
public:

  enum {MINOP = 1, MAXOP = 2, SUMOP = 3};

  vtkSubGroup(int p0, int p1, int me, int tag, vtkCommunicator *c);
  ~vtkSubGroup();

  int Gather(int *data, int *to, int length, int root);
  int Gather(char *data, char *to, int length, int root);
  int Gather(float *data, float *to, int length, int root);
  int Broadcast(float *data, int length, int root);
  int Broadcast(double *data, int length, int root);
  int Broadcast(int *data, int length, int root);
  int Broadcast(char *data, int length, int root);
  int ReduceSum(int *data, int *to, int length, int root);
  int ReduceMax(float *data, float *to, int length, int root);
  int ReduceMax(double *data, double *to, int length, int root);
  int ReduceMax(int *data, int *to, int length, int root);
  int ReduceMin(float *data, float *to, int length, int root);
  int ReduceMin(double *data, double *to, int length, int root);
  int ReduceMin(int *data, int *to, int length, int root);

  int AllReduceUniqueList(int *list, int len, int **newList);
  int MergeSortedUnique(int *list1, int len1, int *list2, int len2, int **newList);

  void setGatherPattern(int root, int length);
  int getLocalRank(int processID);

  int Barrier();

  void PrintSubGroup() const;

  int tag;

private:
  int computeFanInTargets();
  void restoreRoot(int rootLoc);
  void moveRoot(int rootLoc);
  void setUpRoot(int root);

  int fanInFrom[20];         // reduce, broadcast
  int fanInTo;
  int nFrom, nTo;

  int sendId;                // gather
  int sendOffset;
  int sendLength;
  int recvId[20];
  int recvOffset[20];
  int recvLength[20];
  int nSend, nRecv;
  int gatherRoot, gatherLength;

  int *members;
  int nmembers;
  int myLocalRank;

  vtkCommunicator *comm;
};
//ETX
#endif
