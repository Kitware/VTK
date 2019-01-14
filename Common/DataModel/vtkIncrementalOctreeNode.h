/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIncrementalOctreeNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkIncrementalOctreeNode
 * @brief   Octree node constituting incremental
 *  octree (in support of both point location and point insertion)
 *
 *
 *  Octree nodes serve as spatial sub-division primitives to build the search
 *  structure of an incremental octree in a recursive top-down manner. The
 *  hierarchy takes the form of a tree-like representation by which a parent
 *  node contains eight mutually non-overlapping child nodes. Each child is
 *  assigned with an axis-aligned rectangular volume (Spatial Bounding Box)
 *  and the eight children together cover exactly the same region as governed
 *  by their parent. The eight child nodes / octants are ordered as
 *
 *  { (xBBoxMin, xBBoxMid] & (yBBoxMin, yBBoxMid] & (zBBoxMin, zBBoxMid] },
 *  { (xBBoxMid, xBBoxMax] & (yBBoxMin, yBBoxMid] & (zBBoxMin, zBBoxMid] },
 *  { (xBBoxMin, xBBoxMid] & (yBBoxMid, yBBoxMax] & (zBBoxMin, zBBoxMid] },
 *  { (xBBoxMid, xBBoxMax] & (yBBoxMid, yBBoxMax] & (zBBoxMin, zBBoxMid] },
 *  { (xBBoxMin, xBBoxMid] & (yBBoxMin, yBBoxMid] & (zBBoxMid, zBBoxMax] },
 *  { (xBBoxMid, xBBoxMax] & (yBBoxMin, yBBoxMid] & (zBBoxMid, zBBoxMax] },
 *  { (xBBoxMin, xBBoxMid] & (yBBoxMid, yBBoxMax] & (zBBoxMid, zBBoxMax] },
 *  { (xBBoxMid, xBBoxMax] & (yBBoxMid, yBBoxMax] & (zBBoxMid, zBBoxMax] },
 *
 *  where { xrange & yRange & zRange } defines the region of each 3D octant.
 *  In addition, the points falling within and registered, by means of point
 *  indices, in the parent node are distributed to the child nodes for delegated
 *  maintenance. In fact, only leaf nodes, i.e., those without any descendants,
 *  actually store point indices while each node, regardless of a leaf or non-
 *  leaf node, keeps a dynamically updated Data Bounding Box of the inhabitant
 *  points, if any. Given a maximum number of points per leaf node, an octree
 *  is initialized with an empty leaf node that is then recursively sub-divided,
 *  but only on demand as points are incrementally inserted, to construct a
 *  populated tree.
 *
 *  Please note that this octree node class is able to handle a large number
 *  of EXACTLY duplicate points that is greater than the specified maximum
 *  number of points per leaf node. In other words, as an exception, a leaf
 *  node may maintain an arbitrary number of exactly duplicate points to deal
 *  with possible extreme cases.
 *
 * @sa
 *  vtkIncrementalOctreePointLocator
*/

#ifndef vtkIncrementalOctreeNode_h
#define vtkIncrementalOctreeNode_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

class vtkPoints;
class vtkIdList;

class VTKCOMMONDATAMODEL_EXPORT vtkIncrementalOctreeNode : public vtkObject
{
public:
  vtkTypeMacro( vtkIncrementalOctreeNode, vtkObject );
  void PrintSelf( ostream & os, vtkIndent indent ) override;

  static vtkIncrementalOctreeNode * New();

  //@{
  /**
   * Get the number of points inside or under this node.
   */
  vtkGetMacro( NumberOfPoints, int );
  //@}

  //@{
  /**
   * Get the list of point indices, nullptr for a non-leaf node.
   */
  vtkGetObjectMacro( PointIdSet, vtkIdList );
  //@}

  /**
   * Delete the eight child nodes.
   */
  void DeleteChildNodes();

  /**
   * Set the spatial bounding box of the node. This function sets a default
   * data bounding box.
   */
  void SetBounds( double x1, double x2, double y1,
                  double y2, double z1, double z2 );

  /**
   * Get the spatial bounding box of the node. The values are returned via
   * an array in order of: x_min, x_max, y_min, y_max, z_min, z_max.
   */
  void GetBounds( double bounds[6] ) const;

  //@{
  /**
   * Get access to MinBounds. Do not free this pointer.
   */
  vtkGetVector3Macro( MinBounds, double );
  //@}

  //@{
  /**
   * Get access to MaxBounds. Do not free this pointer.
   */
  vtkGetVector3Macro( MaxBounds, double );
  //@}

  /**
   * Get access to MinDataBounds. Note that MinDataBounds is not valid until
   * point insertion.
   */
  double * GetMinDataBounds()
  { return this->NumberOfPoints ? this->MinDataBounds : this->MinBounds; }

  /**
   * Get access to MaxDataBounds. Note that MaxDataBounds is not valid until
   * point insertion.
   */
  double * GetMaxDataBounds()
  { return this->NumberOfPoints ? this->MaxDataBounds : this->MaxBounds; }

  /**
   * Determine whether or not this node is a leaf.
   */
  int IsLeaf() { return ( this->Children == nullptr ) ? 1 : 0; }

  /**
   * Determine which specific child / octant contains a given point. Note that
   * the point is assumed to be inside this node and no checking is performed
   * on the inside issue.
   */
  int GetChildIndex( const double point[3] );

  /**
   * Get quick access to a child of this node. Note that this node is assumed
   * to be a non-leaf one and no checking is performed on the node type.
   */
  vtkIncrementalOctreeNode * GetChild( int i ) { return this->Children[i]; }

  /**
   * A point is in a node if and only if MinBounds[i] < p[i] <= MaxBounds[i],
   * which allows a node to be divided into eight non-overlapping children.
   */
  vtkTypeBool ContainsPoint( const double pnt[3] );

  /**
   * A point is in a node, in terms of data, if and only if MinDataBounds[i]
   * <= p[i] <= MaxDataBounds[i].
   */
  vtkTypeBool ContainsPointByData( const double pnt[3] );

  /**
   * This function is called after a successful point-insertion check and
   * only applies to a leaf node. Prior to a call to this function, the
   * octree should have been retrieved top-down to find the specific leaf
   * node in which this new point (newPt) will be inserted. The actual index
   * of the new point (to be inserted to points) is stored in pntId. Argument
   * ptMode specifies whether the point is not inserted at all but instead only
   * the point index is provided upon 0, the point is inserted via vtkPoints::
   * InsertPoint() upon 1, or it is inserted via vtkPoints::InsertNextPoint()
   * upon 2. For case 0, pntId needs to be specified. For cases 1 and 2, the
   * actual point index is returned via pntId. Note that this function always
   * returns 1 to indicate the success of point insertion.
   */
  int InsertPoint( vtkPoints * points, const double newPnt[3],
                   int maxPts, vtkIdType * pntId, int ptMode );

  /**
   * Given a point inside this node, get the minimum squared distance to all
   * inner boundaries. An inner boundary is a node's face that is shared by
   * another non-root node.
   */
  double GetDistance2ToInnerBoundary( const double point[3],
                                      vtkIncrementalOctreeNode * rootNode );

  /**
   * Compute the minimum squared distance from a point to this node, with all
   * six boundaries considered. The data bounding box is checked if checkData
   * is non-zero.
   */
  double GetDistance2ToBoundary( const double point[3],
    vtkIncrementalOctreeNode * rootNode, int checkData );

  /**
   * Compute the minimum squared distance from a point to this node, with all
   * six boundaries considered. The data bounding box is checked if checkData
   * is non-zero. The closest on-boundary point is returned via closest.
   */
  double GetDistance2ToBoundary( const double point[3], double closest[3],
    vtkIncrementalOctreeNode * rootNode, int checkData );

  /**
   * Export all the indices of the points (contained in or under this node) by
   * inserting them to an allocated vtkIdList via vtkIdList::InsertNextId().
   */
  void ExportAllPointIdsByInsertion( vtkIdList * idList );

  /**
   * Export all the indices of the points (contained in or under this node) by
   * directly setting them in an allocated vtkIdList object. pntIdx indicates
   * the starting location (in terms of vtkIdList) from which new point indices
   * are added to vtkIdList by vtkIdList::SetId().
   */
  void ExportAllPointIdsByDirectSet( vtkIdType * pntIdx, vtkIdList * idList );

protected:

  vtkIncrementalOctreeNode();
  ~vtkIncrementalOctreeNode() override;

private:

  /**
   * Number of points inside or under this node.
   */
  int  NumberOfPoints;

  /**
   * The minimum coordinate of this node's spatial bounding box.
   */
  double MinBounds[3];

  /**
   * The maximum coordinate of this node's spatial bounding box.
   */
  double MaxBounds[3];

  /**
   * The minimum coordinate of the data bounding box that encompasses the
   * points inserted, by means of the point index, to this node. Note this
   * information is invalid if no any point has been inserted to this node.
   */
  double MinDataBounds[3];

  /**
   * The maximum coordinate of the data bounding box that encompasses the
   * points inserted, by means of the point index, to this node. Note this
   * information is invalid if no any point has been inserted to this node.
   */
  double MaxDataBounds[3];

  /**
   * The list of indices of the points maintained by this LEAF node. It is
   * nullptr if this is a non-leaf node.
   */
  vtkIdList * PointIdSet;

  /**
   * The parent of this node, nullptr for the root node of an octree.
   */
  vtkIncrementalOctreeNode *  Parent;

  /**
   * A pointer to the eight children of this node.
   */
  vtkIncrementalOctreeNode ** Children;

  /**
   * Set the parent of this node, nullptr for the root node of an octree.
   */
  virtual void SetParent( vtkIncrementalOctreeNode * );

  /**
   * Set the list of point indices, nullptr for a non-leaf node.
   */
  virtual void SetPointIdSet( vtkIdList * );

  /**
   * Divide this LEAF node into eight child nodes as the number of points
   * maintained by this leaf node has reached the threshold maxPts while
   * another point newPnt is just going to be inserted to it. The available
   * point-indices pntIds are distributed to the child nodes based on the
   * point coordinates (available through points). Note that this function
   * can incur recursive node-division to determine the specific leaf node
   * for accepting the new point (with pntIdx storing the index in points)
   * because the existing maxPts points may fall within only one of the eight
   * child nodes to make a radically imbalanced layout within the node (to
   * be divided). Argument ptMode specifies whether the point is not inserted
   * at all but instead only the point index is provided upon 0, the point is
   * inserted via vtkPoints::InsertPoint() upon 1, or the point is inserted by
   * vtkPoints::InsertNextPoint() upon 2. The returned value of this function
   * indicates whether pntIds needs to be destroyed (1) or just unregistered
   * from this node as it has been attached to another node (0).
   */
  int CreateChildNodes( vtkPoints * points, vtkIdList * pntIds,
    const double newPnt[3], vtkIdType * pntIdx, int maxPts, int ptMode );

  /**
   * Create a vtkIdList object for storing point indices. Two arguments
   * specifies the initial and growing sizes, respectively, of the object.
   */
  void CreatePointIdSet( int initSize, int growSize );

  /**
   * Delete the list of point indices.
   */
  void DeletePointIdSet();

  /**
   * Given a point inserted to either this node (a leaf node) or a descendant
   * leaf (of this node --- when this node is a non-leaf node), update the
   * counter and the data bounding box for this node only.
   */
  void UpdateCounterAndDataBounds( const double point[3] );

  /**
   * Given a point inserted to either this node (a leaf node) or a descendant
   * leaf (of this node --- when this node is a non-leaf node), update the
   * counter and the data bounding box for this node only. The data bounding box
   * is considered only if updateData is non-zero. The returned value indicates
   * whether (1) or not (0) the data bounding box is actually updated. Note that
   * argument nHits must be 1 unless this node is updated with a number (nHits)
   * of exactly duplicate points as a whole via a single call to this function.
   */
  int UpdateCounterAndDataBounds
    ( const double point[3], int nHits, int updateData );

  /**
   * Given a point inserted to either this node (a leaf node) or a descendant
   * leaf (of this node --- when this node is a non-leaf node), update the
   * counter and the data bounding box recursively bottom-up until a specified
   * node. The data bounding box is considered only if updateData is non-zero.
   * The returned value indicates whether (1) or not (0) the data bounding box
   * is actually updated. Note that argument nHits must be 1 unless this node
   * is updated with a number (nHits) of exactly duplicate points as a whole
   * via a single call to this function.
   */
  int UpdateCounterAndDataBoundsRecursively( const double point[3], int nHits,
    int updateData, vtkIncrementalOctreeNode * endNode );

  /**
   * Given a point, determine whether (1) or not (0) it is an exact duplicate
   * of all the points, if any, maintained in this node. In other words, to
   * check if this given point and all existing points, if any, of this node
   * are exactly duplicate with one another.
   */
  int  ContainsDuplicatePointsOnly( const double pnt[3] );

  /**
   * Given a number (>= threshold) of all exactly duplicate points (accessible
   * via points and pntIds, but with exactly the same 3D coordinate) maintained
   * in this leaf node and a point (absolutely not a duplicate any more, with
   * pntIdx storing the index in points)) to be inserted to this node, separate
   * all the duplicate points from this new point by means of usually recursive
   * node sub-division such that the former points are inserted to a descendant
   * leaf while the new point is inserted to a sibling of this descendant leaf.
   * Argument ptMode specifies whether the point is not inserted at all but only
   * the point index is provided upon 0, the point is inserted via vtkPoints::
   * InsertPoint() upon 1, or this point is instead inserted through vtkPoints::
   * InsertNextPoint() upon 2.
   */
  void SeperateExactlyDuplicatePointsFromNewInsertion( vtkPoints * points,
    vtkIdList * pntIds, const double newPnt[3],
    vtkIdType * pntIdx, int maxPts, int ptMode );

  /**
   * Given a point, obtain the minimum squared distance to the closest point
   * on a boundary of this node. As two options, the outer boundaries may be
   * excluded (by comparing them against those of the root node) from
   * consideration and the data bounding box may be checked in place of the
   * spatial bounding box.
   */
  double GetDistance2ToBoundary( const double point[3], double closest[3],
    int innerOnly, vtkIncrementalOctreeNode* rootNode, int checkData = 0 );

  vtkIncrementalOctreeNode( const vtkIncrementalOctreeNode & ) = delete;
  void operator = ( const vtkIncrementalOctreeNode & ) = delete;

};

// In-lined for performance
inline int vtkIncrementalOctreeNode::GetChildIndex( const double point[3] )
{
  // Children[0]->MaxBounds[] is exactly the center point of this node.
  return int( point[0] > this->Children[0]->MaxBounds[0] ) +
    (  ( int( point[1] > this->Children[0]->MaxBounds[1] ) ) << 1  ) +
    (  ( int( point[2] > this->Children[0]->MaxBounds[2] ) ) << 2  );
}

// In-lined for performance
inline vtkTypeBool vtkIncrementalOctreeNode::ContainsPoint( const double pnt[3] )
{
  return (
            ( this->MinBounds[0] < pnt[0] && pnt[0] <= this->MaxBounds[0] &&
              this->MinBounds[1] < pnt[1] && pnt[1] <= this->MaxBounds[1] &&
              this->MinBounds[2] < pnt[2] && pnt[2] <= this->MaxBounds[2]
            ) ? 1 : 0
         );
}

// In-lined for performance
inline vtkTypeBool vtkIncrementalOctreeNode::ContainsPointByData( const double pnt[3] )
{
  return
  (
     ( this->MinDataBounds[0] <= pnt[0] && pnt[0] <= this->MaxDataBounds[0] &&
       this->MinDataBounds[1] <= pnt[1] && pnt[1] <= this->MaxDataBounds[1] &&
       this->MinDataBounds[2] <= pnt[2] && pnt[2] <= this->MaxDataBounds[2]
     ) ? 1 : 0
  );
}

// In-lined for performance
inline int vtkIncrementalOctreeNode::ContainsDuplicatePointsOnly
  ( const double pnt[3] )
{
  return
  (
     ( this->MinDataBounds[0] == pnt[0] && pnt[0] == this->MaxDataBounds[0] &&
       this->MinDataBounds[1] == pnt[1] && pnt[1] == this->MaxDataBounds[1] &&
       this->MinDataBounds[2] == pnt[2] && pnt[2] == this->MaxDataBounds[2]
     ) ? 1 : 0
  );
}

// In-lined for performance
inline void vtkIncrementalOctreeNode::UpdateCounterAndDataBounds
  ( const double point[3] )
{
  this->NumberOfPoints ++;

  this->MinDataBounds[0] = ( point[0] < this->MinDataBounds[0] )
                           ? point[0] : this->MinDataBounds[0];
  this->MinDataBounds[1] = ( point[1] < this->MinDataBounds[1] )
                           ? point[1] : this->MinDataBounds[1];
  this->MinDataBounds[2] = ( point[2] < this->MinDataBounds[2] )
                           ? point[2] : this->MinDataBounds[2];
  this->MaxDataBounds[0] = ( point[0] > this->MaxDataBounds[0] )
                           ? point[0] : this->MaxDataBounds[0];
  this->MaxDataBounds[1] = ( point[1] > this->MaxDataBounds[1] )
                           ? point[1] : this->MaxDataBounds[1];
  this->MaxDataBounds[2] = ( point[2] > this->MaxDataBounds[2] )
                           ? point[2] : this->MaxDataBounds[2];
}

// In-lined for performance
inline int vtkIncrementalOctreeNode::UpdateCounterAndDataBoundsRecursively
  ( const double point[3], int nHits, int updateData,
    vtkIncrementalOctreeNode * endNode )
{
  int  updated = this->UpdateCounterAndDataBounds
                       ( point, nHits, updateData );

  return (    ( this->Parent == endNode )
           ?  updated
           :  this->Parent->UpdateCounterAndDataBoundsRecursively
                            ( point, nHits, updated, endNode )
         );
}
#endif
