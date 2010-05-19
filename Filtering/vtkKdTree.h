/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKdTree.h

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

// .NAME vtkKdTree - a Kd-tree spatial decomposition of a set of points
//
// .SECTION Description
//     Given one or more vtkDataSets, create a load balancing
//     k-d tree decomposition of the points at the center of the cells.
//     Or, create a k-d tree point locator from a list of points.
//
//     This class can also generate a PolyData representation of
//     the boundaries of the spatial regions in the decomposition.
//
//     It can sort the regions with respect to a viewing direction,
//     and it can decompose a list of regions into subsets, each
//     of which represent a convex spatial region (since many algorithms
//     require a convex region).  
//
//     If the points were derived from cells, vtkKdTree
//     can create a list of cell Ids for each region for each data set.  
//     Two lists are available - all cells with centroid in the region, 
//     and all cells that intersect the region but whose centroid lies 
//     in another region.
//
//     For the purpose of removing duplicate points quickly from large
//     data sets, or for finding nearby points, we added another mode for 
//     building the locator.  BuildLocatorFromPoints will build a k-d tree
//     from one or more vtkPoints objects.  This can be followed by
//     BuildMapForDuplicatePoints which returns a mapping from the original
//     ids to a subset of the ids that is unique within a supplied
//     tolerance, or you can use FindPoint and FindClosestPoint to
//     locate points in the original set that the tree was built from.
//
// .SECTION See Also
//      vtkLocator vtkCellLocator vtkPKdTree

#ifndef __vtkKdTree_h
#define __vtkKdTree_h

#include "vtkLocator.h"

class vtkTimerLog;
class vtkIdList;
class vtkIdTypeArray;
class vtkIntArray;
class vtkPointSet;
class vtkPoints;
class vtkCellArray;
class vtkCell;
class vtkKdNode;
class vtkBSPCuts;
class vtkBSPIntersections;
class vtkDataSetCollection;

class VTK_FILTERING_EXPORT vtkKdTree : public vtkLocator
{
public:
  vtkTypeMacro(vtkKdTree, vtkLocator);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkKdTree *New();

  // Description:
  //  Turn on timing of the k-d tree build
  vtkBooleanMacro(Timing, int);
  vtkSetMacro(Timing, int);
  vtkGetMacro(Timing, int);

  // Description:
  //  Minimum number of cells per spatial region.  Default is 100.
  vtkSetMacro(MinCells, int);
  vtkGetMacro(MinCells, int);

  // Description:
  //   Set/Get the number of spatial regions you want to get close
  //   to without going over.  (The number of spatial regions is normally
  //   a power of two.)  Call this before BuildLocator().  Default
  //   is unset (0).

  vtkGetMacro(NumberOfRegionsOrLess, int);
  vtkSetMacro(NumberOfRegionsOrLess, int);

  // Description:
  //   Set/Get the number of spatial regions you want to get close
  //   to while having at least this many regions.  (The number of
  //   spatial regions is normally a power of two.)   Default
  //   is unset (0).

  vtkGetMacro(NumberOfRegionsOrMore, int);
  vtkSetMacro(NumberOfRegionsOrMore, int);
  
  // Description:
  //  Some algorithms on k-d trees require a value that is a very
  //  small distance relative to the diameter of the entire space
  //  divided by the k-d tree.  This factor is the maximum axis-aligned
  //  width of the space multipled by 10e-6.

  vtkGetMacro(FudgeFactor, double);
  vtkSetMacro(FudgeFactor, double);

  // Description:
  //   Get a vtkBSPCuts object, a general object representing an axis-
  //   aligned spatial partitioning.  Used by vtkBSPIntersections.

  vtkGetObjectMacro(Cuts, vtkBSPCuts);

  // Description:
  //   Normally the k-d tree is computed from the dataset(s) provided
  //   in SetDataSet.  Alternatively, you can provide the cuts that will
  //   be applied by calling SetCuts.

  void SetCuts(vtkBSPCuts *cuts);

  // Description:   
  //    Omit partitions along the X axis, yielding shafts in the X direction
  void OmitXPartitioning();
  
  // Description:
  //    Omit partitions along the Y axis, yielding shafts in the Y direction
  void OmitYPartitioning();

  // Description:
  //    Omit partitions along the Z axis, yielding shafts in the Z direction
  void OmitZPartitioning();

  // Description:
  //    Omit partitions along the X and Y axes, yielding slabs along Z
  void OmitXYPartitioning();

  // Description:
  //    Omit partitions along the Y and Z axes, yielding slabs along X
  void OmitYZPartitioning();
  
  // Description:
  //    Omit partitions along the Z and X axes, yielding slabs along Y
  void OmitZXPartitioning();

  // Description:
  //    Partition along all three axes - this is the default
  void OmitNoPartitioning();

  //  Description
  //     This class can compute a spatial decomposition based on the
  //     cells in a list of one or more input data sets.
  //     SetDataSet sets the first data set in the list to the named set.
  //     SetNthDataSet sets the data set at index N to the data set named.
  //     RemoveData set takes either the data set itself or an index and
  //   removes that data set from the list of data sets.
  //     AddDataSet adds a data set to the list of data sets.

  // Description:
  // Clear out all data sets and replace with single data set.  For backward
  // compatibility with superclass.
  virtual void SetDataSet(vtkDataSet *set);

  // Description:
  // This class can compute a spatial decomposition based on the cells in a list
  // of one or more input data sets.  Add them one at a time with this method.
  virtual void AddDataSet(vtkDataSet *set);

  // Description:
  // Remove the given data set.
  virtual void RemoveDataSet(int index);
  virtual void RemoveDataSet(vtkDataSet *set);
  virtual void RemoveAllDataSets();

  // Description:
  //   Get the number of data sets included in spatial paritioning
  int GetNumberOfDataSets();

  // Description:
  //   Get the nth defined data set in the spatial partitioning.
  //   (If you used SetNthDataSet to define 0,1 and 3 and ask for
  //   data set 2, you get 3.)

  // Description:
  // Return the n'th data set.
  vtkDataSet *GetDataSet(int n);

  // Description:
  // Return the 0'th data set.  For compatability with the superclass'
  // interface.
  vtkDataSet *GetDataSet(){ return this->GetDataSet(0); }

  // Description:
  // Return a collection of all the data sets.
  vtkGetObjectMacro(DataSets, vtkDataSetCollection);

  // Description:
  // Return the index of the given data set.  Returns -1 if that data
  // set does not exist.
  int GetDataSetIndex(vtkDataSet *set);

  // Description:
  //   Get the spatial bounds of the entire k-d tree space. Sets
  //    bounds array to xmin, xmax, ymin, ymax, zmin, zmax.
  void GetBounds(double *bounds);

  // Description:
  //   There are certain applications where you want the bounds of
  //   the k-d tree space to be at least as large as a specified
  //   box.  If the k-d tree has been built, you can expand it's 
  //   bounds with this method.  If the bounds supplied are smaller
  //   than those computed, they will be ignored.

  void SetNewBounds(double *bounds);

  // Description:
  //   The number of leaf nodes of the tree, the spatial regions
  vtkGetMacro(NumberOfRegions, int);

  // Description:
  //   Get the spatial bounds of k-d tree region
  void GetRegionBounds(int regionID, double bounds[6]);

  // Description:
  //    Get the bounds of the data within the k-d tree region
  void GetRegionDataBounds(int regionID, double bounds[6]);

  // Description:
  //    Print out nodes of kd tree
  void PrintTree();
  void PrintVerboseTree();
  
  // Description:
  //    Print out leaf node data for given id
  void PrintRegion(int id);
  
  // Description:
  //   Create a list for each of the requested regions, listing
  //   the IDs of all cells whose centroid falls in the region.
  //   These lists are obtained with GetCellList().
  //   If no DataSet is specified, the cell list is created
  //   for DataSet 0.  If no list of requested regions is provided,
  //   the cell lists for all regions are created.  
  //
  //   When CreateCellLists is called again, the lists created
  //   on the previous call  are deleted.
  
  void CreateCellLists(int dataSetIndex, int *regionReqList, 
                       int reqListSize);
  void CreateCellLists(vtkDataSet *set, int *regionReqList,
                       int reqListSize);
  void CreateCellLists(int *regionReqList, int listSize);
  void CreateCellLists(); 
  
  // Description:
  //   If IncludeRegionBoundaryCells is ON,
  //   CreateCellLists() will also create a list of cells which
  //   intersect a given region, but are not assigned
  //   to the region.  These lists are obtained with 
  //   GetBoundaryCellList().  Default is OFF.
  vtkSetMacro(IncludeRegionBoundaryCells, int);
  vtkGetMacro(IncludeRegionBoundaryCells, int);
  vtkBooleanMacro(IncludeRegionBoundaryCells, int);

  // Description:
  //    Free the memory used by the cell lists.
  void DeleteCellLists();

  // Description:
  //    Get the cell list for a region.  This returns a pointer
  //    to vtkKdTree's memory, so don't free it.
  vtkIdList *GetCellList(int regionID);

  // Description:
  //    The cell list obtained with GetCellList is the list
  //    of all cells such that their centroid is contained in
  //    the spatial region.  It may also be desirable to get
  //    a list of all cells intersecting a spatial region,
  //    but with centroid in some other region.  This is that
  //    list.  This list is computed in CreateCellLists() if
  //    and only if IncludeRegionBoundaryCells is ON.  This
  //    returns a pointer to KdTree's memory, so don't free it.
  vtkIdList *GetBoundaryCellList(int regionID);

  // Description:
  //   
  //   For a list of regions, get two cell lists.  The first lists
  //   the IDs  all cells whose centroids lie in one of the regions.
  //   The second lists the IDs of all cells that intersect the regions,
  //   but whose centroid lies in a region not on the list.
  //
  //   The total number of cell IDs written to both lists is returned.
  //   Either list pointer passed in can be NULL, and it will be ignored.
  //   If there are multiple data sets, you must specify which data set
  //   you wish cell IDs for.
  //
  //   The caller should delete these two lists when done.  This method
  //   uses the cell lists created in CreateCellLists().
  //   If the cell list for any of the requested regions does not
  //   exist, then this method will call CreateCellLists() to create
  //   cell lists for *every* region of the k-d tree.  You must remember 
  //   to DeleteCellLists() when done with all calls to this method, as 
  //   cell lists can require a great deal of memory.
  vtkIdType GetCellLists(vtkIntArray *regions, int set, 
                   vtkIdList *inRegionCells, vtkIdList *onBoundaryCells);
  vtkIdType GetCellLists(vtkIntArray *regions, vtkDataSet *set,
            vtkIdList *inRegionCells, vtkIdList *onBoundaryCells);
  vtkIdType GetCellLists(vtkIntArray *regions, vtkIdList *inRegionCells,
                                    vtkIdList *onBoundaryCells);
  
  // Description:
  //    Get the id of the region containing the cell centroid.  If
  //    no DataSet is specified, assume DataSet 0.  If you need the
  //    region ID for every cell, use AllGetRegionContainingCell
  //    instead.  It is more efficient.
  int GetRegionContainingCell(vtkDataSet *set, vtkIdType cellID);
  int GetRegionContainingCell(int set, vtkIdType cellID);
  int GetRegionContainingCell(vtkIdType cellID);

  // Description:
  //    Get a list (in order by data set by cell id) of the
  //    region IDs of the region containing the centroid for
  //    each cell.
  //    This is faster than calling GetRegionContainingCell
  //    for each cell in the DataSet.
  //    vtkKdTree uses this list, so don't delete it.
  int *AllGetRegionContainingCell();

  // Description:
  //    Get the id of the region containing the specified location.
  int GetRegionContainingPoint(double x, double y, double z);
  
  // Description:
  // Create the k-d tree decomposition of the cells of the data set
  // or data sets.  Cells are assigned to k-d tree spatial regions
  // based on the location of their centroids.
  void BuildLocator();

  // Description:
  //   Given a list of region IDs, determine the decomposition of
  //   these regions into the minimal number of convex subregions.  Due
  //   to the way the k-d tree is constructed, those convex subregions
  //   will be axis-aligned boxes.  Return the minimal number of
  //   such convex regions that compose the original region list.
  //   This call will set convexRegionBounds to point to a list
  //   of the bounds of these regions.  Caller should free this.
  //   There will be six values for each convex subregion (xmin,
  //   xmax, ymin, ymax, zmin, zmax).  If the regions in the
  //   regionIdList form a box already, a "1" is returned and the
  //   second argument contains the bounds of the box.

  int MinimalNumberOfConvexSubRegions(vtkIntArray *regionIdList,
                                      double **convexRegionBounds);

  // Description:
  // DO NOT CALL.  Depricated in VTK 5.2.  Use ViewOrderAllRegionsInDirection
  // or ViewOrderAllRegionsFromPosition.
  VTK_LEGACY(int DepthOrderAllRegions(double *dop, vtkIntArray *orderedList));

  // Description:
  // DO NOT CALL.  Depricated in VTK 5.2.  Use ViewOrderRegionsInDirection
  // or ViewOrderRegionsFromPosition.
  VTK_LEGACY(int DepthOrderRegions(vtkIntArray *regionIds, double *dop,
                                   vtkIntArray *orderedList));

  // Description:
  // Given a direction of projection (typically obtained with
  // vtkCamera::GetDirectionOfProjection()), this method, creates a list of the
  // k-d tree region IDs in order from front to back with respect to that
  // direction.  The number of ordered regions is returned.  Use this method to
  // view order regions for cameras that use parallel projection.
  int ViewOrderAllRegionsInDirection(const double directionOfProjection[3],
                                     vtkIntArray *orderedList);

  // Description:
  // Given a direction of projection and a list of k-d tree region IDs, this
  // method, creates a list of the k-d tree region IDs in order from front to
  // back with respect to that direction.  The number of ordered regions is
  // returned.  Use this method to view order regions for cameras that use
  // parallel projection.
  int ViewOrderRegionsInDirection(vtkIntArray *regionIds,
                                  const double directionOfProjection[3],
                                  vtkIntArray *orderedList);

  // Description:
  // Given a camera position (typically obtained with vtkCamera::GetPosition()),
  // this method, creates a list of the k-d tree region IDs in order from front
  // to back with respect to that direction.  The number of ordered regions is
  // returned.  Use this method to view order regions for cameras that use
  // perspective projection.
  int ViewOrderAllRegionsFromPosition(const double directionOfProjection[3],
                                      vtkIntArray *orderedList);

  // Description:
  // Given a camera position and a list of k-d tree region IDs, this method,
  // creates a list of the k-d tree region IDs in order from front to back with
  // respect to that direction.  The number of ordered regions is returned.  Use
  // this method to view order regions for cameras that use perspective
  // projection.
  int ViewOrderRegionsFromPosition(vtkIntArray *regionIds,
                                   const double directionOfProjection[3],
                                   vtkIntArray *orderedList);

  // Description:
  // This is a special purpose locator that builds a k-d tree to 
  // find duplicate and near-by points.  It builds the tree from 
  // one or more vtkPoints objects instead of from the cells of
  // a vtkDataSet.  This build would normally be followed by
  // BuildMapForDuplicatePoints, FindPoint, or FindClosestPoint.
  // Since this will build a normal k-d tree, all the region intersection
  // queries will still work, as will most other calls except those that
  // have "Cell" in the name.
  //
  // This method works most efficiently when the point arrays are
  // float arrays.
  void BuildLocatorFromPoints(vtkPointSet *pointset);
  void BuildLocatorFromPoints(vtkPoints *ptArray);
  void BuildLocatorFromPoints(vtkPoints **ptArray, int numPtArrays);
  
  // Description:
  // This call returns a mapping from the original point IDs supplied
  // to BuildLocatorFromPoints to a subset of those IDs that is unique 
  // within the specified tolerance.  
  // If points 2, 5, and 12 are the same, then 
  // IdMap[2] = IdMap[5] = IdMap[12] = 2 (or 5 or 12).
  //
  // "original point IDs" - For point IDs we start at 0 for the first
  // point in the first vtkPoints object, and increase by 1 for subsequent
  // points and subsequent vtkPoints objects.
  //
  // You must have called BuildLocatorFromPoints() before calling this.
  // You are responsible for deleting the returned array.
  vtkIdTypeArray *BuildMapForDuplicatePoints(float tolerance);

  // Description:
  // Find the Id of the point that was previously supplied
  // to BuildLocatorFromPoints().  Returns -1 if the point
  // was not in the original array.
  vtkIdType FindPoint(double *x);
  vtkIdType FindPoint(double x, double y, double z);

  // Description:
  // Find the Id of the point that was previously supplied
  // to BuildLocatorFromPoints() which is closest to the given point.
  // Set the square of the distance between the two points.
  vtkIdType FindClosestPoint(double *x, double &dist2);
  vtkIdType FindClosestPoint(double x, double y, double z, double &dist2);

  // Description:
  // Given a position x and a radius r, return the id of the point 
  // closest to the point in that radius.
  // dist2 returns the squared distance to the point.
  vtkIdType FindClosestPointWithinRadius(
    double radius, const double x[3], double& dist2);

  // Description:
  // Find the Id of the point in the given region which is
  // closest to the given point.  Return the ID of the point,
  // and set the square of the distance of between the points.
  vtkIdType FindClosestPointInRegion(int regionId, double *x, double &dist2);
  vtkIdType FindClosestPointInRegion(int regionId, double x, double y, double z, 
                                     double &dist2);

  // Description:
  // Find all points within a specified radius R of position x.
  // The result is not sorted in any specific manner.
  // These methods are thread safe if BuildLocator() is directly or
  // indirectly called from a single thread first.
  void FindPointsWithinRadius(double R, const double x[3], vtkIdList *result);

  // Description:
  // Find the closest N points to a position. This returns the closest
  // N points to a position. A faster method could be created that returned
  // N close points to a position, but necessarily the exact N closest.
  // The returned points are sorted from closest to farthest.
  // These methods are thread safe if BuildLocator() is directly or
  // indirectly called from a single thread first.
  void FindClosestNPoints(int N, const double x[3], vtkIdList *result);

  // Description:
  // Get a list of the original IDs of all points in a region.  You
  // must have called BuildLocatorFromPoints before calling this.
  vtkIdTypeArray *GetPointsInRegion(int regionId);

  // Description:
  // Delete the k-d tree data structure. Also delete any
  // cell lists that were computed with CreateCellLists().
  void FreeSearchStructure();
  
  // Description:
  // Create a polydata representation of the boundaries of
  // the k-d tree regions.  If level equals GetLevel(), the
  // leaf nodes are represented.
  void GenerateRepresentation(int level, vtkPolyData *pd);
  
  // Description:
  //    Generate a polygonal representation of a list of regions.
  //    Only leaf nodes have region IDs, so these will be leaf nodes.
  void GenerateRepresentation(int *regionList, int len, vtkPolyData *pd);

  // Description:
  //    The polydata representation of the k-d tree shows the boundaries
  //    of the k-d tree decomposition spatial regions.  The data inside
  //    the regions may not occupy the entire space.  To draw just the
  //    bounds of the data in the regions, set this variable ON.
  vtkBooleanMacro(GenerateRepresentationUsingDataBounds, int);
  vtkSetMacro(GenerateRepresentationUsingDataBounds, int);
  vtkGetMacro(GenerateRepresentationUsingDataBounds, int);

  // Description:
  //    Print timing of k-d tree build
  virtual void PrintTiming(ostream& os, vtkIndent indent);

  // Description:
  //    Return 1 if the geometry of the input data sets
  //    has changed since the last time the k-d tree was built.
  virtual int NewGeometry();

  // Description:
  //    Return 1 if the geometry of these data sets differs
  //    for the geometry of the last data sets used to build
  //    the k-d tree.
  virtual int NewGeometry(vtkDataSet **sets, int numDataSets);

  // Description:
  // Forget about the last geometry used.  The next call to NewGeometry will
  // return 1.  A new k-d tree will be built the next time BuildLocator is
  // called.
  virtual void InvalidateGeometry();

  // Description:
  //    Create a copy of the binary tree representation of the
  //    k-d tree spatial partitioning provided.  

  static vtkKdNode *CopyTree(vtkKdNode *kd);

  // Description:
  // Fill ids with points found in area.  The area is a 6-tuple containing
  // (xmin, xmax, ymin, ymax, zmin, zmax).
  // This method will clear the array by default.  To append ids to an array,
  // set clearArray to false.
  void FindPointsInArea(double* area, vtkIdTypeArray* ids, bool clearArray = true);

protected:

  vtkKdTree();
  ~vtkKdTree();

  vtkBSPIntersections *BSPCalculator;
  int UserDefinedCuts;

  void SetCalculator(vtkKdNode *kd);

  int ProcessUserDefinedCuts(double *bounds);

  void SetCuts(vtkBSPCuts *cuts, int userDefined);

  // Description:
  //   Save enough state so NewGeometry() can work,
  //   and update the BuildTime time stamp.

  void UpdateBuildTime();

  // Description:
  //   Prior to dividing a region at level "level", of size
  //   "numberOfPoints", apply the tests implied by MinCells,
  //   NumberOfRegionsOrMore and NumberOfRegionsOrLess.  Return 1 if it's
  //   OK to divide the region, 0 if you should not.

  int DivideTest(int numberOfPoints, int level);

//BTX
  enum {
    XDIM = 0,  // don't change these values
    YDIM = 1,
    ZDIM = 2
  };
//ETX

  int ValidDirections;

  vtkKdNode *Top;
  vtkKdNode **RegionList;      // indexed by region ID

  vtkTimerLog *TimerLog;

  static void DeleteAllDescendants(vtkKdNode *nd);

  void BuildRegionList();
  virtual int SelectCutDirection(vtkKdNode *kd);
  void SetActualLevel(){this->Level = vtkKdTree::ComputeLevel(this->Top);}

  // Description:
  //    Get back a list of the nodes at a specified level, nodes must
  //    be preallocated to hold 2^^(level) node structures.

  void GetRegionsAtLevel(int level, vtkKdNode **nodes);

  // Description:
  //    Adds to the vtkIntArray the list of region IDs of all leaf
  //    nodes in the given node.

  static void GetLeafNodeIds(vtkKdNode *node, vtkIntArray *ids);


  // Description:
  //   Returns the total number of cells in all the data sets

  int GetNumberOfCells();

  // Description:
  //   Returns the total number of cells in data set 1 through
  //   data set 2.

  int GetDataSetsNumberOfCells(int set1, int set2);

  // Description:
  //    Get or compute the center of one cell.  If the DataSet is
  //    NULL, the first DataSet is used.  This is the point used in
  //    determining to which spatial region the cell is assigned.

  void ComputeCellCenter(vtkDataSet *set, int cellId, float *center);
  void ComputeCellCenter(vtkDataSet *set, int cellId, double *center);

  // Description:
  //    Compute and return a pointer to a list of all cell centers,
  //    in order by data set by cell Id.  If a DataSet is specified
  //    cell centers for cells of that data only are returned.  If
  //    no DataSet is specified, the cell centers of cells in all
  //    DataSets are returned.  The caller should free the list of
  //    cell centers when done.

  float *ComputeCellCenters();
  float *ComputeCellCenters(int set);
  float *ComputeCellCenters(vtkDataSet *set);

  vtkDataSetCollection *DataSets;

  virtual void ReportReferences(vtkGarbageCollector*);

  // Description:
  // Modelled on vtkAlgorithm::UpdateProgress(). 
  // Update the progress when building the locator.
  // Fires vtkCommand::ProgressEvent.
  void UpdateProgress(double amount);

  // Description:
  // Set/Get the execution progress of a process object.
  vtkSetClampMacro(Progress,double,0.0,1.0);
  vtkGetMacro(Progress,double);

protected:
  // So that each suboperation can report progress
  // in [0,1], yet we will be able to report a global 
  // progress. Sub-operations must use UpdateSubOperationProgress()
  // for this to work.
  double ProgressScale;
  double ProgressOffset;
  
  // Update progress for a sub-operation. \c amount goes from 0.0 to 1.0.
  // Actual progress is given by 
  // (this->ProgressOffset + this->ProgressScale* amount).
  void UpdateSubOperationProgress(double amount);

  static void _SetNewBounds(vtkKdNode *kd, double *b, int *fixDim);
  static void CopyChildNodes(vtkKdNode *to, vtkKdNode *from);
  static void CopyKdNode(vtkKdNode *to, vtkKdNode *from);
  static void SetDataBoundsToSpatialBounds(vtkKdNode *kd);
  static void ZeroNumberOfPoints(vtkKdNode *kd);

//BTX
  // Recursive helper for public FindPointsWithinRadius
  void FindPointsWithinRadius(vtkKdNode* node, double R2, 
                              const double x[3], vtkIdList* ids);  

  // Recursive helper for public FindPointsWithinRadius
  void AddAllPointsInRegion(vtkKdNode* node, vtkIdList* ids);

  // Recursive helper for public FindPointsInArea
  void FindPointsInArea(vtkKdNode* node, double* area, vtkIdTypeArray* ids);

  // Recursive helper for public FindPointsInArea
  void AddAllPointsInRegion(vtkKdNode* node, vtkIdTypeArray* ids);

  int DivideRegion(vtkKdNode *kd, float *c1, int *ids, int nlevels);

  void DoMedianFind(vtkKdNode *kd, float *c1, int *ids, int d1, int d2, int d3);

  void SelfRegister(vtkKdNode *kd);

  struct _cellList{
    vtkDataSet *dataSet;        // cell lists for which data set
    int *regionIds;            // NULL if listing all regions
    int nRegions;
    vtkIdList **cells;
    vtkIdList **boundaryCells;
    vtkIdList *emptyList;
  };
//ETX

  void InitializeCellLists();
  vtkIdList *GetList(int regionId, vtkIdList **which);

  void ComputeCellCenter(vtkCell* cell, double *center, double *weights);

  void GenerateRepresentationDataBounds(int level, vtkPolyData *pd);
  void _generateRepresentationDataBounds(vtkKdNode *kd, vtkPoints *pts,
                                         vtkCellArray *polys, int level);

  void GenerateRepresentationWholeSpace(int level, vtkPolyData *pd);
  void _generateRepresentationWholeSpace(vtkKdNode *kd, vtkPoints *pts,
                                         vtkCellArray *polys, int level);

  void AddPolys(vtkKdNode *kd, vtkPoints *pts, vtkCellArray *polys);

  void _printTree(int verbose);

  int SearchNeighborsForDuplicate(int regionId, float *point,
                                  int **pointsSoFar, int *len, 
                                  float tolerance, float tolerance2);

  int SearchRegionForDuplicate(float *point, int *pointsSoFar, 
                               int len, float tolerance2);

  int _FindClosestPointInRegion(int regionId, 
                          double x, double y, double z, double &dist2);

  int FindClosestPointInSphere(double x, double y, double z, double radius,
                               int skipRegion, double &dist2);

  int _ViewOrderRegionsInDirection(vtkIntArray *IdsOfInterest,
                                   const double dop[3],
                                   vtkIntArray *orderedList);

  static int __ViewOrderRegionsInDirection(vtkKdNode *node, vtkIntArray *list, 
                                           vtkIntArray *IdsOfInterest,
                                           const double dir[3], int nextId);

  int _ViewOrderRegionsFromPosition(vtkIntArray *IdsOfInterest,
                                    const double pos[3],
                                    vtkIntArray *orderedList);

  static int __ViewOrderRegionsFromPosition(vtkKdNode *node, vtkIntArray *list, 
                                            vtkIntArray *IdsOfInterest,
                                            const double pos[3], int nextId);

  static int __ConvexSubRegions(int *ids, int len, vtkKdNode *tree, vtkKdNode **nodes);
  static int FoundId(vtkIntArray *idArray, int id);

  void NewParitioningRequest(int req);
  void SetInputDataInfo(int i, 
       int dims[3], double origin[3], double spacing[3]);
  int CheckInputDataInfo(int i, 
       int dims[3], double origin[3], double spacing[3]);
  void ClearLastBuildCache();

//BTX
  static void __printTree(vtkKdNode *kd, int depth, int verbose);
//ETX

  static int MidValue(int dim, float *c1, int nvals, double &coord);

  static int Select(int dim, float *c1, int *ids, int nvals, double &coord);
  static float FindMaxLeftHalf(int dim, float *c1, int K);
  static void _Select(int dim, float *X, int *ids, int L, int R, int K);

//BTX
  static int ComputeLevel(vtkKdNode *kd);
  static int SelfOrder(int id, vtkKdNode *kd);
  static int findRegion(vtkKdNode *node, float x, float y, float z);
  static int findRegion(vtkKdNode *node, double x, double y, double z);
//ETX

  static vtkKdNode **_GetRegionsAtLevel(int level, vtkKdNode **nodes, 
                                        vtkKdNode *kd);

  static void AddNewRegions(vtkKdNode *kd, float *c1, 
                            int midpt, int dim, double coord);

  void NewPartitioningRequest(int req);

  int NumberOfRegionsOrLess;
  int NumberOfRegionsOrMore;

  int IncludeRegionBoundaryCells;
  double CellBoundsCache[6];       // to optimize IntersectsCell()

  int GenerateRepresentationUsingDataBounds;

//BTX
  struct _cellList CellList;
//ETX

  // Region Ids, by data set by cell id - this list is large (one
  // int per cell) but accelerates creation of cell lists

  int *CellRegionList;

  int MinCells;
  int NumberOfRegions;              // number of leaf nodes

  int Timing;
  double FudgeFactor;   // a very small distance, relative to the dataset's size

  // These instance variables are used by the special locator created
  // to find duplicate points. (BuildLocatorFromPoints)

  int NumberOfLocatorPoints;
  float *LocatorPoints;
  int *LocatorIds;
  int *LocatorRegionLocation;

  float MaxWidth;

  // These Last* values are here to save state so we can
  // determine later if k-d tree must be rebuilt.

  int LastNumDataSets;
  int LastDataCacheSize;
  vtkDataSet **LastInputDataSets;
  unsigned long *LastDataSetObserverTags;
  int *LastDataSetType;
  double *LastInputDataInfo;
  double *LastBounds;
  vtkIdType *LastNumPoints;
  vtkIdType *LastNumCells;

  vtkBSPCuts *Cuts;
  double Progress;

  vtkKdTree(const vtkKdTree&); // Not implemented
  void operator=(const vtkKdTree&); // Not implemented
};
#endif
