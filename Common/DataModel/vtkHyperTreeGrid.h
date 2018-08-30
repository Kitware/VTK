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
/**
 * @class   vtkHyperTreeGrid
 * @brief   A dataset containing a grid of vtkHyperTree instances
 * arranged as a rectilinear grid.
 *
 *
 * A hypertree grid is a dataset containing a rectilinear grid of root nodes,
 * each of which can be refined as a vtkHyperTree grid. Each root node
 * corresponds to a cell of the rectilinear grid. This organization of the
 * root nodes allows for the definition of tree-based AMR grids that do not have
 * uniform geometry.
 * Some filters can be applied on this dataset: contour, outline, geometry.
 *
 * @warning
 * It is not a spatial search object. If you are looking for this kind of
 * octree see vtkCellLocator instead.
 * Extent support is not finished yet.
 *
 * @sa
 * vtkHyperTree vtkRectilinearGrid
 *
 * @par Thanks:
 * This class was written by Philippe Pebay, Joachim Pouderoux, and Charles Law, Kitware 2013
 * This class was modified by Guenole Harel and Jacques-Bernard Lekien 2014
 * This class was rewritten by Philippe Pebay, 2016
 * This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)
*/

#ifndef vtkHyperTreeGrid_h
#define vtkHyperTreeGrid_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataSet.h"

#include <map> // STL header for dual point coordinates adjustment

class vtkHyperTree;
class vtkHyperTreeCursor;
class vtkHyperTreeGridCursor;

class vtkBitArray;
class vtkBoundingBox;
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
  class vtkHyperTreeSimpleCursor;
  class vtkHyperTreePositionCursor;
  class vtkHyperTreeGridIterator;
  struct vtkHyperTreeGridSuperCursor;

  static vtkInformationIntegerKey* LEVELS();
  static vtkInformationIntegerKey* DIMENSION();
  static vtkInformationIntegerKey* ORIENTATION();
  static vtkInformationDoubleVectorKey* SIZES();
  static vtkHyperTreeGrid* New();

  vtkTypeMacro(vtkHyperTreeGrid, vtkDataSet);
  void PrintSelf( ostream&, vtkIndent ) override;

  /**
   * Return what type of dataset this is.
   */
  int GetDataObjectType() override;

  /**
   * Copy the internal geometric and topological structure of a
   * vtkHyperTreeGrid object.
   */
  void CopyStructure( vtkDataSet* ) override;

  //@{
  /**
   * Set/Get the number of local cells in each direction for the underlying rectilinear grid dataset.
   */
  void SetGridSize( unsigned int[3] );
  void SetGridSize( unsigned int, unsigned int, unsigned int );
  vtkGetVector3Macro(GridSize, unsigned int);
  //@}

  //@{
  /**
   * Set/Get extent of the underlying rectilinear grid dataset. This is the local extent
   * and is with respect to the points.
   */
  void SetGridExtent(int extent[6]);
  void SetGridExtent(int, int, int, int, int, int );
  //@}

  //@{
  /**
   * Specify whether indexing mode of grid root cells must be transposed to
   * x-axis first, z-axis last, instead of the default z-axis first, k-axis last
   */
  vtkSetMacro(TransposedRootIndexing, bool);
  vtkGetMacro(TransposedRootIndexing, bool);
  void SetIndexingModeToKJI()
    { this->SetTransposedRootIndexing( false ); }
  void SetIndexingModeToIJK()
    { this->SetTransposedRootIndexing( true ); }
  //@}

  //@{
  /**
   * Set/Get the dimensionality of the grid.
   */
  void SetDimension( unsigned int );
  vtkGetMacro(Dimension, unsigned int);
  //@}

  //@{
  /**
   * Set/Get the orientation of 1D or 2D grids:
   * . in 1D: 0, 1, 2 = aligned along X, Y, Z axis
   * . in 2D: 0, 1, 2 = normal to X, Y, Z axis
   * NB: Not used in 3D
   */
  virtual void SetOrientation(unsigned int);
  vtkGetMacro(Orientation, unsigned int);
  //@}

  //@{
  /**
   * Set/Get the subdivision factor in the grid refinement scheme
   */
  void SetBranchFactor( unsigned int );
  vtkGetMacro(BranchFactor, unsigned int);
  //@}

  /**
   * Return the number of trees in the level 0 grid.
   */
  vtkIdType GetNumberOfTrees();

  /**
   * Get the number of vertices in the primal tree grid.
   */
  vtkIdType GetNumberOfVertices();

  /**
   * Get the number of leaves in the primal tree grid.
   */
  vtkIdType GetNumberOfLeaves();

  /**
   * Return the number of cells in the dual grid.
   */
  vtkIdType GetNumberOfCells() override;

  /**
   * Return the number of points in the dual grid.
   */
  vtkIdType GetNumberOfPoints() override;

  /**
   * Return the number of levels in an individual (primal) tree.
   */
  vtkIdType GetNumberOfLevels( vtkIdType );

  /**
   * Return the number of levels in the hyper tree grid.
   */
  vtkIdType GetNumberOfLevels();

  //@{
  /**
   * Set/Get the grid coordinates in the x-direction.
   */
  void SetXCoordinates( vtkDataArray* );
  vtkGetObjectMacro(XCoordinates, vtkDataArray);
  //@}

  //@{
  /**
   * Set/Get the grid coordinates in the y-direction.
   */
  void SetYCoordinates( vtkDataArray* );
  vtkGetObjectMacro(YCoordinates, vtkDataArray);
  //@}

  //@{
  /**
   * Set/Get the grid coordinates in the z-direction.
   */
  void SetZCoordinates( vtkDataArray* );
  vtkGetObjectMacro(ZCoordinates, vtkDataArray);
  //@}

  //@{
  /**
   * Set/Get the blanking mask of primal leaf cells
   */
  void SetMaterialMask( vtkBitArray* );
  vtkGetObjectMacro(MaterialMask, vtkBitArray);
  //@}

  /**
   * Determine whether blanking mask is empty or not
   */
  bool HasMaterialMask();

  //@{
  /**
   * Set/Get the visibility mask of primal leaf cells
   */
  virtual void SetMaterialMaskIndex( vtkIdTypeArray* );
  vtkGetObjectMacro(MaterialMaskIndex, vtkIdTypeArray);
  //@}

  //@{
  /**
   * Set/Get presence or absence of interface
   */
  vtkSetMacro( HasInterface, bool );
  vtkGetMacro( HasInterface, bool );
  vtkBooleanMacro( HasInterface, bool );
  //@}

  //@{
  /**
   * Set/Get names of interface normal vectors arrays
   */
  vtkSetStringMacro(InterfaceNormalsName);
  vtkGetStringMacro(InterfaceNormalsName);
  //@}

  //@{
  /**
   * Set/Get names of interface intercepts arrays
   */
  vtkSetStringMacro(InterfaceInterceptsName);
  vtkGetStringMacro(InterfaceInterceptsName);
  //@}

  /**
   * This method must be called once the tree settings change.
   */
  virtual void GenerateTrees();

  /**
   * Create a new hyper tree cursor: an object that can traverse
   * the cells of an individual hyper tree at given index.
   * If no hyper tree is present at given location, then one
   * will be created only if 'create' flag is true.
   */
  vtkHyperTreeCursor* NewCursor( vtkIdType, bool create=false );

  /**
   * Create a new hyper tree grid cursor: an object that
   * can traverse the cells of a hyper tree grid, starting at given
   * tree root index.
   * If no hyper tree is present at given location, then one
   * will be created only if 'create' flag is true.
   */
  vtkHyperTreeGridCursor* NewGridCursor( vtkIdType,
                                         bool create=false );

  /**
   * Create a new hyper tree grid geometric cursor: an object that
   * can traverse the cells of a hyper tree grid, starting at given
   * tree root index, managing the geometric properties.
   * If no hyper tree is present at given location, then one
   * will be created only if 'create' flag is true.
   */
  vtkHyperTreeGridCursor* NewGeometricCursor( vtkIdType,
                                              bool create=false );

  /**
   * Create a new hyper tree grid Von Neumann super cursor: an object that
   * can traverse the cells of a hyper tree grid, starting at given
   * tree root index, managing geometric properties and von Neumann
   * neighborhood with basic hyper tree grid cursors.
   * If no hyper tree is present at given location, then one
   * will be created only if 'create' flag is true.
   */
  vtkHyperTreeGridCursor* NewVonNeumannSuperCursor( vtkIdType,
                                                    bool create=false );

  /**
   * Create a new hyper tree grid Moore super cursor: an object that
   * can traverse the cells of a hyper tree grid, starting at given
   * tree root index, managing geometric properties and Moore
   * neighborhood with basic hyper tree grid cursors.
   * If no hyper tree is present at given location, then one
   * will be created only if 'create' flag is true.
   */
  vtkHyperTreeGridCursor* NewMooreSuperCursor( vtkIdType,
                                               bool create=false );

  /**
   * Subdivide node pointed by cursor, only if its a leaf.
   * At the end, cursor points on the node that used to be leaf.
   * \pre leaf_exists: leaf!=0
   * \pre is_a_leaf: leaf->CurrentIsLeaf()
   */
  void SubdivideLeaf( vtkHyperTreeCursor*, vtkIdType );

  /**
   * This method should be avoided in favor of cell/point iterators.
   * Random access to points requires that arrays are created explicitly.
   * Get point coordinates with ptId such that: 0 <= ptId < NumberOfPoints.
   * THIS METHOD IS NOT THREAD SAFE.
   */
  double* GetPoint( vtkIdType ) override;

  /**
   * This method should be avoided in favor of cell/point iterators.
   * Random access to points requires that arrays are created explicitly.
   * Copy point coordinates into user provided array x[3] for specified
   * point id.
   * THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
   * THE DATASET IS NOT MODIFIED
   */
  void GetPoint( vtkIdType, double[3] ) override;

  /**
   * This method should be avoided in favor of cell/point iterators.
   * Random access to cells requires that connectivity arrays are created explicitly.
   * Get cell with cellId such that: 0 <= cellId < NumberOfCells.
   * THIS METHOD IS NOT THREAD SAFE.
   */
  vtkCell* GetCell( vtkIdType ) override;

  /**
   * Overridden so as no not unintentionally hide parent class.
   * See -Woverloaded-virtual
   */
  vtkCell* GetCell( int i, int j, int k) override {
    return this->Superclass::GetCell(i,j,k);
  };

  /**
   * This method should be avoided in favor of cell/point iterators.
   * Random access to cells requires that connectivity arrays are created explicitly.
   * Get cell with cellId such that: 0 <= cellId < NumberOfCells.
   * This is a thread-safe alternative to the previous GetCell()
   * method.
   * THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
   * THE DATASET IS NOT MODIFIED
   */
  void GetCell( vtkIdType, vtkGenericCell* ) override;

  /**
   * All cell types are 2: quadrilaters,3d: hexahedrons.  They may be degenerate though.
   * Get type of cell with cellId such that: 0 <= cellId < NumberOfCells.
   * THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
   * THE DATASET IS NOT MODIFIED
   */
  int GetCellType( vtkIdType ) override;

  /**
   * This method should be avoided in favor of cell/point iterators.
   * Random access to cells requires that connectivity arrays are created explicitly.
   * Topological inquiry to get points defining cell.
   * THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
   * THE DATASET IS NOT MODIFIED
   */
  void GetCellPoints( vtkIdType, vtkIdList* ) override;

  /**
   * Return a pointer to a list of point ids defining cell.
   * NB: More efficient than alternative method.
   */
  void GetCellPoints( vtkIdType, vtkIdType&, vtkIdType*& );

  /**
   * This method should be avoided in favor of cell/point iterators.
   * Random access to cells requires that connectivity arrays are created explicitly.
   * Topological inquiry to get cells using point.
   * THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
   * THE DATASET IS NOT MODIFIED
   */
  void GetPointCells( vtkIdType, vtkIdList* ) override;

  /**
   * This method should be avoided in favor of cell/point iterators.
   * Random access to cells requires that connectivity arrays are created explicitly.
   * Topological inquiry to get all cells using list of points exclusive of
   * cell specified (e.g., cellId). Note that the list consists of only
   * cells that use ALL the points provided.
   * This is exactly the same as GetCellNeighbors in unstructured grid.
   * THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
   * THE DATASET IS NOT MODIFIED
   */
  void GetCellNeighbors( vtkIdType, vtkIdList*, vtkIdList* ) override;

  /**
   * Find cell to which this point belongs, or at least closest one,
   * even if the point is outside the grid.
   * Since dual points are leaves, use the structure of the Tree instead
   * of a point locator.
   */
  vtkIdType FindPoint( double x[3] ) override;

  /**
   * Locate cell based on global coordinate x and tolerance
   * squared. If cell and cellId is non-nullptr, then search starts from
   * this cell and looks at immediate neighbors.  Returns cellId >= 0
   * if inside, < 0 otherwise.  The parametric coordinates are
   * provided in pcoords[3]. The interpolation weights are returned in
   * weights[]. (The number of weights is equal to the number of
   * points in the found cell). Tolerance is used to control how close
   * the point is to be considered "in" the cell.
   * NB: There is actually no need for a starting cell, just use the
   * point, as the tree structure is efficient enough.
   * THIS METHOD IS NOT THREAD SAFE.
   */
  vtkIdType FindCell( double x[3], vtkCell *cell, vtkIdType cellId,
                      double tol2, int& subId, double pcoords[3],
                      double *weights ) override;

  /**
   * This is a version of the above method that can be used with
   * multithreaded applications. A vtkGenericCell must be passed in
   * to be used in internal calls that might be made to GetCell()
   * THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
   * THE DATASET IS NOT MODIFIED
   */
  vtkIdType FindCell( double x[3], vtkCell *cell,
                      vtkGenericCell *gencell, vtkIdType cellId,
                      double tol2, int& subId, double pcoords[3],
                      double *weights ) override;

  /**
   * Restore data object to initial state.
   */
  void Initialize() override;

  /**
   * Return tree located at given index of hyper tree grid
   * NB: This will return nullptr if grid slot is empty.
   */
  vtkHyperTree* GetTree( vtkIdType );

  /**
   * Assign given tree to given index of hyper tree grid
   * NB: This will create a new slot in the grid if needed.
   */
  void SetTree( vtkIdType, vtkHyperTree* );

  /**
   * Initialize an iterator to browse level 0 trees.
   * FIXME: this method is completely unnecessary.
   */
  void InitializeTreeIterator( vtkHyperTreeGridIterator& );

  /**
   * Convenience method to return largest cell size in dataset.
   * Generally used to allocate memory for supporting data structures.
   * This is the number of points of a cell.
   * THIS METHOD IS THREAD SAFE
   */
  int GetMaxCellSize() override;

  /**
   * Create shallow copy of hyper tree grid.
   */
  void ShallowCopy( vtkDataObject* ) override;

  /**
   * Create deep copy of hyper tree grid.
   */
  void DeepCopy( vtkDataObject* ) override;

  /**
   * Structured extent. The extent type is a 3D extent.
   */
  int GetExtentType() override { return VTK_3D_EXTENT; }

  /**
   * Return the actual size of the data in kibibytes (1024 bytes). This number
   * is valid only after the pipeline has updated. The memory size
   * returned is guaranteed to be greater than or equal to the
   * memory required to represent the data (e.g., extra space in
   * arrays, etc. are not included in the return value). THIS METHOD
   * IS THREAD SAFE.
   */
  unsigned long GetActualMemorySize() override;

  //@{
  /**
   * The number of children each node can have.
   */
  vtkGetMacro(NumberOfChildren, unsigned int);
  //@}

  /**
   * Recursively initialize pure material mask
   */
  bool RecursivelyInitializePureMaterialMask( vtkHyperTreeGridCursor* cursor );

  /**
   * Get or create pure material mask
   */
  vtkBitArray* GetPureMaterialMask();

  /**
   * Return hard-coded bitcode correspondng to child mask
   * Dimension 1:
   * Factor 2:
   * 0: 100, 1: 001
   * Factor 3:
   * 0: 100, 1: 010, 2: 001
   * Dimension 2:
   * Factor 2:
   * 0: 1101 0000 0, 1: 0110 0100 0
   * 2: 0001 0011 0, 3: 0000 0101 1
   * Factor 3:
   * 0: 1101 0000 0, 1: 0100 0000 0, 2: 0110 0100 0
   * 3: 0001 0000 0, 4: 0000 1000 0, 5: 0000 0100 0
   * 6: 0001 0011 0, 7: 0000 0001 0, 8: 0000 0101 1
   * Dimension 3:
   * Factor 2:
   * 0: 1101 1000 0110 1000 0000 0000 000, 1: 0110 1100 0011 0010 0000 0000 000
   * 2: 0001 1011 0000 1001 1000 0000 000, 3: 0000 1101 1000 0010 1100 0000 000
   * 4: 0000 0000 0110 1000 0011 0110 000, 5: 0000 0000 0011 0010 0001 1011 000
   * 6: 0000 0000 0000 1001 1000 0110 110, 7: 0000 0000 0000 0010 1100 0011 011
   * Factor 3:
   * 0: 1101 1000 0110 1000 0000 0000 000
   * 1: 0100 1000 0010 0000 0000 0000 000
   * 2: 0110 1100 0011 0010 0000 0000 000
   * 3: 0001 1000 0000 1000 0000 0000 000
   * 4: 0000 1000 0000 0000 0000 0000 000
   * 5: 0000 1100 0000 0010 0000 0000 000
   * 6: 0001 1011 0000 1001 1000 0000 000
   * 7: 0000 1001 0000 0000 1000 0000 000
   * 8: 0000 1101 1000 0010 1100 0000 000
   * 9: 0000 0000 0110 1000 0000 0000 000
   * 10: 0000 0000 0010 0000 0000 0000 000
   * 11: 0000 0000 0011 0010 0000 0000 000
   * 12: 0000 0000 0000 1000 0000 0000 000
   * 13: 0000 0000 0000 0100 0000 0000 000
   * 14: 0000 0000 0000 0010 0000 0000 000
   * 15: 0000 0000 0000 1001 1000 0000 000
   * 16: 0000 0000 0000 0000 1000 0000 000
   * 17: 0000 0000 0000 0010 1100 0000 000
   * 18: 0000 0000 0110 1000 0011 0110 000
   * 19: 0000 0000 0010 0000 0001 0010 000
   * 20: 0000 0000 0011 0010 0001 1011 000
   * 21: 0000 0000 0000 1000 0000 0110 000
   * 22: 0000 0000 0000 0000 0000 0010 000
   * 23: 0000 0000 0000 0010 0000 0011 000
   * 24: 0000 0000 0000 1001 1000 0110 110
   * 25: 0000 0000 0000 0000 1000 0010 010
   * 26: 0000 0000 0000 0010 1100 0011 011
   */
  unsigned int GetChildMask( unsigned int );

  /**
   * Convert the global index of a root to its Cartesian coordinates in the grid.
   */
  void GetLevelZeroCoordinatesFromIndex( vtkIdType,
                                         unsigned int&,
                                         unsigned int&,
                                         unsigned int& );

  /**
   * Convert the Cartesian coordinates of a root in the grid to its global index.
   */
  void GetIndexFromLevelZeroCoordinates( vtkIdType&,
                                         unsigned int,
                                         unsigned int,
                                         unsigned int );

  /**
   * Return the root index of a root cell with given index displaced.
   * by a Cartesian vector in the grid.
   * NB: No boundary checks are performed.
   */
  unsigned int GetShiftedLevelZeroIndex( vtkIdType,
                                         int,
                                         int,
                                         int );

  //@{
  /**
   * A simplified hyper tree cursor, to be used by the hyper tree.
   * grid supercursor.
   */
  class VTKCOMMONDATAMODEL_EXPORT vtkHyperTreeSimpleCursor
  {
  public:
    vtkHyperTreeSimpleCursor();
    ~vtkHyperTreeSimpleCursor();
  //@}

    //@{
    /**
     * Methods that belong to the vtkHyperTreeCursor API.
     */
    vtkHyperTree* GetTree() { return this->Tree; }
    //@}

    /**
     * Only valid for leaves.
     */
    vtkIdType GetLeafIndex() { return this->Index; }

    /**
     * Return level at which cursor is positioned.
     */
    unsigned short GetLevel() { return this->Level; }

  private:
    vtkHyperTree* Tree;
    vtkIdType Index;
    unsigned short Level;
  };

  /**
   * Public structure used by filters to move around the hyper
   * tree grid and easily access neighbors to leaves.
   * The super cursor is 'const'. Methods in vtkHyperTreeGrid
   * initialize and compute children for moving toward leaves.
   */
  struct vtkHyperTreeGridSuperCursor
  {
    double Origin[3];
    double Size[3];
    int NumberOfCursors;
    int MiddleCursorId;
    vtkHyperTreeSimpleCursor Cursors[3*3*3];
    vtkHyperTreeSimpleCursor* GetCursor( int );
  };

  /**
   * An iterator object to iteratively access trees in the grid.
   */
  class VTKCOMMONDATAMODEL_EXPORT vtkHyperTreeGridIterator
  {
  public:
    vtkHyperTreeGridIterator() {}

    /**
     * Initialize the iterator on the tree set of the given grid.
     */
    void Initialize( vtkHyperTreeGrid* );

    /**
     * Get the next tree and set its index then increment the iterator.
     * Returns 0 at the end.
     */
    vtkHyperTree* GetNextTree( vtkIdType& index );

    /**
     * Get the next tree and set its index then increment the iterator.
     * Returns 0 at the end.
     */
    vtkHyperTree* GetNextTree();

  protected:
    std::map<vtkIdType, vtkHyperTree*>::iterator Iterator;
    vtkHyperTreeGrid* Tree;
  };

  //@{
  /**
   * Retrieve an instance of this class from an information object
   */
  static vtkHyperTreeGrid* GetData( vtkInformation* info );
  static vtkHyperTreeGrid* GetData( vtkInformationVector* v, int i=0);
  //@}

protected:
  /**
   * Constructor with default bounds (0,1, 0,1, 0,1).
   */
  vtkHyperTreeGrid();

  /**
   * Destructor
   */
  ~vtkHyperTreeGrid() override;

  void ComputeBounds() override;

  /**
   * Traverse tree with 3x3x3 super cursor. Center cursor generates dual point.
   * Smallest leaf (highest level) owns corners/dual cell.  Ties are given to
   * smallest index (z,y,x order)
   * post: Generate Points and Connectivity.
   */
  void ComputeDualGrid();

  vtkPoints* GetPoints();
  vtkIdTypeArray* GetConnectivity();

  unsigned int BranchFactor; // 2 or 3
  unsigned int Dimension;    // 1, 2, or 3
  unsigned int Orientation;  // 0, 1, or 2
  unsigned int GridSize[3];
  int Extent[6];
  unsigned int NumberOfChildren;
  bool TransposedRootIndexing;

  vtkBitArray* MaterialMask;
  vtkBitArray* PureMaterialMask;
  vtkIdTypeArray* MaterialMaskIndex;
  bool InitPureMaterialMask;

  bool HasInterface;
  char* InterfaceNormalsName;
  char* InterfaceInterceptsName;

  vtkDataArray* XCoordinates;
  vtkDataArray* YCoordinates;
  vtkDataArray* ZCoordinates;

  std::map<vtkIdType, vtkHyperTree*> HyperTrees;

  vtkPoints* Points;
  vtkIdTypeArray* Connectivity;
  std::map<vtkIdType, bool> PointShifted;
  std::map<vtkIdType, double> PointShifts[3];
  std::map<vtkIdType, double> ReductionFactors;

  /**
   * Perform left to right deep copy of hyper tree cursors.
   */
  void DeepCopyCursors( vtkHyperTreeCursor*, vtkHyperTreeCursor* );

  /**
   * Remove existing trees.
   */
  void DeleteTrees();

  /**
   * Reset dual mesh
   */
  void ResetDual();

  /**
   * A convenience method to reset all cursors in a super cursor,
   * either Von Neumann or Moore.
   * This is to be used by Initialize() and ToRoot(), factoring
   * out the commonalities shared by these methods, while allowing for
   * different inheritances.
   */
  void ResetSuperCursor();

  /**
   * Recursively descend into tree down to leaves to generate dual.
   */
  void TraverseDualRecursively( vtkHyperTreeGridCursor* );

  /**
   * Recursively descend into tree down to leaves to generate dual,
   * when a mask array is present.
   */
  void TraverseDualRecursively( vtkHyperTreeGridCursor*, vtkBitArray* );

  /**
   * Process leaf cell and issue corresponding dual corner point in 1D.
   */
  void GenerateDualCornerFromLeaf1D( vtkHyperTreeGridCursor* );

  /**
   * Process leaf cell and issue corresponding dual corner point in 1D,
   * when a mask array is present.
   */
  void GenerateDualCornerFromLeaf1D( vtkHyperTreeGridCursor*, vtkBitArray* );

  /**
   * Process leaf cell and issue corresponding dual corner point in 2D.
   */
  void GenerateDualCornerFromLeaf2D( vtkHyperTreeGridCursor* );

  /**
   * Process leaf cell and issue corresponding dual corner point in 2D,
   * when a mask array is present.
   */
  void GenerateDualCornerFromLeaf2D( vtkHyperTreeGridCursor*, vtkBitArray* );

  /**
   * Process leaf cell and issue corresponding dual corner point in 3D.
   */
  void GenerateDualCornerFromLeaf3D( vtkHyperTreeGridCursor* );

  /**
   * Process leaf cell and issue corresponding dual corner point in 3D,
   * when a mask array is present.
   */
  void GenerateDualCornerFromLeaf3D( vtkHyperTreeGridCursor*, vtkBitArray* );

  /**
   * Compute appropriate shifts for dual corners of masked cells in 2D.
   */
  void ShiftDualCornerFromMaskedLeaf2D( vtkHyperTreeGridCursor*, vtkBitArray* );

  /**
   * Compute appropriate shifts for dual corners of masked cells in 3D.
   */
  void ShiftDualCornerFromMaskedLeaf3D( vtkHyperTreeGridCursor*, vtkBitArray* );

  /**
   * Recursive method called under the hood by FindPoint().
   */
  vtkIdType RecursivelyFindPoint( double x[3],
                                  vtkHyperTreeGridCursor*,
                                  double*,
                                  double* );

#if !defined(__VTK_WRAP__) && !defined(__WRAP_GCCXML__)
  void EvaluateDualCorner( vtkHyperTreeSimpleCursor* );
#endif

  //@{
  /**
   * These are needed by the GetCell() method.
   */
  vtkLine* Line;
  vtkPixel* Pixel;
  vtkVoxel* Voxel;
  //@}

  //@{
  /**
   * Not really needed. Might be removed (is it a part of the vtkDataSet API?).
   */
  vtkCellLinks* Links;
  void BuildLinks();
  //@}

private:
  vtkHyperTreeGrid(const vtkHyperTreeGrid&) = delete;
  void operator=(const vtkHyperTreeGrid&) = delete;

  void GetCellImplementation( vtkIdType, vtkCell* );
};

#endif
