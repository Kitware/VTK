/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHyperTreeGrid - A dataset structured as a tree where each node has
// exactly either 2^n or 3^n children.
//
// .SECTION Description
// An hypertree is a dataset where each node has either exactly 2^n or 3^n children
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
// By construction, an hypertree is efficient in memory usage when the
// geometry is sparse. The LOD feature allows to cull quickly part of the
// dataset.
//
// Some filters can be applied on this dataset: contour, outline, geometry.
//
// .SECTION Case with 2^n children
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
// It is not a spatial search object. If you are looking for this kind of
// octree see vtkCellLocator instead.
//
// .SECTION Thanks
// This test was written by Philippe Pebay and Charles Law, Kitware 2012
// This work was supported in part by Commissariat a l'Energie Atomique (CEA/DIF)

#ifndef __vtkHyperTreeGrid_h
#define __vtkHyperTreeGrid_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataSet.h"

class vtkHyperTreeLightWeightCursor;
//BTX
class vtkHyperTreeSuperCursor;
//ETX
class vtkHyperTreeCursor;
class vtkHyperTreeInternal;

class vtkDataArray;
class vtkIdTypeArray;
class vtkPoints;
class vtkDataSetAttributes;

class vtkLine;
class vtkPixel;
class vtkVoxel;
class vtkCellLinks;



// Used to advance the super cursor; One Entry per cursor node.
// Private.
class vtkSuperCursorEntry
{
public:
  // For the new node, start with the node in super cursor as parent.
  unsigned char Parent;
  // Traverse to this child.
  unsigned char Child;
};

class VTKCOMMONDATAMODEL_EXPORT vtkHyperTreeGrid : public vtkDataSet
{
public:
  static vtkInformationIntegerKey* LEVELS();
  static vtkInformationIntegerKey* DIMENSION();
  static vtkInformationDoubleVectorKey* SIZES();
  static vtkHyperTreeGrid *New();

  vtkTypeMacro(vtkHyperTreeGrid,vtkDataSet);
  void PrintSelf( ostream& os, vtkIndent indent );

  // Description:
  // Return what type of dataset this is.
  int GetDataObjectType();

  // Description:
  // Copy the geometric and topological structure of an input rectilinear grid
  // object.
  void CopyStructure(vtkDataSet *ds);

  // Description:
  // Set/Get sizes of this rectilinear grid dataset
  void SetGridSize( int[3] );
  vtkGetVector3Macro(GridSize,int);

  // Description:
  // Return the dimension of the tree (1D:binary tree(2 children), 2D:quadtree(4 children),
  // 3D:octree (8 children))
  // \post valid_result: result>=1 && result<=3
  int GetDimension();

  // Description:
  // Set the dimension of the tree with `dim'. See GetDimension() for details.
  // \pre valid_dim: dim>=1 && dim<=3
  // \post dimension_is_set: GetDimension()==dim
  void SetDimension(int);

  // Description:
  // Branch factor can be 2 or 3. (Octree, quadtree) or (nontree or 27tree)
  vtkGetMacro(AxisBranchFactor,int);
  void SetAxisBranchFactor(int factor);

  // Description:
  // Return the number of cells in the dual grid or grid.
  // This call should be avoided for dual grids.  Estimate
  // the number of cells from the number of leaves and use the dual
  // grid cell iterator.
  // \post positive_result: result>=0
  vtkIdType GetNumberOfCells();

  // Description:
  // Get the number of leaves in the tree grid.
  int GetNumberOfLeaves();

  // Description:
  // Return the number of points in the dual grid or grid.
  // This call should be avoided for the normal grid.
  // \post positive_result: result>=0
  vtkIdType GetNumberOfPoints();

  // Description:
  // Return the number of levels.
  // \post result_greater_or_equal_to_one: result>=1
  int GetNumberOfLevels( int );

  // Description:
  // Specify the grid coordinates in the x-direction.
  virtual void SetXCoordinates( vtkDataArray* );
  vtkGetObjectMacro(XCoordinates,vtkDataArray);

  // Description:
  // Specify the grid coordinates in the y-direction.
  virtual void SetYCoordinates( vtkDataArray* );
  vtkGetObjectMacro(YCoordinates,vtkDataArray);

  // Description:
  // Specify the grid coordinates in the z-direction.
  virtual void SetZCoordinates( vtkDataArray* );
  vtkGetObjectMacro(ZCoordinates,vtkDataArray);

  // Description:
  // Law: I feel this is internal and should not be exposed in the API.
  // Create a new cursor: an object that can traverse
  // the cells of an individual hyper tree.
  // \post result_exists: result!=0
  vtkHyperTreeCursor *NewCellCursor( int, int, int );

  // Description:
  // Subdivide node pointed by cursor, only if its a leaf.
  // At the end, cursor points on the node that used to be leaf.
  // \pre leaf_exists: leaf!=0
  // \pre is_a_leaf: leaf->CurrentIsLeaf()
  void SubdivideLeaf(vtkHyperTreeCursor *leaf, vtkIdType);

  // Description:
  // This method should be avoided in favor of cell/point iterators.
  // Random access to points requires that arrays are created explicitly.
  // Get point coordinates with ptId such that: 0 <= ptId < NumberOfPoints.
  // THIS METHOD IS NOT THREAD SAFE.
  virtual double *GetPoint(vtkIdType ptId);

  // Description:
  // This method should be avoided in favor of cell/point iterators.
  // Random access to points requires that arrays are created explicitly.
  // Copy point coordinates into user provided array x[3] for specified
  // point id.
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  virtual void GetPoint(vtkIdType id, double x[3]);

  // Description:
  // This method should be avoided in favor of cell/point iterators.
  // Random access to cells requires that connectivity arrays are created explicitly.
  // Get cell with cellId such that: 0 <= cellId < NumberOfCells.
  // THIS METHOD IS NOT THREAD SAFE.
  virtual vtkCell *GetCell(vtkIdType cellId);

  // Description:
  // This method should be avoided in favor of cell/point iterators.
  // Random access to cells requires that connectivity arrays are created explicitly.
  // Get cell with cellId such that: 0 <= cellId < NumberOfCells. 
  // This is a thread-safe alternative to the previous GetCell()
  // method.
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  virtual void GetCell(vtkIdType cellId, vtkGenericCell *cell);

  // Description:
  // All cell types are 2: quadrilaters,3d: hexahedrons.  They may be degenerate though.
  // Get type of cell with cellId such that: 0 <= cellId < NumberOfCells.
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  virtual int GetCellType(vtkIdType cellId);

  // Description:
  // This method should be avoided in favor of cell/point iterators.
  // Random access to cells requires that connectivity arrays are created explicitly.
  // Topological inquiry to get points defining cell.
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  virtual void GetCellPoints(vtkIdType cellId, vtkIdList *ptIds);
  virtual void GetCellPoints(vtkIdType cellId, vtkIdType& npts,
                             vtkIdType* &pts);

  // Description:
  // This method should be avoided in favor of cell/point iterators.
  // Random access to cells requires that connectivity arrays are created explicitly.
  // Topological inquiry to get cells using point.
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  virtual void GetPointCells(vtkIdType ptId, vtkIdList *cellIds);

  // Description:
  // This method should be avoided in favor of cell/point iterators.
  // Random access to cells requires that connectivity arrays are created explicitly.
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
  // Initialize a super cursor to point to one of the root trees
  // in the grid.  The super cursor points to a node in a tree and
  // also keeps pointers to the 26 neighbors of said node.
  void InitializeSuperCursor(vtkHyperTreeSuperCursor* superCursor, int i, int j, int k);
//ETX
  // Description:
  // Generate the table before calling InitializeSuperCursorChild.
  void GenerateSuperCursorTraversalTable();
//BTX
  // Description:
  // Initialize a cursor to point to a child of an existing super cursor.
  // This will not work in place.
  void InitializeSuperCursorChild(vtkHyperTreeSuperCursor* parent,
                                  vtkHyperTreeSuperCursor* child,
                                  int childIdx);
//ETX
  // Description:
  // The number of children each node can have.
  vtkGetMacro(NumberOfChildren,int);


protected:
  // Constructor with default bounds (0,1, 0,1, 0,1).
  vtkHyperTreeGrid();
  ~vtkHyperTreeGrid();

  void ComputeBounds();
  void UpdateTree();

  int Dimension;    // 1, 2 or 3.
  int GridSize[3];
  int AxisBranchFactor;
  int NumberOfChildren;

  vtkDataArray *XCoordinates;
  vtkDataArray *YCoordinates;
  vtkDataArray *ZCoordinates;

  vtkHyperTreeInternal** CellTree;
  vtkIdType* CellTreeLeafIdOffsets;

  //BTX
  friend class vtkHyperTreeLightWeightCursor;
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

  int UpdateCellTreeLeafIdOffsets();

  void DeleteInternalArrays();
//BTX
  void TraverseDualRecursively( vtkHyperTreeSuperCursor*,
                                int );
  void TraverseGridRecursively( vtkHyperTreeSuperCursor*,
                                unsigned char*);
  void EvaluateDualCorner( vtkHyperTreeLightWeightCursor* );
  vtkIdType EvaluateGridCorner( int,
                                vtkHyperTreeSuperCursor*,
                                unsigned char*,
                                int* );
//ETX
  // Generalizing for 27 tree.  I cannot use 3 bits to encode the child to move to.
  // Input: root in supercursor(3x3x3=27), child(3x3x3=27)
  // Output: root, child
  // It is easier to abstract dimensions when we use a single array.
  vtkSuperCursorEntry SuperCursorTraversalTable[729]; // 27*27

  // for the GetCell method
  vtkLine *Line;
  vtkPixel *Pixel;
  vtkVoxel *Voxel;

  // I would like to get rid of this.
  // Is it a part of the vtkDataSet API?
  vtkCellLinks* Links;
  void BuildLinks();

  vtkIdType RecursiveFindPoint(double x[3],
    vtkHyperTreeLightWeightCursor* cursor,
    double *origin, double *size);

  // This toggles the data set API between the leaf cells and
  // the dual grid (leaves are points, corners are cells). 
  int DualGridFlag;

private:
  vtkHyperTreeGrid(const vtkHyperTreeGrid&);  // Not implemented.
  void operator=(const vtkHyperTreeGrid&);    // Not implemented.
};


//BTX

class VTK_EXPORT vtkHyperTreeLightWeightCursor
{
public:
  vtkHyperTreeLightWeightCursor();
  ~vtkHyperTreeLightWeightCursor();

  void Clear();
  void Initialize( vtkHyperTreeGrid*, vtkIdType*, int, int, int, int );
  void ToRoot();
  void ToChild( int );
  unsigned short GetIsLeaf();
  vtkHyperTreeInternal* GetTree() { return this->Tree; }
  int GetLeafIndex() { return this->Index; } // Only valid for leaves.

  int GetGlobalLeafIndex() { return this->Offset + this->Index; }
  vtkIdType GetOffset() { return this->Offset; }
  unsigned short GetLevel() { return this->Level; }
private:
  vtkHyperTreeInternal* Tree;
  int Index;
  vtkIdType Offset;
  unsigned short IsLeaf;
  unsigned short Level;
};


// Public structure filters used to move around the tree.
// The super cursor keeps neighbor cells so filters can
// easily access neighbor to leaves.
// The super cursor is static.  Methods in vtkHyperTreeGrid
// initialize and compute children for moving toward leaves.
class vtkHyperTreeSuperCursor
{
 public:
  vtkHyperTreeLightWeightCursor Cursors[27];
  int NumberOfCursors;
  int MiddleCursorId;
  double Origin[3];
  double Size[3];
  vtkHyperTreeLightWeightCursor* GetCursor( int idx ) { return this->Cursors + this->MiddleCursorId + idx; }
};




//ETX

#endif
