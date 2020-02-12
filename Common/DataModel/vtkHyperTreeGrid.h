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
 * An hypertree grid is a dataset containing a rectilinear grid of root nodes,
 * each of which can be refined as a vtkHyperTree grid. This organization of the
 * root nodes allows for the definition of tree-based AMR grids that do not have
 * uniform geometry.
 * Some filters can be applied on this dataset: contour, outline, geometry.
 *
 * JB A valider la suite
 * The order and number of points must match that specified by the dimensions
 * of the grid. The point order increases in i fastest (from 0<=i<dims[0]),
 * then j (0<=j<dims[1]), then k (0<=k<dims[2]) where dims[] are the
 * dimensions of the grid in the i-j-k topological directions. The number of
 * points is dims[0]*dims[1]*dims[2]. The same is true for the cells of the
 * grid. The order and number of cells must match that specified by the
 * dimensions of the grid. The cell order increases in i fastest (from
 * 0<=i<(dims[0]-1)), then j (0<=j<(dims[1]-1)), then k (0<=k<(dims[2]-1))
 * The number of cells is (dims[0]-1)*(dims[1]-1)*(dims[2]-1).
 * JB
 * Dimensions : number of points by direction of rectilinear grid
 * CellDims : number of cells by directions of rectilinear grid
 * (1 for each dimensions 1)
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
// #include "vtkPointData.h" // vtkPointData

#include <cassert> // std::assert
#include <map>     // std::map
#include <memory>  // std::shared_ptr

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
class vtkHyperTreeGridNonOrientedVonNeumannSuperCursor;
class vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight;
class vtkHyperTreeGridNonOrientedMooreSuperCursor;
class vtkHyperTreeGridNonOrientedMooreSuperCursorLight;
class vtkDoubleArray;
class vtkDataSetAttributes;
class vtkIdTypeArray;
class vtkLine;
class vtkPixel;
class vtkPoints;
class vtkPointData;
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
  int GetDataObjectType() override { return VTK_HYPER_TREE_GRID; }

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

  //@{
  /**
   * Set/Get sizes of this rectilinear grid dataset
   */
  void SetDimensions(const unsigned int dims[3]);
  void SetDimensions(const int dims[3]);
  void SetDimensions(unsigned int i, unsigned int j, unsigned int k);
  void SetDimensions(int i, int j, int k);
  //@}

  //@{
  /**
   * Get dimensions of this rectilinear grid dataset.
   * The dimensions correspond to the number of points
   */
  const unsigned int* GetDimensions() const VTK_SIZEHINT(3);
  // JB Dommage, car vtkGetVectorMacro(Dimensions,int,3); not const function
  void GetDimensions(int dim[3]) const;
  void GetDimensions(unsigned int dim[3]) const;
  //@}

  //@{
  /**
   * Different ways to set the extent of the data array.  The extent
   * should be set before the "Scalars" are set or allocated.
   * The Extent is stored in the order (X, Y, Z).
   * Set/Get extent of this rectilinear grid dataset.
   */
  void SetExtent(const int extent[6]);
  void SetExtent(int x1, int x2, int y1, int y2, int z1, int z2);
  vtkGetVector6Macro(Extent, int);
  //@}

  //@{
  /**
   * JB Get grid sizes of this structured cells dataset.
   * Valeurs deduites a partir de Dimensions/Extent
   * Les dimensions non exprimees auront pour valeur 1.
   */
  const unsigned int* GetCellDims() const VTK_SIZEHINT(3);
  void GetCellDims(int cellDims[3]) const;
  void GetCellDims(unsigned int cellDims[3]) const;
  //@}

  // --------------------------------------------------------------------------

  //@{
  /**
   * JB Get the dimensionality of the grid deduite a partir
   * de Dimensions/Extent.
   */
  unsigned int GetDimension() const { return this->Dimension; }
  //@}

  //@{
  /**
   * JB retourne l'indice de la dimension valide.
   */
  void Get1DAxis(unsigned int& axis) const
  {
    assert("pre: valid_dim" && this->GetDimension() == 1);
    axis = this->Axis[0];
  }
  //@}

  //@{
  /**
   * JB Retourne l'indice des deux dimensions valides.
   */
  void Get2DAxes(unsigned int& axis1, unsigned int& axis2) const
  {
    assert("pre: valid_dim" && this->GetDimension() == 2);
    axis1 = this->Axis[0];
    axis2 = this->Axis[1];
  }
  //@}

  //@{
  /**
   * JB Get the axis information (used for CopyStructure)
   */
  const unsigned int* GetAxes() const { return this->Axis; }
  //@}

  //@{
  /**
   * The number of children each node can have.
   */
  // vtkGetMacro(NumberOfChildren, unsigned int); not const
  unsigned int GetNumberOfChildren() const { return this->NumberOfChildren; }
  //@}

  /**
   * Get the number or trees available along the 3 axis.
   * For 2D or 1D the empty dimension will be equal to 1.
   * The empty dimension being any axis that contain a
   * single value for their point coordinate.
   *
   * SetDimensions() must be called in order to have a valid
   * NumberOfTreesPerDimension[3].
   */
  // JB ?? virtual void GetNumberOfTreesPerDimension(unsigned int dimsOut[3]);

  //@{
  /**
   * Specify whether indexing mode of grid root cells must be transposed to
   * x-axis first, z-axis last, instead of the default z-axis first, k-axis last
   */
  vtkSetMacro(TransposedRootIndexing, bool);
  vtkGetMacro(TransposedRootIndexing, bool);
  void SetIndexingModeToKJI() { this->SetTransposedRootIndexing(false); }
  void SetIndexingModeToIJK() { this->SetTransposedRootIndexing(true); }
  //@}

  //@{
  /**
   * Get the orientation of 1D or 2D grids:
   * . in 1D: 0, 1, 2 = aligned along X, Y, Z axis
   * . in 2D: 0, 1, 2 = normal to X, Y, Z axis
   * NB: Not used in 3D
   */
  unsigned int GetOrientation() const { return this->Orientation; }
  //@}

  //@{
  /**
   * Get the state of frozen
   */
  vtkGetMacro(FreezeState, bool);
  //@}

  //@{
  /**
   * Set/Get the subdivision factor in the grid refinement scheme
   */
  void SetBranchFactor(unsigned int);
  unsigned int GetBranchFactor() const { return this->BranchFactor; }
  //@}

  /**
   * Return the maximum number of trees in the level 0 grid.
   */
  vtkIdType GetMaxNumberOfTrees();

  /**
   * Get the number of vertices in the primal tree grid.
   */
  vtkIdType GetNumberOfVertices();

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

  //@{
  /**
   * Set/Get the grid coordinates in the x-direction.
   */
  virtual void SetXCoordinates(vtkDataArray*);
  vtkGetObjectMacro(XCoordinates, vtkDataArray);
  //@}

  //@{
  /**
   * Set/Get the grid coordinates in the y-direction.
   */
  virtual void SetYCoordinates(vtkDataArray*);
  vtkGetObjectMacro(YCoordinates, vtkDataArray);
  //@}

  //@{
  /**
   * Set/Get the grid coordinates in the z-direction.
   */
  virtual void SetZCoordinates(vtkDataArray*);
  vtkGetObjectMacro(ZCoordinates, vtkDataArray);
  //@}

  //@{
  /**
   * JB Augented services on Coordinates.
   */
  virtual void CopyCoordinates(const vtkHyperTreeGrid* output);
  virtual void SetFixedCoordinates(unsigned int axis, double value);
  //@}

  //@{
  /**
   * Set/Get the blanking mask of primal leaf cells
   */
  void SetMask(vtkBitArray*);
  vtkGetObjectMacro(Mask, vtkBitArray);
  //@}

  /**
   * Determine whether blanking mask is empty or not
   */
  bool HasMask();

  //@{
  /**
   * Set/Get presence or absence of interface
   */
  vtkSetMacro(HasInterface, bool);
  vtkGetMacro(HasInterface, bool);
  vtkBooleanMacro(HasInterface, bool);
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

  //@{
  /**
   * Set/Get depth limiter value
   */
  vtkSetMacro(DepthLimiter, unsigned int);
  vtkGetMacro(DepthLimiter, unsigned int);
  //@}

  /**
   * JB
   */
  void InitializeOrientedCursor(
    vtkHyperTreeGridOrientedCursor* cursor, vtkIdType index, bool create = false);
  vtkHyperTreeGridOrientedCursor* NewOrientedCursor(vtkIdType index, bool create = false);

  /**
   * JB
   */
  void InitializeOrientedGeometryCursor(
    vtkHyperTreeGridOrientedGeometryCursor* cursor, vtkIdType index, bool create = false);
  vtkHyperTreeGridOrientedGeometryCursor* NewOrientedGeometryCursor(
    vtkIdType index, bool create = false);

  /**
   * JB
   */
  void InitializeNonOrientedCursor(
    vtkHyperTreeGridNonOrientedCursor* cursor, vtkIdType index, bool create = false);
  vtkHyperTreeGridNonOrientedCursor* NewNonOrientedCursor(vtkIdType index, bool create = false);

  /**
   * JB
   */
  void InitializeNonOrientedGeometryCursor(
    vtkHyperTreeGridNonOrientedGeometryCursor* cursor, vtkIdType index, bool create = false);
  vtkHyperTreeGridNonOrientedGeometryCursor* NewNonOrientedGeometryCursor(
    vtkIdType index, bool create = false);

  /**
   * JB Retourne un curseur geometrique pointant une des mailles comportant la position spatiale x
   */
  vtkHyperTreeGridNonOrientedGeometryCursor* FindNonOrientedGeometryCursor(double x[3]);

private:
  unsigned int RecurseDichotomic(
    double value, vtkDoubleArray* coord, unsigned int ideb, unsigned int ifin) const;

  unsigned int FindDichotomic(double value, vtkDataArray* coord) const;

public:
  virtual unsigned int FindDichotomicX(double value) const;
  virtual unsigned int FindDichotomicY(double value) const;
  virtual unsigned int FindDichotomicZ(double value) const;

  /**
   * JB
   */
  void InitializeNonOrientedVonNeumannSuperCursor(
    vtkHyperTreeGridNonOrientedVonNeumannSuperCursor* cursor, vtkIdType index, bool create = false);
  vtkHyperTreeGridNonOrientedVonNeumannSuperCursor* NewNonOrientedVonNeumannSuperCursor(
    vtkIdType index, bool create = false);

  /**
   * JB
   */
  void InitializeNonOrientedVonNeumannSuperCursorLight(
    vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight* cursor, vtkIdType index,
    bool create = false);
  vtkHyperTreeGridNonOrientedVonNeumannSuperCursorLight* NewNonOrientedVonNeumannSuperCursorLight(
    vtkIdType index, bool create = false);

  /**
   * JB
   */
  void InitializeNonOrientedMooreSuperCursor(
    vtkHyperTreeGridNonOrientedMooreSuperCursor* cursor, vtkIdType index, bool create = false);
  vtkHyperTreeGridNonOrientedMooreSuperCursor* NewNonOrientedMooreSuperCursor(
    vtkIdType index, bool create = false);

  /**
   * JB
   */
  void InitializeNonOrientedMooreSuperCursorLight(
    vtkHyperTreeGridNonOrientedMooreSuperCursorLight* cursor, vtkIdType index, bool create = false);
  vtkHyperTreeGridNonOrientedMooreSuperCursorLight* NewNonOrientedMooreSuperCursorLight(
    vtkIdType index, bool create = false);

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
  int GetExtentType() override { return VTK_3D_EXTENT; }

  /**
   * Return the actual size of the data in kibibytes (1024 bytes). This number
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
   * Recursively initialize pure material mask
   */
  bool RecursivelyInitializePureMask(
    vtkHyperTreeGridNonOrientedCursor* cursor, vtkDataArray* normale);

  /**
   * Get or create pure material mask
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
   * by a Cartesian vector in the grid.
   * NB: No boundary checks are performed.
   */
  vtkIdType GetShiftedLevelZeroIndex(vtkIdType, unsigned int, unsigned int, unsigned int) const;

  /**
   * Convert the global index of a root to its Cartesian coordinates in the grid.
   */
  void GetLevelZeroCoordinatesFromIndex(
    vtkIdType, unsigned int&, unsigned int&, unsigned int&) const;

  /**
   * Convert the global index of a root to its Spacial coordinates origin and size.
   */
  virtual void GetLevelZeroOriginAndSizeFromIndex(vtkIdType, double*, double*);

  /**
   * JB Convert the global index of a root to its Spacial coordinates origin and size.
   */
  virtual void GetLevelZeroOriginFromIndex(vtkIdType, double*);

  /**
   * JB Retourne la valeur maximale du global index.
   * Cette information est indispensable pour construire une nouvelle
   * grandeur puisqu'elle devra au moins etre de cette taille.
   * Pour les memes raisons, dans le cas de la construction du maillage dual,
   * afin de reutiliser les grandeurs de l'HTG, le nombre de sommets
   * sera dimensionne a cette valeur.
   */
  vtkIdType GetGlobalNodeIndexMax();

  /**
   * JB Permet d'initialiser les index locaux de chacun des HT de cet HTG
   * une fois que TOUS les HTs aient ete COMPLETEMENT construits/raffines !
   * A l'utilisateur ensuite de fournir les grandeurs suivant cet ordre.
   */
  void InitializeLocalIndexNode();

  /**
   * Returns 1 if there are any ghost cells
   * 0 otherwise.
   */
  bool HasAnyGhostCells() const;

  /**
   * Accessor on ghost cells
   */
  vtkUnsignedCharArray* GetGhostCells();

  /**
   * Gets the array that defines the ghost type of each point.
   * We cache the pointer to the array to save a lookup involving string comparisons
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
    vtkHyperTreeGridIterator() {}

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
    std::map<vtkIdType, vtkSmartPointer<vtkHyperTree> >::iterator Iterator;
    vtkHyperTreeGrid* Grid;
  };

  /**
   * Initialize an iterator to browse level 0 trees.
   * FIXME: this method is completely unnecessary.
   */
  void InitializeTreeIterator(vtkHyperTreeGridIterator&);

  //@{
  /**
   * Retrieve an instance of this class from an information object
   */
  static vtkHyperTreeGrid* GetData(vtkInformation* info);
  static vtkHyperTreeGrid* GetData(vtkInformationVector* v, int i = 0);
  //@}

  /**
   * Return a pointer to the geometry bounding box in the form
   * (xmin,xmax, ymin,ymax, zmin,zmax).
   * THIS METHOD IS NOT THREAD SAFE.
   */
  virtual double* GetBounds() VTK_SIZEHINT(6);

  /**
   * Return a pointer to the geometry bounding box in the form
   * (xmin,xmax, ymin,ymax, zmin,zmax).
   * THIS METHOD IS NOT THREAD SAFE.
   */
  void GetBounds(double bounds[6]);

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

  //@{
  /**
   * Return a pointer to this dataset's point/tree data.
   * THIS METHOD IS THREAD SAFE
   */
  vtkPointData* GetPointData();
  //@}

protected:
  /**
   * Constructor with default bounds (0,1, 0,1, 0,1).
   */
  vtkHyperTreeGrid();

  /**
   * Destructor
   */
  virtual ~vtkHyperTreeGrid() override;

  /**
   * JB ModeSqueeze
   */
  char* ModeSqueeze;

  double Bounds[6]; // (xmin,xmax, ymin,ymax, zmin,zmax) geometric bounds
  double Center[3]; // geometric center

  bool FreezeState;
  unsigned int BranchFactor; // 2 or 3
  unsigned int Dimension;    // 1, 2, or 3

  //@{
  /**
   * These arrays pointers are caches used to avoid a string comparison (when
   * getting ghost arrays using GetArray(name))
   */
  vtkUnsignedCharArray* TreeGhostArray;
  bool TreeGhostArrayCached;
  //@}
private:
  unsigned int Orientation; // 0, 1, or 2
  unsigned int Axis[2];

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
  bool InitPureMask;

  bool HasInterface;
  char* InterfaceNormalsName;
  char* InterfaceInterceptsName;

  std::map<vtkIdType, vtkSmartPointer<vtkHyperTree> > HyperTrees;

  vtkNew<vtkPointData> PointData; // Scalars, vectors, etc. associated w/ each point

  unsigned int DepthLimiter;

private:
  vtkHyperTreeGrid(const vtkHyperTreeGrid&) = delete;
  void operator=(const vtkHyperTreeGrid&) = delete;
};

#endif
