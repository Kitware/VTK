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
// .NAME vtkHyperTreeGrid - A dataset containing a grid of vtkHyperTree instances
// arranged as a rectilinear grid.
//
// .SECTION Description
// An hypertree grid is a dataset containing a rectilinear grid of root nodes,
// each of which can be refined as a vtkHyperTree grid. This organization of the
// root nodes allows for the definition of tree-based AMR grids that do not have
// uniform geometry.
// Some filters can be applied on this dataset: contour, outline, geometry.
//
// .SECTION Caveats
// It is not a spatial search object. If you are looking for this kind of
// octree see vtkCellLocator instead.
//
// .SECTION See Also
// vtkHyperTree vtkRectilinearGrid
//
// .SECTION Thanks
// This class was written by Philippe Pebay and Charles Law, Kitware 2013
// This work was supported in part by Commissariat a l'Energie Atomique (CEA/DIF)

#ifndef __vtkHyperTreeGrid_h
#define __vtkHyperTreeGrid_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataSet.h"
#include <map> // STL header for dual point coordinates ajustment

class vtkHyperTreeCursor;
class vtkHyperTree;

class vtkBitArray;
class vtkCellLinks;
class vtkCollection;
class vtkDataArray;
class vtkDataSetAttributes;
class vtkIdTypeArray;
class vtkLine;
class vtkPixel;
class vtkPoints;
class vtkVoxel;

class VTKCOMMONDATAMODEL_EXPORT vtkHyperTreeGrid : public vtkDataSet
{
public:
//BTX
  class vtkHyperTreeSimpleCursor;
  struct vtkHyperTreeGridSuperCursor;
//ETX

  static vtkInformationIntegerKey* LEVELS();
  static vtkInformationIntegerKey* DIMENSION();
  static vtkInformationDoubleVectorKey* SIZES();
  static vtkHyperTreeGrid* New();

  vtkTypeMacro(vtkHyperTreeGrid, vtkDataSet);
  void PrintSelf( ostream&, vtkIndent );

  // Description:
  // Return what type of dataset this is.
  int GetDataObjectType();

  // Description:
  // Copy the internal geometric and topological structure of a
  // vtkHyperTreeGrid object.
  void CopyStructure( vtkDataSet* );

  // Description:
  // Set/Get sizes of this rectilinear grid dataset
  void SetGridSize( unsigned int[3] );
  vtkGetVector3Macro(GridSize, unsigned int);

  // Description:
  // Set/Get the subdivision factor in the grid refinement scheme
  // NB: Can only be 2 or 3
  void SetBranchFactor( unsigned int );
  vtkGetMacro(BranchFactor, unsigned int);

  // Description:
  // Set/Get the dimensionality of the grid
  // NB: Can only be 1, 2 or 3
  void SetDimension( unsigned int );
  vtkGetMacro(Dimension, unsigned int);

  // Description:
  // Get number of root cells
  vtkGetMacro(NumberOfRoots, unsigned int);

  // Description:
  // Return the number of cells in the dual grid.
  vtkIdType GetNumberOfCells();

  // Description:
  // Return the number of points in the dual grid.
  vtkIdType GetNumberOfPoints();

  // Description:
  // Get the number of leaves in the primal tree grid.
  unsigned int GetNumberOfLeaves();

  // Description:
  // Return the number of levels in an individual (primal) tree
  unsigned int GetNumberOfLevels( unsigned int );

  // Description:
  // Specify the grid coordinates in the x-direction.
  void SetXCoordinates( vtkDataArray* );
  vtkGetObjectMacro(XCoordinates, vtkDataArray);

  // Description:
  // Specify the grid coordinates in the y-direction.
  void SetYCoordinates( vtkDataArray* );
  vtkGetObjectMacro(YCoordinates, vtkDataArray);

  // Description:
  // Specify the grid coordinates in the z-direction.
  void SetZCoordinates( vtkDataArray* );
  vtkGetObjectMacro(ZCoordinates, vtkDataArray);

  // Description:
  // Specify the blanking mask of primal leaf cells
  void SetMaterialMask( vtkBitArray* );
  vtkGetObjectMacro(MaterialMask, vtkBitArray);

  // Description:
  // Create a new cursor: an object that can traverse
  // the cells of an individual hyper tree.
  // \post result_exists: result!=0
  vtkHyperTreeCursor* NewCursor( int );

  // Description:
  // Subdivide node pointed by cursor, only if its a leaf.
  // At the end, cursor points on the node that used to be leaf.
  // \pre leaf_exists: leaf!=0
  // \pre is_a_leaf: leaf->CurrentIsLeaf()
  void SubdivideLeaf( vtkHyperTreeCursor*, vtkIdType );

  // Description:
  // This method should be avoided in favor of cell/point iterators.
  // Random access to points requires that arrays are created explicitly.
  // Get point coordinates with ptId such that: 0 <= ptId < NumberOfPoints.
  // THIS METHOD IS NOT THREAD SAFE.
  virtual double* GetPoint( vtkIdType );

  // Description:
  // This method should be avoided in favor of cell/point iterators.
  // Random access to points requires that arrays are created explicitly.
  // Copy point coordinates into user provided array x[3] for specified
  // point id.
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  virtual void GetPoint( vtkIdType, double[3] );

  // Description:
  // This method should be avoided in favor of cell/point iterators.
  // Random access to cells requires that connectivity arrays are created explicitly.
  // Get cell with cellId such that: 0 <= cellId < NumberOfCells.
  // THIS METHOD IS NOT THREAD SAFE.
  virtual vtkCell* GetCell( vtkIdType );

  // Description:
  // This method should be avoided in favor of cell/point iterators.
  // Random access to cells requires that connectivity arrays are created explicitly.
  // Get cell with cellId such that: 0 <= cellId < NumberOfCells.
  // This is a thread-safe alternative to the previous GetCell()
  // method.
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  virtual void GetCell( vtkIdType, vtkGenericCell* );

  // Description:
  // All cell types are 2: quadrilaters,3d: hexahedrons.  They may be degenerate though.
  // Get type of cell with cellId such that: 0 <= cellId < NumberOfCells.
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  virtual int GetCellType( vtkIdType );

  // Description:
  // This method should be avoided in favor of cell/point iterators.
  // Random access to cells requires that connectivity arrays are created explicitly.
  // Topological inquiry to get points defining cell.
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  virtual void GetCellPoints( vtkIdType, vtkIdList* );

  // Description:
  // Return a pointer to a list of point ids defining cell.
  // NB: More efficient than alternative method.
  virtual void GetCellPoints( vtkIdType, vtkIdType&, vtkIdType*& );

  // Description:
  // This method should be avoided in favor of cell/point iterators.
  // Random access to cells requires that connectivity arrays are created explicitly.
  // Topological inquiry to get cells using point.
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  virtual void GetPointCells( vtkIdType, vtkIdList* );

  // Description:
  // This method should be avoided in favor of cell/point iterators.
  // Random access to cells requires that connectivity arrays are created explicitly.
  // Topological inquiry to get all cells using list of points exclusive of
  // cell specified (e.g., cellId). Note that the list consists of only
  // cells that use ALL the points provided.
  // This is exactly the same as GetCellNeighbors in unstructured grid.
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  virtual void GetCellNeighbors( vtkIdType, vtkIdList*, vtkIdList* );

  // Description:
  // Find cell to which this point belongs, or at least closest one,
  // even if the point is outside the grid.
  // Since dual points are leaves, use the structure of the Tree instead
  // of a point locator.
  virtual vtkIdType FindPoint( double x[3] );

  // Description:
  // Locate cell based on global coordinate x and tolerance
  // squared. If cell and cellId is non-NULL, then search starts from
  // this cell and looks at immediate neighbors.  Returns cellId >= 0
  // if inside, < 0 otherwise.  The parametric coordinates are
  // provided in pcoords[3]. The interpolation weights are returned in
  // weights[]. (The number of weights is equal to the number of
  // points in the found cell). Tolerance is used to control how close
  // the point is to be considered "in" the cell.
  // NB: There is actually no need for a starting cell, just use the
  // point, as the tree structure is efficient enough.
// THIS METHOD IS NOT THREAD SAFE.
  virtual vtkIdType FindCell( double x[3], vtkCell *cell, vtkIdType cellId,
                              double tol2, int& subId, double pcoords[3],
                              double *weights );

  // Description:
  // This is a version of the above method that can be used with
  // multithreaded applications. A vtkGenericCell must be passed in
  // to be used in internal calls that might be made to GetCell()
  // THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
  // THE DATASET IS NOT MODIFIED
  virtual vtkIdType FindCell( double x[3], vtkCell *cell,
                              vtkGenericCell *gencell, vtkIdType cellId,
                              double tol2, int& subId, double pcoords[3],
                              double *weights );

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
  void ShallowCopy( vtkDataObject* );
  void DeepCopy( vtkDataObject* );

  // Description:
  // Return the actual size of the data in kilobytes. This number
  // is valid only after the pipeline has updated. The memory size
  // returned is guaranteed to be greater than or equal to the
  // memory required to represent the data (e.g., extra space in
  // arrays, etc. are not included in the return value). THIS METHOD
  // IS THREAD SAFE.
  unsigned long GetActualMemorySize();

  // Description:
  // Generate the table before calling InitializeSuperCursorChild.
  void GenerateSuperCursorTraversalTable();

//BTX
#ifndef __WRAP__
  // Description:
  // Initialize a super cursor to point to one of the root trees
  // in the grid.  The super cursor points to a node in a tree and
  // also keeps pointers to the 26 neighbors of said node.
  void InitializeSuperCursor( vtkHyperTreeGridSuperCursor*,
                              unsigned int,
                              unsigned int,
                              unsigned int,
                              unsigned int );
  // Description:
  // Initialize a cursor to point to a child of an existing super cursor.
  // This will not work in place.
  void InitializeSuperCursorChild( vtkHyperTreeGridSuperCursor* parent,
                                   vtkHyperTreeGridSuperCursor* child,
                                   int childIdx );
#endif
//ETX
  // Description:
  // The number of children each node can have.
  vtkGetMacro(NumberOfChildren, int);

protected:
  // Constructor with default bounds (0,1, 0,1, 0,1).
  vtkHyperTreeGrid();
  ~vtkHyperTreeGrid();

  void ComputeBounds();
  void UpdateTree();

  void GetCell( vtkIdType, vtkCell* );

  void ComputeDualGrid();
  vtkPoints* GetPoints();
  vtkIdTypeArray* GetConnectivity();

  unsigned int Dimension;    // 1, 2 or 3.
  unsigned int GridSize[3];
  unsigned int NumberOfRoots;
  unsigned int BranchFactor;
  unsigned int NumberOfChildren;

  vtkBitArray* MaterialMask;

  vtkDataArray* XCoordinates;
  vtkDataArray* YCoordinates;
  vtkDataArray* ZCoordinates;

  vtkCollection* HyperTrees;
  vtkIdType* HyperTreesLeafIdOffsets;

  vtkPoints* Points;
  vtkIdTypeArray* Connectivity;
  std::map<vtkIdType, bool> PointShifted;
  std::map<vtkIdType, double> PointShifts[3];
  std::map<vtkIdType, double> ReductionFactors;

  int UpdateHyperTreesLeafIdOffsets();

  void DeleteInternalArrays();

//BTX
#ifndef __WRAP__
  void TraverseDualRecursively( vtkHyperTreeGridSuperCursor*, int );

  void TraverseDualMaskedLeaf( vtkHyperTreeGridSuperCursor* );

  void TraverseDualLeaf( vtkHyperTreeGridSuperCursor* );

  void EvaluateDualCorner( vtkHyperTreeSimpleCursor* );
#endif
//ETX

  // Used to advance the super cursor; One Entry per cursor node.
  // Private.
  struct vtkSuperCursorEntry
  {
    // For the new node, start with the node in super cursor as parent.
    unsigned char Parent;
    // Traverse to this child.
    unsigned char Child;
  };

  // Generalizing for 27 tree. Cannot use 3 bits to encode the child to move to.
  // Input: root in supercursor(3x3x3=27), child(3x3x3=27)
  // Output: root, child
  // It is easier to abstract dimensions when we use a single array.
  vtkSuperCursorEntry SuperCursorTraversalTable[27*27];

  // for the GetCell method
  vtkLine* Line;
  vtkPixel* Pixel;
  vtkVoxel* Voxel;

  // I would like to get rid of this.
  // Is it a part of the vtkDataSet API?
  vtkCellLinks* Links;
  void BuildLinks();

//BTX
  vtkIdType RecursiveFindPoint( double x[3],
                                vtkHyperTreeSimpleCursor* cursor,
                                double* origin, double* size);
//ETX

public:
//BTX

  // A simplified hyper tree cursor, to be used by the hyper tree
  // grid supercursor.
  class VTKCOMMONDATAMODEL_EXPORT vtkHyperTreeSimpleCursor
  {
  public:
    vtkHyperTreeSimpleCursor();
    ~vtkHyperTreeSimpleCursor();

    void Clear();
    void Initialize( vtkHyperTreeGrid*, vtkIdType*, vtkIdType, int[3] );
    void ToRoot();
    void ToChild( int );
    bool IsLeaf();
    vtkHyperTree* GetTree() { return this->Tree; }
    vtkIdType GetLeafIndex() { return this->Index; } // Only valid for leaves.

    vtkIdType GetGlobalLeafIndex() { return this->Offset + this->Index; }
    vtkIdType GetOffset() { return this->Offset; }
    unsigned short GetLevel() { return this->Level; }

  private:
    vtkHyperTree* Tree;
    vtkIdType Index;
    vtkIdType Offset;
    unsigned short Level;
    bool Leaf;
  };

  // Public structure filters use to move around the tree.
  // The super cursor keeps neighbor cells so filters can
  // easily access neighbor to leaves.
  // The super cursor is 'const'.  Methods in vtkHyperTreeGrid
  // initialize and compute children for moving toward leaves.
  struct vtkHyperTreeGridSuperCursor
  {
    double Origin[3];
    double Size[3];
    int NumberOfCursors;
    int MiddleCursorId;
    vtkHyperTreeSimpleCursor Cursors[3*3*3];

    vtkHyperTreeSimpleCursor* GetCursor( int idx )
    {
      return this->Cursors + this->MiddleCursorId + idx;
    }
  };
//ETX

private:
  vtkHyperTreeGrid(const vtkHyperTreeGrid&);  // Not implemented.
  void operator=(const vtkHyperTreeGrid&);    // Not implemented.
};

#endif
