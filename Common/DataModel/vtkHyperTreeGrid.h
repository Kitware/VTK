// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHyperTreeGrid
 * @brief   A dataset containing a grid of vtkHyperTree instances
 * arranged as a rectilinear grid.
 *
 *
 * An hypertree grid is a dataset containing a rectilinear grid of root nodes,
 * each of which can be refined as a vtkHyperTree grid. This organization of the
 * root nodes allows for the definition of tree-based AMR grids that do not have
 * uniform geometry.
 * Some filters can be applied on this dataset: contour, outline, geometry.
 *
 * The order and number of points must match that specified by the dimensions
 * of the grid. The point order increases in i fastest (from 0<=i<dims[0]),
 * then j (0<=j<dims[1]), then k (0<=k<dims[2]) where dims[] are the
 * dimensions of the grid in the i-j-k topological directions. The number of
 * points is dims[0]*dims[1]*dims[2]. The same is true for the cells of the
 * grid. The order and number of cells must match that specified by the
 * dimensions of the grid. The cell order increases in i fastest (from
 * 0<=i<(dims[0]-1)), then j (0<=j<(dims[1]-1)), then k (0<=k<(dims[2]-1))
 * The number of cells is (dims[0]-1)*(dims[1]-1)*(dims[2]-1).
 *
 * Dimensions : number of points by direction of rectilinear grid
 * CellDims : number of cells by directions of rectilinear grid
 * (1 for each dimensions 1)
 *
 * Interface : plane that cuts a HTG cell.
 * It is defined (for each cell) by a normal and the distance between the origin and the plane along
 * that normal (i.e. the orthogonal distance). The name of the arrays containing each information is
 * specified in `InterfaceInterceptsName` and `InterfaceNormalsName` The normals array is a 3D array
 * that contains the 3D normal for each cell's interface (for lower dimensions, some values are
 * ignored). The intercepts (or distances) array is also a 3D array containing:
 *  - the distance to the first plane (if exists, otherwise ignored)
 *  - the distance to the second plane (if exists, otherwise ignored)
 *  - the type of cell (mixed/pure, cf. vtkHyperTreeGridGeometryImpl.h:CellInterfaceType)
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
 * This class was modified by Jacques-Bernard Lekien 2018
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGrid_h
#define vtkHyperTreeGrid_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataObject.h"

#include "vtkNew.h"          // vtkSmartPointer
#include "vtkSmartPointer.h" // vtkSmartPointer

#include <cassert> // std::assert
#include <map>     // std::map
#include <memory>  // std::shared_ptr

VTK_ABI_NAMESPACE_BEGIN
class vtkBitArray;
class vtkBoundingBox;
class vtkCellLinks;
class vtkCollection;
class vtkDataArray;
class vtkHyperTree;
class vtkHyperTreeGridOrientedCursor;
class vtkHyperTreeGridOrientedGeometryCursor;
class vtkHyperTreeGridNonOrientedCursor;
class vtkHyperTreeGridNonOrientedGeometryCursor;
class vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor;
class vtkHyperTreeGridNonOrientedVonNeumannSuperCursor;
class vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight;
class vtkHyperTreeGridNonOrientedMooreSuperCursor;
class vtkHyperTreeGridNonOrientedMooreSuperCursorLight;
class vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor;
class vtkDoubleArray;
class vtkDataSetAttributes;
class vtkIdTypeArray;
class vtkLine;
class vtkPixel;
class vtkPoints;
class vtkCellData;
class vtkUnsignedCharArray;

class VTKCOMMONDATAMODEL_EXPORT vtkHyperTreeGrid : public vtkDataObject
{
public:
  static vtkInformationIntegerKey* LEVELS();
  static vtkInformationIntegerKey* DIMENSION();
  static vtkInformationIntegerKey* ORIENTATION();
  static vtkInformationDoubleVectorKey* SIZES();
  static vtkHyperTreeGrid* New();

  vtkTypeMacro(vtkHyperTreeGrid, vtkDataObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Invalid index that is returned for undefined nodes, for example for nodes that are out of
   * bounds (they can exist with the super cursors).
   */
  static constexpr vtkIdType InvalidIndex = ~0;

  /**
   * Set/Get mode squeeze
   */
  vtkSetStringMacro(ModeSqueeze); // By copy
  vtkGetStringMacro(ModeSqueeze);

  /**
   * Squeeze this representation.
   */
  virtual void Squeeze();

  /**
   * Return what type of dataset this is.
   */
  int GetDataObjectType() VTK_FUTURE_CONST override { return VTK_HYPER_TREE_GRID; }

  /**
   * Copy the internal geometric and topological structure of a
   * vtkHyperTreeGrid object.
   */
  virtual void CopyStructure(vtkDataObject*);

  /**
   * Copy the internal structure with no data associated.
   */
  virtual void CopyEmptyStructure(vtkDataObject*);

  // --------------------------------------------------------------------------
  // RectilinearGrid common API
  // --------------------------------------------------------------------------

  ///@{
  /**
   * Set/Get sizes of this rectilinear grid dataset
   */
  void SetDimensions(const unsigned int dims[3]);
  void SetDimensions(const int dims[3]);
  void SetDimensions(unsigned int i, unsigned int j, unsigned int k);
  void SetDimensions(int i, int j, int k);
  ///@}

  ///@{
  /**
   * Get dimensions of this rectilinear grid dataset.
   * The dimensions correspond to the number of points
   */
  const unsigned int* GetDimensions() const VTK_SIZEHINT(3);
  void GetDimensions(int dim[3]) const;
  void GetDimensions(unsigned int dim[3]) const;
  ///@}

  ///@{
  /**
   * Different ways to set the extent of the data array.  The extent
   * should be set before the "Scalars" are set or allocated.
   * The Extent is stored in the order (X, Y, Z).
   * Set/Get extent of this rectilinear grid dataset.
   */
  void SetExtent(const int extent[6]);
  void SetExtent(int x1, int x2, int y1, int y2, int z1, int z2);
  vtkGetVector6Macro(Extent, int);
  ///@}

  ///@{
  /**
   * Get grid sizes of this structured cells dataset.
   * Values are deduced from the Dimensions/Extent
   * Dimensions default to 1 if not specified.
   */
  const unsigned int* GetCellDims() const VTK_SIZEHINT(3);
  void GetCellDims(int cellDims[3]) const;
  void GetCellDims(unsigned int cellDims[3]) const;
  ///@}

  // --------------------------------------------------------------------------

  ///@{
  /**
   * Get the dimensionality of the grid from the Dimensions/Extent.
   */
  unsigned int GetDimension() const { return this->Dimension; }
  ///@}

  ///@{
  /**
   * Return the index of the valid dimension.
   */
  void Get1DAxis(unsigned int& axis) const
  {
    assert("pre: valid_dim" && this->GetDimension() == 1);
    axis = this->Axis[0];
  }
  ///@}

  ///@{
  /**
   * Return the indices of the two valid dimensions.
   */
  void Get2DAxes(unsigned int& axis1, unsigned int& axis2) const
  {
    assert("pre: valid_dim" && this->GetDimension() == 2);
    axis1 = this->Axis[0];
    axis2 = this->Axis[1];
  }
  ///@}

  ///@{
  /**
   * Get the axis information (used for CopyStructure)
   */
  const unsigned int* GetAxes() const { return this->Axis; }
  ///@}

  ///@{
  /**
   * The number of children each node can have.
   */
  // vtkGetMacro(NumberOfChildren, unsigned int); not const
  unsigned int GetNumberOfChildren() const { return this->NumberOfChildren; }
  ///@}

  ///@{
  /**
   * Specify whether indexing mode of grid root cells must be transposed to
   * x-axis first, z-axis last, instead of the default z-axis first, k-axis last
   */
  vtkSetMacro(TransposedRootIndexing, bool);
  vtkGetMacro(TransposedRootIndexing, bool);
  void SetIndexingModeToKJI() { this->SetTransposedRootIndexing(false); }
  void SetIndexingModeToIJK() { this->SetTransposedRootIndexing(true); }
  ///@}

  ///@{
  /**
   * Get the orientation of 1D or 2D grids:
   * - in 1D: 0, 1, 2 = aligned along X, Y, Z axis
   * - in 2D: 0, 1, 2 = normal to X, Y, Z axis
   * NB: Not used in 3D
   */
  unsigned int GetOrientation() const { return this->Orientation; }
  ///@}

  ///@{
  /**
   * Get the state of frozen
   */
  vtkGetMacro(FreezeState, bool);
  ///@}

  ///@{
  /**
   * Set/Get the subdivision factor in the grid refinement scheme
   */
  void SetBranchFactor(unsigned int);
  unsigned int GetBranchFactor() const { return this->BranchFactor; }
  ///@}

  /**
   * Return the maximum number of trees in the level 0 grid.
   */
  vtkIdType GetMaxNumberOfTrees() const;

  /**
   * Get the number of non empty trees in this grid.
   */
  vtkIdType GetNumberOfNonEmptyTrees();

  /**
   * Get the number of leaves in the primal tree grid.
   */
  vtkIdType GetNumberOfLeaves();

  /**
   * Return the number of levels in an individual (primal) tree.
   */
  unsigned int GetNumberOfLevels(vtkIdType);

  /**
   * Return the number of levels in the hyper tree grid.
   */
  unsigned int GetNumberOfLevels();

  ///@{
  /**
   * Set/Get the grid coordinates in the x-direction.
   */
  virtual void SetXCoordinates(vtkDataArray*);
  vtkGetObjectMacro(XCoordinates, vtkDataArray);
  ///@}

  ///@{
  /**
   * Set/Get the grid coordinates in the y-direction.
   */
  virtual void SetYCoordinates(vtkDataArray*);
  vtkGetObjectMacro(YCoordinates, vtkDataArray);
  ///@}

  ///@{
  /**
   * Set/Get the grid coordinates in the z-direction.
   */
  virtual void SetZCoordinates(vtkDataArray*);
  vtkGetObjectMacro(ZCoordinates, vtkDataArray);
  ///@}

  ///@{
  /**
   * Utility methods to set coordinates.
   */
  virtual void CopyCoordinates(const vtkHyperTreeGrid* output);
  virtual void SetFixedCoordinates(unsigned int axis, double value);
  ///@}

  ///@{
  /**
   * Set/Get the blanking mask of primal leaf cells
   */
  void SetMask(vtkBitArray*);
  vtkGetObjectMacro(Mask, vtkBitArray);
  ///@}

  /**
   * Determine whether blanking mask is empty or not
   */
  bool HasMask();

  ///@{
  /**
   * Set/Get presence or absence of interface
   */
  vtkSetMacro(HasInterface, bool);
  vtkGetMacro(HasInterface, bool);
  vtkBooleanMacro(HasInterface, bool);
  ///@}

  ///@{
  /**
   * Set/Get names of interface normal vectors arrays
   */
  vtkSetStringMacro(InterfaceNormalsName);
  vtkGetStringMacro(InterfaceNormalsName);
  ///@}

  ///@{
  /**
   * Set/Get names of interface intercepts arrays
   */
  vtkSetStringMacro(InterfaceInterceptsName);
  vtkGetStringMacro(InterfaceInterceptsName);
  ///@}

  ///@{
  /**
   * Set/Get depth limiter value
   */
  vtkSetMacro(DepthLimiter, unsigned int);
  vtkGetMacro(DepthLimiter, unsigned int);
  ///@}

  ///@{
  /**
   * Used to initialize a cursor of the given type.
   *
   * cursor: the cursor to initialize
   *
   * index: the index of the tree to use in the HTG
   *
   * create: allow to construct the hyper tree if the slot is empty
   */
  void InitializeOrientedCursor(
    vtkHyperTreeGridOrientedCursor* cursor, vtkIdType index, bool create = false);
  VTK_NEWINSTANCE
  vtkHyperTreeGridOrientedCursor* NewOrientedCursor(vtkIdType index, bool create = false);

  void InitializeOrientedGeometryCursor(
    vtkHyperTreeGridOrientedGeometryCursor* cursor, vtkIdType index, bool create = false);
  VTK_NEWINSTANCE
  vtkHyperTreeGridOrientedGeometryCursor* NewOrientedGeometryCursor(
    vtkIdType index, bool create = false);

  void InitializeNonOrientedCursor(
    vtkHyperTreeGridNonOrientedCursor* cursor, vtkIdType index, bool create = false);
  VTK_NEWINSTANCE
  vtkHyperTreeGridNonOrientedCursor* NewNonOrientedCursor(vtkIdType index, bool create = false);

  void InitializeNonOrientedGeometryCursor(
    vtkHyperTreeGridNonOrientedGeometryCursor* cursor, vtkIdType index, bool create = false);
  VTK_NEWINSTANCE
  vtkHyperTreeGridNonOrientedGeometryCursor* NewNonOrientedGeometryCursor(
    vtkIdType index, bool create = false);

  void InitializeNonOrientedUnlimitedGeometryCursor(
    vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor* cursor, vtkIdType index,
    bool create = false);
  VTK_NEWINSTANCE
  vtkHyperTreeGridNonOrientedUnlimitedGeometryCursor* NewNonOrientedUnlimitedGeometryCursor(
    vtkIdType index, bool create = false);
  ///@}

  /**
   * Return a geometric cursor pointing to one of the nodes at position `x`
   */
  vtkHyperTreeGridNonOrientedGeometryCursor* FindNonOrientedGeometryCursor(double x[3]);

private:
  unsigned int RecurseDichotomic(
    double value, vtkDoubleArray* coord, double tol, unsigned int ideb, unsigned int ifin) const;

  unsigned int FindDichotomic(double value, vtkDataArray* coord, double tol) const;

public:
  virtual unsigned int FindDichotomicX(double value, double tol = 0.0) const;
  virtual unsigned int FindDichotomicY(double value, double tol = 0.0) const;
  virtual unsigned int FindDichotomicZ(double value, double tol = 0.0) const;

  ///@{
  /**
   * Used to initialize a cursor of the given type.
   *
   * cursor: the cursor to initialize
   *
   * index: the index of the tree to use in the HTG
   *
   * create: allow to construct the hyper tree if the slot is empty
   */
  void InitializeNonOrientedVonNeumannSuperCursor(
    vtkHyperTreeGridNonOrientedVonNeumannSuperCursor* cursor, vtkIdType index, bool create = false);
  VTK_NEWINSTANCE
  vtkHyperTreeGridNonOrientedVonNeumannSuperCursor* NewNonOrientedVonNeumannSuperCursor(
    vtkIdType index, bool create = false);

  void InitializeNonOrientedVonNeumannSuperCursorLight(
    vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight* cursor, vtkIdType index,
    bool create = false);
  VTK_NEWINSTANCE
  vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight* NewNonOrientedVonNeumannSuperCursorLight(
    vtkIdType index, bool create = false);

  void InitializeNonOrientedMooreSuperCursor(
    vtkHyperTreeGridNonOrientedMooreSuperCursor* cursor, vtkIdType index, bool create = false);
  VTK_NEWINSTANCE
  vtkHyperTreeGridNonOrientedMooreSuperCursor* NewNonOrientedMooreSuperCursor(
    vtkIdType index, bool create = false);

  void InitializeNonOrientedMooreSuperCursorLight(
    vtkHyperTreeGridNonOrientedMooreSuperCursorLight* cursor, vtkIdType index, bool create = false);
  VTK_NEWINSTANCE
  vtkHyperTreeGridNonOrientedMooreSuperCursorLight* NewNonOrientedMooreSuperCursorLight(
    vtkIdType index, bool create = false);

  void InitializeNonOrientedUnlimitedMooreSuperCursor(
    vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor* cursor, vtkIdType index,
    bool create = false);
  VTK_NEWINSTANCE
  vtkHyperTreeGridNonOrientedUnlimitedMooreSuperCursor* NewNonOrientedUnlimitedMooreSuperCursor(
    vtkIdType index, bool create = false);
  ///@}

  /**
   * Restore data object to initial state.
   */
  void Initialize() override;

  /**
   * Return tree located at given index of hyper tree grid
   * NB: This will construct a new HyperTree if grid slot is empty.
   */
  virtual vtkHyperTree* GetTree(vtkIdType, bool create = false);

  /**
   * Assign given tree to given index of hyper tree grid
   * NB: This will create a new slot in the grid if needed.
   */
  void SetTree(vtkIdType, vtkHyperTree*);

  /**
   * Remove the tree at the given index.
   * Return the number of trees removed (0 or 1).
   */
  size_t RemoveTree(vtkIdType index);

  /**
   * Create shallow copy of hyper tree grid.
   */
  void ShallowCopy(vtkDataObject*) override;

  /**
   * Create deep copy of hyper tree grid.
   */
  void DeepCopy(vtkDataObject*) override;

  /**
   * Structured extent. The extent type is a 3D extent.
   */
  int GetExtentType() VTK_FUTURE_CONST override { return VTK_3D_EXTENT; }

  /**
   * Return the actual size of the data in bytes. This number
   * is valid only after the pipeline has updated. The memory size
   * returned is guaranteed to be greater than or equal to the
   * memory required to represent the data (e.g., extra space in
   * arrays, etc. are not included in the return value). THIS METHOD
   * IS THREAD SAFE.
   */
  virtual unsigned long GetActualMemorySizeBytes();

  /**
   * Return the actual size of the data in kibibytes (1024 bytes). This number
   * is valid only after the pipeline has updated. The memory size
   * returned is guaranteed to be greater than or equal to the
   * memory required to represent the data (e.g., extra space in
   * arrays, etc. are not included in the return value). THIS METHOD
   * IS THREAD SAFE.
   */
  unsigned long GetActualMemorySize() override;

  /**
   * Returns true if type is CELL, false otherwise
   */
  bool SupportsGhostArray(int type) override;

private:
  /**
   * Recursively initialize pure material mask
   */
  bool RecursivelyInitializePureMask(vtkHyperTreeGridNonOrientedCursor*, vtkDataArray*);

  /**
   * Clean pure material mask
   *
   * Filters modifying the mask will call SetMask which will call CleanPureMask
   * in order to allow an update during the next GetPureMask
   */
  void CleanPureMask();

public:
  /**
   * Get or create pure material mask
   *
   * PureMask is a boolean array size to the number of cells which describes,
   * for each cell, if it is pure material mask (PMM), a mask which is true if
   * the cell is not pure.
   * The PMM of a cell is true:
   * - if the cell is hidden; we do not take into account if the cell is leaf or coarse;
   * - if the fine/leaf cell is mixed (HasInterface is true, InterfaceInterceptsName and
   *   InterfaceNormalsName are the vector value field names with 3 components);
   *   the description of its type at the interface (the third component of the field
   *   named InterfaceInterceptsName) is < 2
   *   (2 indicates that this cell contains only one material, cell is pure);
   * - if the coarse cell has at least one of its child cells which has set PMM to true.
   *
   * The PureMask array is deleted during a call to the SetMask method (which itself
   * calls the CleanPureMask method).
   * It will be (re)built during the first call to this GetPureMask method.
   * A second call to this same method will be free because this array is stored
   * permanently in memory, as long as the CleanPureMask method is not called.
   */
  vtkBitArray* GetPureMask();

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
  unsigned int GetChildMask(unsigned int);

  /**
   * Convert the Cartesian coordinates of a root in the grid  to its global index.
   */
  void GetIndexFromLevelZeroCoordinates(vtkIdType&, unsigned int, unsigned int, unsigned int) const;

  /**
   * Return the root index of a root cell with given index displaced.
   * by a cartesian vector in the grid (di,dj,dk).
   *
   * However, in HTG 2D, this method used Orientation information.
   * According to the orientation values (0/1/2), the association of
   * the topological axes changes with the real axes (YZ/XZ/XY).
   *
   * NB: No boundary checks are performed.
   */
  vtkIdType GetShiftedLevelZeroIndex(vtkIdType, int, int, int) const;

  /**
   * Convert the global index of a root to its Cartesian coordinates in the grid.
   */
  void GetLevelZeroCoordinatesFromIndex(
    vtkIdType, unsigned int&, unsigned int&, unsigned int&) const;

  /**
   * Convert the global index of a root to its Spatial coordinates origin and size.
   */
  virtual void GetLevelZeroOriginAndSizeFromIndex(vtkIdType, double*, double*);

  /**
   * Convert the global index of a root to its Spatial coordinates origin and size.
   */
  virtual void GetLevelZeroOriginFromIndex(vtkIdType, double*);

  /**
   * Return the maximum global index value.
   * Can be useful to allocate new cell arrays.
   */
  vtkIdType GetGlobalNodeIndexMax();

  /**
   * Initialize local indexes for every individual Hyper Tree after they have been refined.
   */
  void InitializeLocalIndexNode();

  /**
   * Returns true if a ghost cell array is defined.
   */
  bool HasAnyGhostCells() const;

  /**
   * Gets the array that defines the ghost type of each cell.
   * see also GetTreeGhostArray().
   */
  vtkUnsignedCharArray* GetGhostCells();

  /**
   * Gets the array that defines the ghost type of each cell.
   * Unlike GetGhostCells(), we cache the pointer to the array
   * to save a lookup involving string comparisons
   */
  vtkUnsignedCharArray* GetTreeGhostArray();

  /**
   * Allocate ghost array for points.
   */
  vtkUnsignedCharArray* AllocateTreeGhostArray();

  /**
   * An iterator object to iteratively access trees in the grid.
   */
  class VTKCOMMONDATAMODEL_EXPORT vtkHyperTreeGridIterator
  {
  public:
    vtkHyperTreeGridIterator() = default;

    /**
     * Initialize the iterator on the tree set of the given grid.
     */
    void Initialize(vtkHyperTreeGrid*);

    /**
     * Get the next tree and set its index then increment the iterator.
     * Returns 0 at the end.
     */
    vtkHyperTree* GetNextTree(vtkIdType& index);

    /**
     * Get the next tree and set its index then increment the iterator.
     * Returns 0 at the end.
     */
    vtkHyperTree* GetNextTree();

  protected:
    std::map<vtkIdType, vtkSmartPointer<vtkHyperTree>>::iterator Iterator;
    vtkHyperTreeGrid* Grid;
  };

  /**
   * Initialize an iterator to browse level 0 trees.
   * FIXME: this method is completely unnecessary.
   */
  void InitializeTreeIterator(vtkHyperTreeGridIterator&);

  ///@{
  /**
   * Retrieve an instance of this class from an information object
   */
  static vtkHyperTreeGrid* GetData(vtkInformation* info);
  static vtkHyperTreeGrid* GetData(vtkInformationVector* v, int i = 0);
  ///@}

  /**
   * Compute the hyper tree grid bounding box ignoring masked cells.
   * THIS METHOD IS NOT THREAD SAFE.
   */
  virtual void ComputeBounds();

  ///@{
  /**
   * Return a pointer to the geometry bounding box in the form
   * (xmin,xmax, ymin,ymax, zmin,zmax).
   *
   * This method was incorrectly providing grid bounds before vtk 9.4,
   * grid bounds are available in GetGridBounds() if needed.
   * THIS METHOD IS NOT THREAD SAFE.
   */
  virtual double* GetBounds() VTK_SIZEHINT(6);
  void GetBounds(double bounds[6]);
  ///@}

  /**
   * Return a pointer to the grid bounding box in the form
   * (xmin,xmax, ymin,ymax, zmin,zmax).
   * THIS METHOD IS NOT THREAD SAFE.
   */
  virtual void GetGridBounds(double bounds[6]);

  /**
   * Get the center of the bounding box.
   * THIS METHOD IS NOT THREAD SAFE.
   */
  double* GetCenter() VTK_SIZEHINT(3);

  /**
   * Get the center of the bounding box.
   * THIS METHOD IS NOT THREAD SAFE.
   */
  void GetCenter(double center[3]);

  /**
   * Return a pointer to this dataset's hypertree node data.
   * THIS METHOD IS THREAD SAFE
   */
  vtkCellData* GetCellData();

  /**
   * Returns the hypertree node field data stored as cell data.
   * If type != vtkDataObject::AttributeTypes::CELL,
   * it defers to vtkDataObject;
   */
  vtkFieldData* GetAttributesAsFieldData(int type) override;

  /**
   * Returns the number of nodes.
   * Ii type == vtkDataObject::AttributeTypes::CELL,
   * it defers to vtkDataObject.
   */
  vtkIdType GetNumberOfElements(int type) override;

  /**
   * Return the number of cells.
   * It matches the total number of internal nodes and leaves of the underlying hypertrees.
   */
  vtkIdType GetNumberOfCells();

protected:
  /**
   * Constructor with default bounds (0,1, 0,1, 0,1).
   */
  vtkHyperTreeGrid();

  /**
   * Destructor
   */
  ~vtkHyperTreeGrid() override;

  /**
   * ModeSqueeze
   */
  char* ModeSqueeze;

  double Bounds[6]; // (xmin,xmax, ymin,ymax, zmin,zmax) geometric bounds
  double Center[3]; // geometric center

  bool FreezeState;
  unsigned int BranchFactor; // 2 or 3
  unsigned int Dimension;    // 1, 2, or 3

  ///@{
  /**
   * These arrays pointers are caches used to avoid a string comparison (when
   * getting ghost arrays using GetArray(name))
   */
  vtkUnsignedCharArray* TreeGhostArray;
  bool TreeGhostArrayCached;
  ///@}
private:
  unsigned int Orientation; // 0, 1, or 2
  unsigned int Axis[2];

  vtkTimeStamp ComputeTime;

protected:
  unsigned int NumberOfChildren;
  bool TransposedRootIndexing;

  // --------------------------------
  // RectilinearGrid common fields
  // --------------------------------
private:
  unsigned int Dimensions[3]; // Just for GetDimensions
  unsigned int CellDims[3];   // Just for GetCellDims
protected:
  int DataDescription;
  int Extent[6];

  bool WithCoordinates;
  vtkDataArray* XCoordinates;
  vtkDataArray* YCoordinates;
  vtkDataArray* ZCoordinates;
  // --------------------------------

  vtkBitArray* Mask;
  vtkBitArray* PureMask;

  bool HasInterface;
  char* InterfaceNormalsName;
  char* InterfaceInterceptsName;

  std::map<vtkIdType, vtkSmartPointer<vtkHyperTree>> HyperTrees;

  vtkNew<vtkCellData> CellData; // Scalars, vectors, etc. associated w/ each point

  unsigned int DepthLimiter;

private:
  vtkHyperTreeGrid(const vtkHyperTreeGrid&) = delete;
  void operator=(const vtkHyperTreeGrid&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
