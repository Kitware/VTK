/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIncrementalOctreePointLocator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
 
=========================================================================*/
// .NAME vtkIncrementalOctreePointLocator - Incremental octree in support
//  of both point location and point insertion.
// 
// .SECTION Description
//  As opposed to the uniform bin-based search structure (adopted in class
//  vtkPointLocator) with a fixed spatial resolution, an octree mechanism 
//  employs a hierarchy of tree-like sub-division of the 3D data domain. Thus
//  it enables data-aware multi-resolution and accordingly accelerated point
//  location as well as insertion, particularly when handling a radically 
//  imbalanced layout of points as not uncommon in datasets defined on 
//  adaptive meshes. Compared to a static point locator supporting pure
//  location functionalities through some search structure established from
//  a fixed set of points, an incremental point locator allows for, in addition,
//  point insertion capabilities, with the search structure maintaining a 
//  dynamically increasing number of points.
//  Class vtkIncrementalOctreePointLocator is an octree-based accelerated
//  implementation of the functionalities of the uniform bin-based incremental
//  point locator vtkPointLocator. For point location, an octree is built by
//  accessing a vtkDataSet, specifically a vtkPointSet. For point insertion,
//  an empty octree is inited and then incrementally populated as points are
//  inserted. Three increasingly complex point insertion modes, i.e., direct
//  check-free insertion, zero tolerance insertion, and non-zero tolerance 
//  insertion, are supported. In fact, the octree used in the point location
//  mode is actually constructed via direct check-free point insertion. This
//  class also provides a polygonal representation of the octree boundary.
//  
// .SECTION See Also
//  vtkAbstractPointLocator, vtkIncrementalPointLocator, vtkPointLocator,
//  vtkMergePoints

#ifndef __vtkIncrementalOctreePointLocator_h
#define __vtkIncrementalOctreePointLocator_h

#include "vtkIncrementalPointLocator.h"

class vtkPoints;
class vtkIdList;
class vtkPolyData;
class vtkCellArray;
class vtkIncrementalOctreeNode;

class VTK_FILTERING_EXPORT vtkIncrementalOctreePointLocator : public vtkIncrementalPointLocator
{
public:
 
  vtkTypeMacro( vtkIncrementalOctreePointLocator, vtkIncrementalPointLocator );
  void PrintSelf( ostream & os, vtkIndent indent );

  static vtkIncrementalOctreePointLocator * New();
  
  // Description:
  // Set/Get the maximum number of points that a leaf node may maintain.
  // Note that the actual number of points maintained by a leaf node might
  // exceed this threshold if there is a large number (equal to or greater
  // than the threshold) of exactly duplicate points (with zero distance)
  // to be inserted (e.g., to construct an octree for subsequent point 
  // location) in extreme cases. Respecting this threshold in such scenarios
  // would cause endless node sub-division. Thus this threshold is broken, but
  // only in case of such situations.
  vtkSetClampMacro( MaxPointsPerLeaf, int, 16, 256 );
  vtkGetMacro( MaxPointsPerLeaf, int );
  
  // Description:
  // Set/Get whether the search octree is built as a cubic shape or not.
  vtkSetMacro( BuildCubicOctree, int );
  vtkGetMacro( BuildCubicOctree, int );
  vtkBooleanMacro( BuildCubicOctree, int );
  
  // Description:
  // Get access to the vtkPoints object in which point coordinates are stored
  // for either point location or point insertion.
  vtkGetObjectMacro( LocatorPoints, vtkPoints );
  
  // Description:
  // Delete the octree search structure.
  virtual void Initialize() { this->FreeSearchStructure(); }
  
  // Description:
  // Delete the octree search structure. 
  virtual void FreeSearchStructure();
  
  // Description:
  // Get the spatial bounding box of the octree.
  virtual void GetBounds( double * bounds );
  
  // Description:
  // Get the spatial bounding box of the octree.
  virtual double * GetBounds()
    { this->GetBounds( this->Bounds );  return this->Bounds; }
  
  // Description:
  // Get the number of points maintained by the octree.
  int GetNumberOfPoints();
  
  // Description:
  // Given a point x assumed to be covered by the octree, return the index of
  // the closest in-octree point regardless of the associated minimum squared
  // distance relative to the squared insertion-tolerance distance. This method
  // is used when performing incremental point insertion. Note -1 indicates that
  // no point is found. InitPointInsertion() should have been called in advance.
  virtual vtkIdType FindClosestInsertedPoint( const double x[3] );
  
  // Description:
  // Create a polygonal representation of the octree boundary (from the root
  // node to a specified level).
  virtual void GenerateRepresentation( int nodeLevel, vtkPolyData * polysData );
  
  // -------------------------------------------------------------------------
  // ---------------------------- Point  Location ----------------------------
  // -------------------------------------------------------------------------
  
  // Description:
  // Load points from a dataset to construct an octree for point location. 
  // This function resorts to InitPointInsertion() to fulfill some of the work.
  virtual void BuildLocator();
  
  // Description:
  // Given a point x, return the id of the closest point. BuildLocator() should
  // have been called prior to this function. This method is thread safe if 
  // BuildLocator() is directly or indirectly called from a single thread first.
  virtual vtkIdType FindClosestPoint( const double x[3] );
  
  // Description:
  // Given a point (x, y, z), return the id of the closest point. Note that
  // BuildLocator() should have been called prior to this function. This method
  // is thread safe if BuildLocator() is directly or indirectly called from a 
  // single thread first.
  virtual vtkIdType FindClosestPoint( double x, double y, double z );
  
  // Description:
  // Given a point x, return the id of the closest point and the associated
  // minimum squared distance (via miniDist2). Note BuildLocator() should have
  // been called prior to this function. This method is thread safe if 
  // BuildLocator() is directly or indirectly called from a single thread first.
  virtual vtkIdType FindClosestPoint( const double x[3], double * miniDist2 );
  
  // Description:
  // Given a point (x, y, z), return the id of the closest point and the
  // associated minimum squared distance (via miniDist2). BuildLocator() should
  // have been called prior to this function. This method is thread safe if 
  // BuildLocator() is directly or indirectly called from a single thread first.
  virtual vtkIdType FindClosestPoint( double x, double y, double z, double * miniDist2 );
  
  // Description:
  // Given a point x and a radius, return the id of the closest point within
  // the radius and the associated minimum squared distance (via dist2, this 
  // returned distance is valid only if the point id is not -1). Note that
  // BuildLocator() should have been called prior to this function. This method
  // is thread safe if BuildLocator() is directly or indirectly called from a 
  // single thread first. 
  virtual vtkIdType FindClosestPointWithinRadius
    ( double radius, const double x[3], double & dist2 );
    
  // Description:
  // Given a point x and a squared radius radius2, return the id of the closest
  // point within the radius and the associated minimum squared distance (via
  // dist2, note this returned distance is valid only if the point id is not
  // -1). BuildLocator() should have been called prior to this function.This 
  // method is thread safe if BuildLocator() is directly or indirectly called 
  // from a single thread first. 
  vtkIdType FindClosestPointWithinSquaredRadius
    ( double radius2, const double x[3], double & dist2 );
    
  // Description:
  // Find all points within a radius R relative to a given point x. The returned
  // point ids (stored in result) are not sorted in any way. BuildLocator() should
  // have been called prior to this function. This method is thread safe if 
  // BuildLocator() is directly or indirectly called from a single thread first.
  virtual void FindPointsWithinRadius
    ( double R, const double x[3], vtkIdList * result );
  
  // Description:
  // Find all points within a squared radius R2 relative to a given point x. The
  // returned point ids (stored in result) are not sorted in any way. BuildLocator()
  // should have been called prior to this function. This method is thread safe if 
  // BuildLocator() is directly or indirectly called from a single thread first.
  void FindPointsWithinSquaredRadius
    ( double R2, const double x[3], vtkIdList * result );
                                      
  // Description:
  // Find the closest N points to a given point. The returned point ids (via 
  // result) are sorted from closest to farthest. BuildLocator() should have 
  // been called prior to this function. This method is thread safe if 
  // BuildLocator() is directly or indirectly called from a single thread first.
  virtual void FindClosestNPoints
    ( int N, const double x[3], vtkIdList * result );
                       
  // -------------------------------------------------------------------------
  // ---------------------------- Point Insertion ----------------------------
  // -------------------------------------------------------------------------
  
  // Description:
  // Initialize the point insertion process. points is an object, storing 3D
  // point coordinates, to which incremental point insertion put coordinates.
  // It is created and provided by an external VTK class. Argument bounds
  // represents the spatial bounding box, into which the points fall. In fact, 
  // an adjusted version of the bounding box is used to build the octree to 
  // make sure no any point (to be inserted) falls outside the octree. This
  // function is not thread safe.
  virtual int InitPointInsertion( vtkPoints * points, const double bounds[6] );

  // Description:
  // Initialize the point insertion process. points is an object, storing 3D
  // point coordinates, to which incremental point insertion put coordinates.
  // It is created and provided by an external VTK class. Argument bounds
  // represents the spatial bounding box, into which the points fall. In fact, 
  // an adjusted version of the bounding box is used to build the octree to 
  // make sure no any point (to be inserted) falls outside the octree. Argument
  // estSize specifies the initial estimated size of the vtkPoints object. This
  // function is not thread safe.
  virtual int InitPointInsertion( vtkPoints * points, const double bounds[6], 
                                  vtkIdType estSize );                            
  
  // Description:
  // Determine whether or not a given point has been inserted into the octree.
  // Return the id of the already inserted point if true, otherwise return -1.
  // InitPointInsertion() should have been called in advance.
  virtual vtkIdType IsInsertedPoint( const double x[3] );
  
  // Description:
  // Determine whether or not a given point has been inserted into the octree.
  // Return the id of the already inserted point if true, otherwise return -1.
  // InitPointInsertion() should have been called in advance.
  virtual vtkIdType IsInsertedPoint( double x, double  y, double z );
  
  // Description:
  // Insert a point to the octree unless there has been a duplciate point.
  // Whether the point is actually inserted (return 1) or not (return 0 upon a
  // rejection by an existing duplicate), the index of the point (either new
  // or the duplicate) is returned via pntId. Note that InitPointInsertion() 
  // should have been called prior to this function. vtkPoints::InsertNextPoint()
  // is invoked. This method is not thread safe.
  virtual int InsertUniquePoint( const double point[3], vtkIdType & pntId );
  
  // Description:
  // Insert a given point into the octree with a specified point index ptId.
  // InitPointInsertion() should have been called prior to this function. In 
  // addition, IsInsertedPoint() should have been called in advance to ensure
  // that the given point has not been inserted unless point duplication is 
  // allowed (Note that in this case, this function involves a repeated leaf 
  // container location). vtkPoints::InsertPoint() is invoked.
  virtual void InsertPoint( vtkIdType ptId, const double x[3] );
  
  // Description:
  // Insert a given point into the octree and return the point index. Note that
  // InitPointInsertion() should have been called prior to this function. In 
  // addition, IsInsertedPoint() should have been called in advance to ensure 
  // that the given point has not been inserted unless point duplication is 
  // allowed (in this case, this function invovles a repeated leaf container 
  // location). vtkPoints::InsertNextPoint() is invoked.
  virtual vtkIdType InsertNextPoint( const double x[3] );
  
  // Description:
  // "Insert" a point to the octree without any checking. Argument insert means
  // whether vtkPoints::InsertNextPoint() upon 1 is called or the point itself
  // is not inserted to the vtkPoints at all but instead only the point index is 
  // inserted to a vtkIdList upon 0. For case 0, the point index needs to be
  // specified via pntId. For case 1, the actual point index is returned via
  // pntId. InitPointInsertion() should have been called.
  void InsertPointWithoutChecking
    ( const double point[3], vtkIdType  & pntId, int insert );

//BTX
protected:

  vtkIncrementalOctreePointLocator();
  virtual ~vtkIncrementalOctreePointLocator();
  
private:

  int         BuildCubicOctree;
  int         MaxPointsPerLeaf;
  double      InsertTolerance2;
  double      OctreeMaxDimSize;
  double      FudgeFactor;
  vtkPoints * LocatorPoints;
  vtkIncrementalOctreeNode * OctreeRootNode;
  
  // Description:
  // Delete all descendants of a node.
  static void DeleteAllDescendants( vtkIncrementalOctreeNode * node );
  
  // Description:
  // Add the polygonal representation of a given node to the allocated vtkPoints
  // and vtkCellArray objects.
  static void AddPolys( vtkIncrementalOctreeNode * node, 
                        vtkPoints * points, vtkCellArray * polygs );
  
  // Description:
  // Given a point and a reference node, find the leaf containing the point.
  // Note the point is assumed to be inside or under the reference node.
  vtkIncrementalOctreeNode * GetLeafContainer( vtkIncrementalOctreeNode * node,
    const double pnt[3] ); 
  
  // Description:
  // Given a point (under check, either inside or outside the octree) and a leaf
  // node (not necessarily the container of this point), find the closest point
  // (possibly a duplicate of the point under check) within the node and return
  // the point index as well as the associated minimum squared distance (via dist2).
  // InitPointInsertion() or BuildLocator() should have been called.
  vtkIdType FindClosestPointInLeafNode( vtkIncrementalOctreeNode * leafNode, 
                                        const double point[3], double * dist2 );

  // Description:
  // This function may not be directly called. Please use the follwing two ones:
  // FindClosestPointInSphereWithTolerance() for point insertion and 
  // FindClosestPointInSphereWithoutTolerance() for point location. Arguments
  // refDist2 and the initialization of minDist2 determine which version is used.
  // Given a point (under check) and an already-checked node (possibly NULL), 
  // find the closest point across a set of neighboring nodes within a specified
  // squared radius to the given point --- to perform an extended within-radius
  // inter-node search. The leaf (mask) node itself is excluded from the search 
  // scope. Returned are the point index and the associated minimum squared
  // distance. InitPointInsertion() or BuildLocator() should have been called.
  vtkIdType FindClosestPointInSphere
    ( const double point[3], double radius2, vtkIncrementalOctreeNode * maskNode,
      double * minDist2, const double * refDist2 );
      
      
  // -------------------------------------------------------------------------
  // ---------------------------- Point  Location ----------------------------
  // -------------------------------------------------------------------------
  
  // Description:
  // This function is intended for point location, excluding point insertion.
  // Given a point (under check, covered or uncovered by the octree) and an
  // already-checked leaf node (maskNode, possibly NULL), find the closest point
  // across a set of neighboring nodes within a specified squared radius to the 
  // given point --- to perform an extended within-radius inter-node search. The
  // leaf (mask) node itself is excluded from the search scope. Returned are the 
  // point index and the associated minimum squared distance (via minDist2). Note
  // that BuildLocator() should have been called.
  vtkIdType FindClosestPointInSphereWithoutTolerance( const double point[3],
    double radius2, vtkIncrementalOctreeNode * maskNode, double * minDist2 );
    
  // Description:
  // Find all points, inside a given node, within a squared radius relative to
  // a given point. Returned are the associated un-sorted point indices (idList).
  // Note that BuildLocator() should have been called prior to this function.
  void FindPointsWithinSquaredRadius( vtkIncrementalOctreeNode * node, 
    double radius2, const double point[3], vtkIdList * idList );
  
  // -------------------------------------------------------------------------
  // ---------------------------- Point Insertion ----------------------------
  // -------------------------------------------------------------------------
  
  // Description:
  // This function is intended for point insertion, excluding point location.
  // Given a point (under check for insertion, must be covered by the octree)
  // and an already-checked node (maskNode, the container leaf node, possibly
  // NULL if no any node has been checked), find the closest point across a set
  // of neighbor nodes within a specified squared radius radius2 to the given 
  // point --- to perform an extended within-radius inter-node search. The leaf
  // (mask) node itself is excluded from the search scope. Returned are the point
  // index and the associated minimum squared distance (via minDist2). Note that
  // InitPointInsertion() should have been called.
  vtkIdType FindClosestPointInSphereWithTolerance( const double point[3],
    double radius2, vtkIncrementalOctreeNode * maskNode, double * minDist2 );
    
  // Description:
  // Determine whether or not a given point has been inserted into the octree.
  // Return the id of the already inserted point if true, otherwise return -1.
  // Argument leafContainer is useful for access only if -1 is returned. This
  // returned parameter indicates the leaf node that contains the given point. 
  // This function resorts to IsInsertedPointForZeroTolerance() for zero 
  // tolerance insertion or IsInsertedPointForNonZeroTolerance() for non-zero 
  // tolerance insertion. InitPointInsertion() should have been called.
  vtkIdType IsInsertedPoint( const double x[3], 
                             vtkIncrementalOctreeNode ** leafContainer );
  
  // Description:
  // Determine whether or not a given point has been inserted into the octree.
  // Return the id of the already inserted point if true, otherwise return -1.
  // Argument leafContainer is useful for access only if -1 is returned. This
  // returned parameter indicates the leaf node that contains the given point. 
  // This variant is invoked by IsInsertedPoint(x, vtkIncrementalOctreeNode **)
  // for zero tolerance insertion. InitPointInsertion() should have been called.
  vtkIdType IsInsertedPointForZeroTolerance
    ( const double x[3], vtkIncrementalOctreeNode ** leafContainer );
    
  // Description:
  // Determine whether or not a given point has been inserted into the octree.
  // Return the id of the already inserted point if true, otherwise return -1.
  // Argument leafContainer is useful for access only if -1 is returned. This
  // returned parameter indicates the leaf node that contains the given point. 
  // This variant is invoked by IsInsertedPoint(x, vtkIncrementalOctreeNode **) 
  // for non-zero tolerance insertion. InitPointInsertion() should have been 
  // called in advance.
  vtkIdType IsInsertedPointForNonZeroTolerance
    ( const double x[3], vtkIncrementalOctreeNode ** leafContainer );
                              
  // Description:
  // Given a point (under check for zero tolerance insertion) and a leaf node, 
  // find its duplicate, if any, in the node and return the point index (-1 if
  // no duplicate is found). Note that the leaf node, already with at least one 
  // point, is the container of the point under check. InitPointInsertion() 
  // should have been called.
  vtkIdType FindDuplicatePointInLeafNode( vtkIncrementalOctreeNode * leafNode, 
                                          const double point[3] );

  // Description:
  // Given a point (under check for zero tolerance insertion) and a leaf node, 
  // find its duplicate, if any, in the node and return the point index (-1 if
  // no duplicate is found). Note that the leaf node, already with at least one 
  // point, is the container of the point under check. This function is invoked 
  // for type VTK_FLOAT. InitPointInsertion() should have been called.
  vtkIdType FindDuplicateFloatTypePointInVisitedLeafNode
    ( vtkIncrementalOctreeNode * leafNode, const double point[3] );

  // Description:
  // Given a point (under check for zero tolerance insertion) and a leaf node, 
  // find its duplicate, if any, in the node and return the point index (-1 if 
  // no duplicate is found). Note that the leaf node, already with at least one 
  // point, is the container of the point under check. This function is invoked
  // for type VTK_DOUBLE. InitPointInsertion() should have been called.
  vtkIdType FindDuplicateDoubleTypePointInVisitedLeafNode
    ( vtkIncrementalOctreeNode * leafNode, const double point[3] );
                         
  vtkIncrementalOctreePointLocator
    ( const vtkIncrementalOctreePointLocator & ); // Not implemented
  void operator = ( const vtkIncrementalOctreePointLocator & );// Not implemented
//ETX
};
#endif
