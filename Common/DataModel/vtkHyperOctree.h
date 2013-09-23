/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperOctree.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHyperOctree - A dataset structured as a tree where each node has
// exactly 2^n children.
// .SECTION Description
// An hyperoctree is a dataset where each node has either exactly 2^n children
// or no child at all if the node is a leaf. `n' is the dimension of the
// dataset (1 (binary tree), 2 (quadtree) or 3 (octree) ).
// The class name comes from the following paper:
//
// \verbatim
// @ARTICLE{yau-srihari-1983,
//  author={Mann-May Yau and Sargur N. Srihari},
//  title={A Hierarchical Data Structure for Multidimensional Digital Images},
//  journal={Communications of the ACM},
//  month={July},
//  year={1983},
//  volume={26},
//  number={7},
//  pages={504--515}
//  }
// \endverbatim
//
// Each node is a cell. Attributes are associated with cells, not with points.
// The geometry is implicitly given by the size of the root node on each axis
// and position of the center and the orientation. (TODO: review center
// position and orientation). The geometry is then not limited to an hybercube
// but can have a rectangular shape.
// Attributes are associated with leaves. For LOD (Level-Of-Detail) purpose,
// attributes can be computed on none-leaf nodes by computing the average
// values from its children (which can be leaves or not).
//
// By construction, an hyperoctree is efficient in memory usage when the
// geometry is sparse. The LOD feature allows to cull quickly part of the
// dataset.
//
// A couple of filters can be applied on this dataset: contour, outline,
// geometry.
//
// * 3D case (octree)
// for each node, each child index (from 0 to 7) is encoded in the following
// orientation. It is easy to access each child as a cell of a grid.
// Note also that the binary representation is relevant, each bit code a
// side: bit 0 encodes -x side (0) or +x side (1)
// bit 1 encodes -y side (0) or +y side (1)
// bit 2 encodes -z side (0) or +z side (2)
// - the -z side first
// - 0: -y -x sides
// - 1: -y +x sides
// - 2: +y -x sides
// - 3: +y +x sides
// \verbatim
//              +y
// +-+-+        ^
// |2|3|        |
// +-+-+  O +z  +-> +x
// |0|1|
// +-+-+
// \endverbatim
//
// - then the +z side, in counter-clockwise
// - 4: -y -x sides
// - 5: -y +x sides
// - 6: +y -x sides
// - 7: +y +x sides
// \verbatim
//              +y
// +-+-+        ^
// |6|7|        |
// +-+-+  O +z  +-> +x
// |4|5|
// +-+-+
// \endverbatim
//
// The cases with fewer dimensions are consistent with the octree case:
//
// * Quadtree:
// in counter-clockwise
// - 0: -y -x edges
// - 1: -y +x edges
// - 2: +y -x edges
// - 3: +y +x edges
// \verbatim
//         +y
// +-+-+   ^
// |2|3|   |
// +-+-+  O+-> +x
// |0|1|
// +-+-+
// \endverbatim
//
// * Binary tree:
// \verbatim
// +0+1+  O+-> +x
// \endverbatim
//
// .SECTION Caveats
// It is not a spatial search object! If you looking for this kind of
// octree see vtkCellLocator instead.

// .SECTION See Also
// vtkHyperOctreeAlgorithm

#ifndef __vtkHyperOctree_h
#define __vtkHyperOctree_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataSet.h"

class vtkHyperOctreeLightWeightCursor;
class vtkHyperOctreeCursor;
class vtkHyperOctreeInternal;
class vtkHyperOctreePointsGrabber;

class vtkHyperOctreeIdSet; // Pimpl idiom
class vtkPolygon;
class vtkIdTypeArray;
class vtkPoints;
class vtkPointLocator;
class vtkOrderedTriangulator;
class vtkDataSetAttributes;

class vtkLine;
class vtkPixel;
class vtkVoxel;
class vtkCellLinks;

class VTKCOMMONDATAMODEL_EXPORT vtkHyperOctree : public vtkDataSet
{
public:
  static vtkInformationIntegerKey* LEVELS();
  static vtkInformationIntegerKey* DIMENSION();
  static vtkInformationDoubleVectorKey* SIZES();
  static vtkHyperOctree *New();

  vtkTypeMacro(vtkHyperOctree,vtkDataSet);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return what type of dataset this is.
  int GetDataObjectType();

  // Description:
  // Copy the geometric and topological structure of an input rectilinear grid
  // object.
  void CopyStructure(vtkDataSet *ds);

  // Return the node describes by the path from the root.
  // Path is a sequence of number between 0 and 7.
  // \pre path_exists: path!=0
  // \pre node_exists: IsANode(path)
//  vtkOctree *GetNode(vtkPath *path);

  // Description:
  // Return the dimension of the tree (1D:binary tree(2 children), 2D:quadtree(4 children),
  // 3D:octree (8 children))
  // \post valid_result: result>=1 && result<=3
  int GetDimension();

  // Description:
  // Set the dimension of the tree with `dim'. See GetDimension() for details.
  // \pre valid_dim: dim>=1 && dim<=3
  // \post dimension_is_set: GetDimension()==dim
  void SetDimension(int dim);

  // Return if the node for the given path exists or not.
  // \pre path_exists: path!=0
//  int IsANode(vtkPath *path);

  // Return if the node for the given path is a leaf or not.
  // \pre path_exists: path!=0
  // \pre node_exists: IsANode(path)
//  int IsALeaf(vtkPath *path);

  // Measurement: topology

  // Description:
  // Return the number of cells in the dual grid.
  // \post positive_result: result>=0
  vtkIdType GetNumberOfCells();

  // Description:
  // Get the number of leaves in the tree.
  vtkIdType GetNumberOfLeaves();

  // Description:
  // Return the number of points in the dual grid.
  // \post positive_result: result>=0
  vtkIdType GetNumberOfPoints();

  // Description:
  // Return the number of points corresponding to an hyperoctree starting at
  // level `level' where all the leaves at at the last level. In this case, the
  // hyperoctree is like a uniform grid. So this number is the number of points
  // of the uniform grid.
  // \pre positive_level: level>=0 && level<this->GetNumberOfLevels()
  // \post definition: result==(2^(GetNumberOfLevels()-level-1)+1)^GetDimension()
  vtkIdType GetMaxNumberOfPoints(int level);

  // Description:
  // Return the number of points corresponding to the boundary of an
  // hyperoctree starting at level `level' where all the leaves at at the last
  // level. In this case, the hyperoctree is like a uniform grid. So this
  // number is the number of points of on the boundary of the uniform grid.
  // For an octree, the boundary are the faces. For a quadtree, the boundary
  // are the edges.
  // \pre 2d_or_3d: this->GetDimension()==2 || this->GetDimension()==3
  // \pre positive_level: level>=0 && level<this->GetNumberOfLevels()
  // \post min_result: result>=GetMaxNumberOfPoints(this->GetNumberOfLevels()-1)
  // \post max_result: result<=GetMaxNumberOfPoints(level)
  vtkIdType GetMaxNumberOfPointsOnBoundary(int level);

  // Description:
  // Return the number of cells corresponding to the boundary of a cell
  // of level `level' where all the leaves at at the last level.
  // \pre positive_level: level>=0 && level<this->GetNumberOfLevels()
  // \post positive_result: result>=0
  vtkIdType GetMaxNumberOfCellsOnBoundary(int level);

  // Description:
  // Return the number of levels.
  // \post result_greater_or_equal_to_one: result>=1
  vtkIdType GetNumberOfLevels();

  // Measurement: geometry

  // Description:
  // Set the size on each axis.
  vtkSetVector3Macro(Size,double);

  // Description:
  // Return the size on each axis.
  vtkGetVector3Macro(Size,double);

  // Description:
  // Set the origin (position of corner (0,0,0) of the root.
  vtkSetVector3Macro(Origin,double);
  // Return the origin (position of corner (0,0,0) ) of the root.
  vtkGetVector3Macro(Origin,double);

  // Description:
  // Create a new cursor: an object that can traverse
  // the cell of an hyperoctree.
  // \post result_exists: result!=0
  vtkHyperOctreeCursor *NewCellCursor();

  // Description:
  // Subdivide node pointed by cursor, only if its a leaf.
  // At the end, cursor points on the node that used to be leaf.
  // \pre leaf_exists: leaf!=0
  // \pre is_a_leaf: leaf->CurrentIsLeaf()
  void SubdivideLeaf(vtkHyperOctreeCursor *leaf);

  // Description:
  // Collapse a node for which all children are leaves.
  // At the end, cursor points on the leaf that used to be a node.
  // \pre node_exists: node!=0
  // \pre node_is_node: !node->CurrentIsLeaf()
  // \pre children_are_leaves: node->CurrentIsTerminalNode()
  void CollapseTerminalNode(vtkHyperOctreeCursor *node);

  // Description:
  // Get point coordinates with ptId such that: 0 <= ptId < NumberOfPoints.
  // THIS METHOD IS NOT THREAD SAFE.
  virtual double *GetPoint(vtkIdType ptId);

  // Description:
  // Copy point coordinates into user provided array x[3] for specified
  // point id.
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  virtual void GetPoint(vtkIdType id, double x[3]);

  // Description:
  // Get cell with cellId such that: 0 <= cellId < NumberOfCells.
  // THIS METHOD IS NOT THREAD SAFE.
  virtual vtkCell *GetCell(vtkIdType cellId);

  // Description:
  // Get cell with cellId such that: 0 <= cellId < NumberOfCells.
  // This is a thread-safe alternative to the previous GetCell()
  // method.
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  virtual void GetCell(vtkIdType cellId, vtkGenericCell *cell);


  // Description:
  // Get type of cell with cellId such that: 0 <= cellId < NumberOfCells.
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  virtual int GetCellType(vtkIdType cellId);

  // Description:
  // Topological inquiry to get points defining cell.
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  virtual void GetCellPoints(vtkIdType cellId, vtkIdList *ptIds);
  virtual void GetCellPoints(vtkIdType cellId, vtkIdType& npts,
                             vtkIdType* &pts);

  // Description:
  // Topological inquiry to get cells using point.
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  virtual void GetPointCells(vtkIdType ptId, vtkIdList *cellIds);


  // Description:
  // Topological inquiry to get all cells using list of points exclusive of
  // cell specified (e.g., cellId). Note that the list consists of only
  // cells that use ALL the points provided.
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  virtual void GetCellNeighbors(vtkIdType cellId, vtkIdList *ptIds,
                                vtkIdList *cellIds);

  virtual vtkIdType FindPoint(double x[3]);

  // Description:
  // Locate cell based on global coordinate x and tolerance
  // squared. If cell and cellId is non-NULL, then search starts from
  // this cell and looks at immediate neighbors.  Returns cellId >= 0
  // if inside, < 0 otherwise.  The parametric coordinates are
  // provided in pcoords[3]. The interpolation weights are returned in
  // weights[]. (The number of weights is equal to the number of
  // points in the found cell). Tolerance is used to control how close
  // the point is to be considered "in" the cell.
  // THIS METHOD IS NOT THREAD SAFE.
  virtual vtkIdType FindCell(double x[3], vtkCell *cell, vtkIdType cellId,
                             double tol2, int& subId, double pcoords[3],
                             double *weights);

  // Description:
  // This is a version of the above method that can be used with
  // multithreaded applications. A vtkGenericCell must be passed in
  // to be used in internal calls that might be made to GetCell()
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  virtual vtkIdType FindCell(double x[3], vtkCell *cell,
                             vtkGenericCell *gencell, vtkIdType cellId,
                             double tol2, int& subId, double pcoords[3],
                             double *weights);

  // Description:
  // Restore data object to initial state,
  // THIS METHOD IS NOT THREAD SAFE.
  void Initialize();

  // Description:
  // Convenience method returns largest cell size in dataset. This is generally
  // used to allocate memory for supporting data structures.
  // This is the number of points of a cell.
  // THIS METHOD IS THREAD SAFE
  virtual int GetMaxCellSize();

  // Description:
  // Shallow and Deep copy.
  void ShallowCopy(vtkDataObject *src);
  void DeepCopy(vtkDataObject *src);

  // Description:
  // Get the points of node `sibling' on its face `face'.
  // \pre sibling_exists: sibling!=0
  // \pre sibling_not_leaf: !sibling->CurrentIsLeaf()
  // \pre sibling_3d: sibling->GetDimension()==3
  // \pre valid_face: face>=0 && face<6
  // \pre valid_level_not_leaf: level>=0 level<(this->GetNumberOfLevels()-1)
  void GetPointsOnFace(vtkHyperOctreeCursor *sibling,
                       int face,
                       int level,
                       vtkHyperOctreePointsGrabber *grabber);

  // Description:
  // Get the points of the parent node of `cursor' on its faces `faces' at
  // level `level' or deeper.
  // \pre cursor_exists: cursor!=0
  // \pre cursor_3d: cursor->GetDimension()==3
  // \pre valid_level: level>=0
  // \pre boolean_faces: (faces[0]==0 || faces[0]==1) && (faces[1]==0 || faces[1]==1) && (faces[2]==0 || faces[2]==1)
  void GetPointsOnParentFaces(int faces[3],
                              int level,
                              vtkHyperOctreeCursor *cursor,
                              vtkHyperOctreePointsGrabber *grabber);

  // Description:
  // Get the points of node `sibling' on its edge `axis','k','j'.
  // If axis==0, the edge is X-aligned and k gives the z coordinate and j the
  // y-coordinate. If axis==1, the edge is Y-aligned and k gives the x coordinate
  // and j the z coordinate. If axis==2, the edge is Z-aligned and k gives the
  // y coordinate and j the x coordinate.
  // \pre sibling_exists: sibling!=0
  // \pre sibling_3d: sibling->GetDimension()==3
  // \pre sibling_not_leaf: !sibling->CurrentIsLeaf()
  // \pre valid_axis: axis>=0 && axis<3
  // \pre valid_k: k>=0 && k<=1
  // \pre valid_j: j>=0 && j<=1
  // \pre valid_level_not_leaf: level>=0 level<(this->Input->GetNumberOfLevels()-1)
  void GetPointsOnEdge(vtkHyperOctreeCursor *sibling,
                       int level,
                       int axis,
                       int k,
                       int j,
                       vtkHyperOctreePointsGrabber *grabber);

  // Description:
  // Get the points of the parent node of `cursor' on its edge `axis','k','j'
  // at level `level' or deeper.
  // If axis==0, the edge is X-aligned and k gives the z coordinate and j the
  // y-coordinate. If axis==1, the edge is Y-aligned and k gives the x
  // coordinate and j the z coordinate. If axis==2, the edge is Z-aligned and
  // k gives the y coordinate and j the x coordinate.
  // \pre cursor_exists: cursor!=0
  // \pre cursor_3d: cursor->GetDimension()==3
  // \pre valid_level: level>=0
  // \pre valid_range_axis: axis>=0 && axis<3
  // \pre valid_range_k: k>=0 && k<=1
  // \pre valid_range_j: j>=0 && j<=1
  void GetPointsOnParentEdge(vtkHyperOctreeCursor *cursor,
                             int level,
                             int axis,
                             int k,
                             int j,
                             vtkHyperOctreePointsGrabber *grabber);

  // Description:
  // Get the points of node `sibling' on its edge `edge'.
  // \pre sibling_exists: sibling!=0
  // \pre sibling_not_leaf: !sibling->CurrentIsLeaf()
  // \pre sibling_2d: sibling->GetDimension()==2
  // \pre valid_edge: edge>=0 && edge<4
  // \pre valid_level_not_leaf: level>=0 level<(this->Input->GetNumberOfLevels()-1)
  void GetPointsOnEdge2D(vtkHyperOctreeCursor *sibling,
                         int edge,
                         int level,
                         vtkHyperOctreePointsGrabber *grabber);

  // Description:
  // Get the points of the parent node of `cursor' on its edge `edge' at
  // level `level' or deeper. (edge=0 for -X, 1 for +X, 2 for -Y, 3 for +Y)
  // \pre cursor_exists: cursor!=0
  // \pre cursor_2d: cursor->GetDimension()==2
  // \pre valid_level: level>=0
  // \pre valid_edge: edge>=0 && edge<4
  void GetPointsOnParentEdge2D(vtkHyperOctreeCursor *cursor,
                               int edge,
                               int level,
                               vtkHyperOctreePointsGrabber *grabber);

  // Description:
  // A generic way to set the leaf data attributes.
  // This can be either point data for dual or cell data for normal grid.
  vtkDataSetAttributes* GetLeafData();

  // Description:
  // Switch between returning leaves as cells, or the dual grid.
  void SetDualGridFlag(int flag);
  vtkGetMacro(DualGridFlag,int);

  // Description:
  // Return the actual size of the data in kilobytes. This number
  // is valid only after the pipeline has updated. The memory size
  // returned is guaranteed to be greater than or equal to the
  // memory required to represent the data (e.g., extra space in
  // arrays, etc. are not included in the return value). THIS METHOD
  // IS THREAD SAFE.
  unsigned long GetActualMemorySize();

  //BTX
  // Description:
  // Retrieve an instance of this class from an information object.
  static vtkHyperOctree* GetData(vtkInformation* info);
  static vtkHyperOctree* GetData(vtkInformationVector* v, int i=0);
  //ETX

protected:
  // Constructor with default bounds (0,1, 0,1, 0,1).
  vtkHyperOctree();
  ~vtkHyperOctree();

  void ComputeBounds();

  int Dimension; // 1, 2 or 3.

  double Size[3]; // size on each axis
  double Origin[3]; // position of corner (0,0,0) of the root.

  vtkHyperOctreeInternal *CellTree;

  vtkHyperOctreeCursor *TmpChild; // to avoid allocation in the loop

  //BTX
  friend class vtkHyperOctreeLightWeightCursor;
  //ETX

  // Initialize the arrays if necessary, then return it.
  void UpdateDualArrays();
  vtkPoints* GetLeafCenters();
  vtkIdTypeArray* GetCornerLeafIds();
  vtkPoints *LeafCenters;
  vtkIdTypeArray *CornerLeafIds;

  void UpdateGridArrays();
  vtkPoints* GetCornerPoints();
  vtkIdTypeArray* GetLeafCornerIds();
  vtkPoints* CornerPoints;
  vtkIdTypeArray* LeafCornerIds;

  void DeleteInternalArrays();

  void TraverseDualRecursively(vtkHyperOctreeLightWeightCursor* neighborhood,
                               unsigned short *xyzIds, int level);
  void TraverseGridRecursively(vtkHyperOctreeLightWeightCursor* neighborhood,
                               unsigned char* visited,
                               double* origin, double* size);
  void EvaluateDualCorner(vtkHyperOctreeLightWeightCursor* neighborhood);
  vtkIdType EvaluateGridCorner(int level,vtkHyperOctreeLightWeightCursor* neighborhood,
                               unsigned char* visited, int* cornerNeighborIds);

  // This is a table for traversing a neighborhood down an octree.
  // 8 children x 27 cursors
  // First three bits encode the child,  rest encode the cursor id.
  // 8xCursorId + childId.
  // This will be shorter when we get rid of the 3x3x3 neighborhood.
  // I was using unsigned char, but VS60 optimized build had a problem.
  int NeighborhoodTraversalTable[216];
  void GenerateGridNeighborhoodTraversalTable();
  void GenerateDualNeighborhoodTraversalTable();

  // for the GetCell method
  vtkLine *Line;
  vtkPixel *Pixel;
  vtkVoxel *Voxel;

  vtkCellLinks* Links;
  void BuildLinks();

  vtkIdType RecursiveFindPoint(double x[3],
    vtkHyperOctreeLightWeightCursor* cursor,
    double *origin, double *size);

  // This toggles the data set API between the leaf cells and
  // the dual grid (leaves are points, corners are cells).
  int DualGridFlag;

private:
  vtkHyperOctree(const vtkHyperOctree&);  // Not implemented.
  void operator=(const vtkHyperOctree&);    // Not implemented.
};


//BTX

class VTKCOMMONDATAMODEL_EXPORT vtkHyperOctreeLightWeightCursor
{
public:
  vtkHyperOctreeLightWeightCursor();
  void Initialize(vtkHyperOctree* tree);
  void ToRoot();
  void ToChild(int child);
  unsigned short GetIsLeaf();
  int GetLeafIndex() {return this->Index;} // Only valid for leaves.
  vtkHyperOctree* GetTree() { return this->Tree; }
  unsigned short GetLevel() {return this->Level;}
private:
  vtkHyperOctree* Tree;
  int Index;
  unsigned short IsLeaf;
  unsigned short Level;
};

//ETX

#endif
