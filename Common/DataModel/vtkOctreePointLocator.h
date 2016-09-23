/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOctreePointLocator.h

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
 * @class   vtkOctreePointLocator
 * @brief   an octree spatial decomposition of a set of points
 *
 *
 * Given a vtkDataSet, create an octree that is locally refined
 * such that all leaf octants contain less than a certain
 * amount of points.  Note that there is no size constraint that
 * a leaf octant in relation to any of its neighbors.
 *
 * This class can also generate a PolyData representation of
 * the boundaries of the spatial regions in the decomposition.
 *
 * @sa
 * vtkLocator vtkPointLocator vtkOctreePointLocatorNode
*/

#ifndef vtkOctreePointLocator_h
#define vtkOctreePointLocator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkAbstractPointLocator.h"

class vtkCellArray;
class vtkIdTypeArray;
class vtkOctreePointLocatorNode;
class vtkPoints;
class vtkPolyData;

class VTKCOMMONDATAMODEL_EXPORT vtkOctreePointLocator : public vtkAbstractPointLocator
{
public:
  vtkTypeMacro(vtkOctreePointLocator, vtkAbstractPointLocator);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  static vtkOctreePointLocator *New();

  //@{
  /**
   * Maximum number of points per spatial region.  Default is 100.
   */
  vtkSetMacro(MaximumPointsPerRegion, int);
  vtkGetMacro(MaximumPointsPerRegion, int);
  //@}

  //@{
  /**
   * Get/Set macro for CreateCubicOctants.
   */
  vtkSetMacro(CreateCubicOctants, int);
  vtkGetMacro(CreateCubicOctants, int);
  //@}

  //@{
  /**
   * Some algorithms on octrees require a value that is a very
   * small distance relative to the diameter of the entire space
   * divided by the octree.  This factor is the maximum axis-aligned
   * width of the space multiplied by 10e-6.
   */
  vtkGetMacro(FudgeFactor, double);
  vtkSetMacro(FudgeFactor, double);
  //@}

  //@{
  /**
   * Get the spatial bounds of the entire octree space. Sets
   * bounds array to xmin, xmax, ymin, ymax, zmin, zmax.
   */
  double *GetBounds() VTK_OVERRIDE;
  void GetBounds(double *bounds) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * The number of leaf nodes of the tree, the spatial regions
   */
  vtkGetMacro(NumberOfLeafNodes, int);
  //@}

  /**
   * Get the spatial bounds of octree region
   */
  void GetRegionBounds(int regionID, double bounds[6]);

  /**
   * Get the bounds of the data within the leaf node
   */
  void GetRegionDataBounds(int leafNodeID, double bounds[6]);

  /**
   * Get the id of the leaf region containing the specified location.
   */
  int GetRegionContainingPoint(double x, double y, double z);

  /**
   * Create the octree decomposition of the cells of the data set
   * or data sets.  Cells are assigned to octree spatial regions
   * based on the location of their centroids.
   */
  void BuildLocator() VTK_OVERRIDE;

  //@{
  /**
   * Return the Id of the point that is closest to the given point.
   * Set the square of the distance between the two points.
   */
  vtkIdType FindClosestPoint(const double x[3]) VTK_OVERRIDE;
  vtkIdType FindClosestPoint(double x, double y, double z, double &dist2);
  //@}

  /**
   * Given a position x and a radius r, return the id of the point
   * closest to the point in that radius.
   * dist2 returns the squared distance to the point.
   */
  vtkIdType FindClosestPointWithinRadius(
    double radius, const double x[3], double& dist2) VTK_OVERRIDE;

  //@{
  /**
   * Find the Id of the point in the given leaf region which is
   * closest to the given point.  Return the ID of the point,
   * and set the square of the distance of between the points.
   */
  vtkIdType FindClosestPointInRegion(int regionId, double *x, double &dist2);
  vtkIdType FindClosestPointInRegion(int regionId, double x, double y,
                                     double z, double &dist2);
  //@}

  /**
   * Find all points within a specified radius of position x.
   * The result is not sorted in any specific manner.
   */
  void FindPointsWithinRadius(
    double radius, const double x[3], vtkIdList *result) VTK_OVERRIDE;

  /**
   * Find the closest N points to a position. This returns the closest
   * N points to a position. A faster method could be created that returned
   * N close points to a position, but not necessarily the exact N closest.
   * The returned points are sorted from closest to farthest.
   * These methods are thread safe if BuildLocator() is directly or
   * indirectly called from a single thread first.
   */
  void FindClosestNPoints(int N, const double x[3],
                          vtkIdList *result) VTK_OVERRIDE;

  /**
   * Get a list of the original IDs of all points in a leaf node.
   */
  vtkIdTypeArray *GetPointsInRegion(int leafNodeId);

  /**
   * Delete the octree data structure.
   */
  void FreeSearchStructure() VTK_OVERRIDE;

  /**
   * Create a polydata representation of the boundaries of
   * the octree regions.
   */
  void GenerateRepresentation(int level, vtkPolyData *pd) VTK_OVERRIDE;

  /**
   * Fill ids with points found in area.  The area is a 6-tuple containing
   * (xmin, xmax, ymin, ymax, zmin, zmax).
   * This method will clear the array by default.  To append ids to an array,
   * set clearArray to false.
   */
  void FindPointsInArea(double* area, vtkIdTypeArray* ids, bool clearArray = true);

protected:

  vtkOctreePointLocator();
  ~vtkOctreePointLocator() VTK_OVERRIDE;

  vtkOctreePointLocatorNode *Top;
  vtkOctreePointLocatorNode **LeafNodeList;      // indexed by region/node ID

  void BuildLeafNodeList(vtkOctreePointLocatorNode* node, int & index);

  //@{
  /**
   * Given a point and a node return the leaf node id that contains the
   * point.  The function returns -1 if no nodes contain the point.
   */
  int FindRegion(vtkOctreePointLocatorNode* node, float x, float y, float z);
  int FindRegion(vtkOctreePointLocatorNode* node, double x, double y, double z);
  //@}

  static void SetDataBoundsToSpatialBounds(vtkOctreePointLocatorNode *node);

  static void DeleteAllDescendants(vtkOctreePointLocatorNode* octant);

  /**
   * Recursive helper for public FindPointsWithinRadius.  radiusSquared
   * is the square of the radius and is used in order to avoid the
   * expensive square root calculation.
   */
  void FindPointsWithinRadius(vtkOctreePointLocatorNode* node, double radiusSquared,
                              const double x[3], vtkIdList* ids);

  // Recursive helper for public FindPointsWithinRadius
  void AddAllPointsInRegion(vtkOctreePointLocatorNode* node, vtkIdList* ids);

  // Recursive helper for public FindPointsInArea
  void FindPointsInArea(vtkOctreePointLocatorNode* node, double* area, vtkIdTypeArray* ids);

  // Recursive helper for public FindPointsInArea
  void AddAllPointsInRegion(vtkOctreePointLocatorNode* node, vtkIdTypeArray* ids);

  void DivideRegion(vtkOctreePointLocatorNode *node, int* ordering, int level);

  int DivideTest(int size, int level);

  void AddPolys(vtkOctreePointLocatorNode *node, vtkPoints *pts, vtkCellArray *polys);

  /**
   * Given a leaf node id and point, return the local id and the squared distance
   * between the closest point and the given point.
   */
  int _FindClosestPointInRegion(int leafNodeId, double x, double y,
                                double z, double &dist2);

  /**
   * Given a location and a radiues, find the closest point within
   * this radius.  The function does not examine the region with Id
   * equal to skipRegion (do not set skipRegion to -1 as all non-leaf
   * octants have -1 as their Id).  The Id is returned along with
   * the distance squared for success and -1 is returned for failure.
   */
  int FindClosestPointInSphere(double x, double y, double z, double radius,
                               int skipRegion, double &dist2);

  //@{
  /**
   * The maximum number of points in a region/octant before it is subdivided.
   */
  int MaximumPointsPerRegion;
  int NumberOfLeafNodes;
  //@}

  double FudgeFactor;   // a very small distance, relative to the dataset's size
  int NumberOfLocatorPoints;
  float *LocatorPoints;
  int *LocatorIds;

  float MaxWidth;

  /**
   * If CreateCubicOctants is non-zero, the bounding box of the points will
   * be expanded such that all octants that are created will be cube-shaped
   * (e.g. have equal lengths on each side).  This may make the tree deeper
   * but also results in better shaped octants for doing searches. The
   * default is to have this set on.
   */
  int CreateCubicOctants;

  vtkOctreePointLocator(const vtkOctreePointLocator&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOctreePointLocator&) VTK_DELETE_FUNCTION;
};
#endif
