/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkUnstructuredGrid
 * @brief   dataset represents arbitrary combinations of
 * all possible cell types
 *
 * vtkUnstructuredGrid is a data object that is a concrete implementation of
 * vtkDataSet. vtkUnstructuredGrid represents any combinations of any cell
 * types. This includes 0D (e.g., points), 1D (e.g., lines, polylines), 2D
 * (e.g., triangles, polygons), and 3D (e.g., hexahedron, tetrahedron,
 * polyhedron, etc.). vtkUnstructuredGrid provides random access to cells, as
 * well as topological information (such as lists of cells using each point).
 */

#ifndef vtkUnstructuredGrid_h
#define vtkUnstructuredGrid_h

#include "vtkCellArray.h"             //inline GetCellPoints()
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkIdTypeArray.h"           //inline GetCellPoints()
#include "vtkUnstructuredGridBase.h"

#include "vtkSmartPointer.h" // for smart pointer

class vtkCellArray;
class vtkAbstractCellLinks;
class vtkBezierCurve;
class vtkBezierQuadrilateral;
class vtkBezierHexahedron;
class vtkBezierTriangle;
class vtkBezierTetra;
class vtkBezierWedge;
class vtkConvexPointSet;
class vtkEmptyCell;
class vtkHexahedron;
class vtkIdList;
class vtkIdTypeArray;
class vtkLagrangeCurve;
class vtkLagrangeQuadrilateral;
class vtkLagrangeHexahedron;
class vtkLagrangeTriangle;
class vtkLagrangeTetra;
class vtkLagrangeWedge;
class vtkLine;
class vtkPixel;
class vtkPolyLine;
class vtkPolyVertex;
class vtkPolygon;
class vtkPyramid;
class vtkPentagonalPrism;
class vtkHexagonalPrism;
class vtkQuad;
class vtkQuadraticEdge;
class vtkQuadraticHexahedron;
class vtkQuadraticWedge;
class vtkQuadraticPolygon;
class vtkQuadraticPyramid;
class vtkQuadraticQuad;
class vtkQuadraticTetra;
class vtkQuadraticTriangle;
class vtkTetra;
class vtkTriangle;
class vtkTriangleStrip;
class vtkUnsignedCharArray;
class vtkVertex;
class vtkVoxel;
class vtkWedge;
class vtkTriQuadraticHexahedron;
class vtkQuadraticLinearWedge;
class vtkQuadraticLinearQuad;
class vtkBiQuadraticQuad;
class vtkBiQuadraticQuadraticWedge;
class vtkBiQuadraticQuadraticHexahedron;
class vtkBiQuadraticTriangle;
class vtkCubicLine;
class vtkPolyhedron;
class vtkIdTypeArray;

class VTKCOMMONDATAMODEL_EXPORT vtkUnstructuredGrid : public vtkUnstructuredGridBase
{
public:
  /**
   * Standard instantiation method.
   */
  static vtkUnstructuredGrid* New();

  //@{
  /**
   * Standard methods for type information and printing.
   */
  vtkTypeMacro(vtkUnstructuredGrid, vtkUnstructuredGridBase);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  /**
   * Standard vtkDataSet API methods. See vtkDataSet for more information.
   */
  int GetDataObjectType() override { return VTK_UNSTRUCTURED_GRID; }

  /**
   * @brief Pre-allocate memory in internal data structures. Does not change
   * the number of cells, only the array capacities. Existing data is NOT
   * preserved.
   * @param numCells The number of expected cells in the dataset.
   * @param maxCellSize The number of points per cell to allocate memory for.
   * @return True if allocation succeeds.
   * @sa Squeeze();
   */
  bool AllocateEstimate(vtkIdType numCells, vtkIdType maxCellSize)
  {
    return this->AllocateExact(numCells, numCells * maxCellSize);
  }

  /**
   * @brief Pre-allocate memory in internal data structures. Does not change
   * the number of cells, only the array capacities. Existing data is NOT
   * preserved.
   * @param numCells The number of expected cells in the dataset.
   * @param connectivitySize The total number of pointIds stored for all cells.
   * @return True if allocation succeeds.
   * @sa Squeeze();
   */
  bool AllocateExact(vtkIdType numCells, vtkIdType connectivitySize);

  /**
   * Method allocates initial storage for the cell connectivity. Use this
   * method before the method InsertNextCell(). The array capacity is
   * doubled when the inserting a cell exceeds the current capacity.
   * extSize is no longer used.
   *
   * @note Prefer AllocateExact or AllocateEstimate, which give more control
   * over how allocations are distributed.
   */
  void Allocate(vtkIdType numCells = 1000, int vtkNotUsed(extSize) = 1000) override
  {
    this->AllocateExact(numCells, numCells);
  }

  //@{
  /**
   * Standard vtkDataSet methods; see vtkDataSet.h for documentation.
   */
  void Reset();
  void CopyStructure(vtkDataSet* ds) override;
  vtkIdType GetNumberOfCells() override;
  using vtkDataSet::GetCell;
  vtkCell* GetCell(vtkIdType cellId) override;
  void GetCell(vtkIdType cellId, vtkGenericCell* cell) override;
  void GetCellBounds(vtkIdType cellId, double bounds[6]) override;
  void GetCellPoints(vtkIdType cellId, vtkIdList* ptIds) override;
  void GetPointCells(vtkIdType ptId, vtkIdList* cellIds) override;
  vtkCellIterator* NewCellIterator() override;
  //@}

  /**
   * Get the type of the cell with the given cellId.
   */
  int GetCellType(vtkIdType cellId) override;

  /**
   * Get a list of types of cells in a dataset. The list consists of an array
   * of types (not necessarily in any order), with a single entry per type.
   * For example a dataset with 5 triangles, 3 lines, and 100 hexahedra would
   * result in a list of three entries, corresponding to the types VTK_TRIANGLE,
   * VTK_LINE, and VTK_HEXAHEDRON. This override implements an optimization that
   * recomputes cell types only when the types of cells may have changed.
   *
   * THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
   * THE DATASET IS NOT MODIFIED
   */
  void GetCellTypes(vtkCellTypes* types) override;

  /**
   * A higher-performing variant of the virtual vtkDataSet::GetCellPoints()
   * for unstructured grids. Given a cellId, return the number of defining
   * points and the list of points defining the cell.
   *
   * @warning Subsequent calls to this method may invalidate previous call
   * results.
   *
   * The @a pts pointer must not be modified.
   */
  void GetCellPoints(vtkIdType cellId, vtkIdType& npts, vtkIdType const*& pts)
  {
    this->Connectivity->GetCellAtId(cellId, npts, pts);
  }

  //@{
  /**
   * Special (efficient) operation to return the list of cells using the
   * specified point ptId. Use carefully (i.e., make sure that BuildLinks()
   * has been called).
   */
  void GetPointCells(vtkIdType ptId, vtkIdType& ncells, vtkIdType*& cells)
    VTK_SIZEHINT(cells, ncells);
#ifndef VTK_LEGACY_REMOVE
  VTK_LEGACY(void GetPointCells(vtkIdType ptId, unsigned short& ncells, vtkIdType*& cells))
  VTK_SIZEHINT(cells, ncells);
#endif
  //@}

  /**
   * Get the array of all cell types in the grid. Each single-component
   * tuple in the array at an index that corresponds to the type of the cell
   * with the same index. To get an array of only the distinct cell types in
   * the dataset, use GetCellTypes().
   */
  vtkUnsignedCharArray* GetCellTypesArray();

  /**
   * Squeeze all arrays in the grid to conserve memory.
   */
  void Squeeze() override;

  /**
   * Reset the grid to an empty state and free any memory.
   */
  void Initialize() override;

  /**
   * Get the size, in number of points, of the largest cell.
   */
  int GetMaxCellSize() override;

  /**
   * Build topological links from points to lists of cells that use each point.
   * See vtkAbstractCellLinks for more information.
   */
  void BuildLinks();

  /**
   * Get the cell links. The cell links will be one of nullptr=0;
   * vtkCellLinks=1; vtkStaticCellLinksTemplate<VTK_UNSIGNED_SHORT>=2;
   * vtkStaticCellLinksTemplate<VTK_UNSIGNED_INT>=3;
   * vtkStaticCellLinksTemplate<VTK_ID_TYPE>=4.  (See enum types defined in
   * vtkAbstractCellLinks.)
   */
  vtkAbstractCellLinks* GetCellLinks();

  /**
   * Get the face stream of a polyhedron cell in the following format:
   * (numCellFaces, numFace0Pts, id1, id2, id3, numFace1Pts,id1, id2, id3, ...).
   * If the requested cell is not a polyhedron, then the standard GetCellPoints
   * is called to return a list of unique point ids (id1, id2, id3, ...).
   */
  void GetFaceStream(vtkIdType cellId, vtkIdList* ptIds);

  /**
   * Get the number of faces and the face stream of a polyhedral cell.
   * The output \a ptIds has the following format:
   * (numFace0Pts, id1, id2, id3, numFace1Pts,id1, id2, id3, ...).
   * If the requested cell is not a polyhedron, then the standard GetCellPoints
   * is called to return the number of points and a list of unique point ids
   * (id1, id2, id3, ...).
   */
  void GetFaceStream(vtkIdType cellId, vtkIdType& nfaces, vtkIdType const*& ptIds);

  //@{
  /**
   * Provide cell information to define the dataset.
   *
   * Cells like vtkPolyhedron require points plus a list of faces. To handle
   * vtkPolyhedron, SetCells() support a special input cellConnectivities format
   * (numCellFaces, numFace0Pts, id1, id2, id3, numFace1Pts,id1, id2, id3, ...)
   * The functions use vtkPolyhedron::DecomposeAPolyhedronCell() to convert
   * polyhedron cells into standard format.
   */
  void SetCells(int type, vtkCellArray* cells);
  void SetCells(int* types, vtkCellArray* cells);
  void SetCells(vtkUnsignedCharArray* cellTypes, vtkCellArray* cells);
  void SetCells(vtkUnsignedCharArray* cellTypes, vtkCellArray* cells, vtkIdTypeArray* faceLocations,
    vtkIdTypeArray* faces);
  //@}

  /**
   * Return the unstructured grid connectivity array.
   */
  vtkCellArray* GetCells() { return this->Connectivity; }

  /**
   * Topological inquiry to get all cells using list of points exclusive of
   * cell specified (e.g., cellId).
   * THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
   * THE DATASET IS NOT MODIFIED
   */
  void GetCellNeighbors(vtkIdType cellId, vtkIdList* ptIds, vtkIdList* cellIds) override;

  //@{
  /**
   * Use these methods only if the dataset has been specified as
   * Editable. See vtkPointSet for more information.
   */
  vtkIdType InsertNextLinkedCell(int type, int npts, const vtkIdType pts[]) VTK_SIZEHINT(pts, npts);
  void RemoveReferenceToCell(vtkIdType ptId, vtkIdType cellId);
  void AddReferenceToCell(vtkIdType ptId, vtkIdType cellId);
  void ResizeCellList(vtkIdType ptId, int size);
  //@}

  //@{
  /**
   * Set / Get the piece and the number of pieces. Similar to extent in 3D.
   */
  virtual int GetPiece();
  virtual int GetNumberOfPieces();
  //@}

  /**
   * Get the ghost level.
   */
  virtual int GetGhostLevel();

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
   * Shallow and Deep copy.
   */
  void ShallowCopy(vtkDataObject* src) override;
  void DeepCopy(vtkDataObject* src) override;
  //@}

  /**
   * Fill vtkIdTypeArray container with list of cell Ids.  This
   * method traverses all cells and, for a particular cell type,
   * inserts the cell Id into the container.
   */
  void GetIdsOfCellsOfType(int type, vtkIdTypeArray* array) override;

  /**
   * Returns whether cells are all of the same type.
   */
  int IsHomogeneous() override;

  /**
   * This method will remove any cell that is marked as ghost
   * (has the vtkDataSetAttributes::DUPLICATECELL bit set).
   */
  void RemoveGhostCells();

  //@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  static vtkUnstructuredGrid* GetData(vtkInformation* info);
  static vtkUnstructuredGrid* GetData(vtkInformationVector* v, int i = 0);
  //@}

  /**
   * Special support for polyhedron. Return nullptr for all other cell types.
   */
  vtkIdType* GetFaces(vtkIdType cellId);

  //@{
  /**
   * Get pointer to faces and facelocations. Support for polyhedron cells.
   */
  vtkIdTypeArray* GetFaces();
  vtkIdTypeArray* GetFaceLocations();
  //@}

  /**
   * Special function used by vtkUnstructuredGridReader.
   * By default vtkUnstructuredGrid does not contain face information, which is
   * only used by polyhedron cells. If so far no polyhedron cells have been
   * added, Faces and FaceLocations pointers will be nullptr. In this case, need to
   * initialize the arrays and assign values to the previous non-polyhedron cells.
   */
  int InitializeFacesRepresentation(vtkIdType numPrevCells);

  /**
   * Return the mesh (geometry/topology) modification time.
   * This time is different from the usual MTime which also takes into
   * account the modification of data arrays. This function can be used to
   * track the changes on the mesh separately from the data arrays
   * (eg. static mesh over time with transient data).
   */
  virtual vtkMTimeType GetMeshMTime();

  /**
   * A static method for converting a polyhedron vtkCellArray of format
   * [nCellFaces, nFace0Pts, i, j, k, nFace1Pts, i, j, k, ...]
   * into three components: (1) an integer indicating the number of faces
   * (2) a standard vtkCellArray storing point ids [nCell0Pts, i, j, k]
   * and (3) an vtkIdTypeArray storing face connectivity in format
   * [nFace0Pts, i, j, k, nFace1Pts, i, j, k, ...]
   * Note: input is assumed to contain only one polyhedron cell.
   * Outputs (2) and (3) will be stacked at the end of the input
   * cellArray and faces. The original data in the input will not
   * be touched.
   */
  static void DecomposeAPolyhedronCell(vtkCellArray* polyhedronCellArray, vtkIdType& nCellpts,
    vtkIdType& nCellfaces, vtkCellArray* cellArray, vtkIdTypeArray* faces);

  static void DecomposeAPolyhedronCell(const vtkIdType* polyhedronCellStream, vtkIdType& nCellpts,
    vtkIdType& nCellfaces, vtkCellArray* cellArray, vtkIdTypeArray* faces);

  /**
   * A static method for converting an input polyhedron cell stream of format
   * [nFace0Pts, i, j, k, nFace1Pts, i, j, k, ...]
   * into three components: (1) an integer indicating the number of faces
   * (2) a standard vtkCellArray storing point ids [nCell0Pts, i, j, k]
   * and (3) an vtkIdTypeArray storing face connectivity in format
   * [nFace0Pts, i, j, k, nFace1Pts, i, j, k, ...]
   * Note: input is assumed to contain only one polyhedron cell.
   * Outputs (2) and (3) will be stacked at the end of the input
   * cellArray and faces. The original data in the input will not
   * be touched.
   */
  static void DecomposeAPolyhedronCell(vtkIdType nCellFaces, const vtkIdType* inFaceStream,
    vtkIdType& nCellpts, vtkCellArray* cellArray, vtkIdTypeArray* faces);

  /**
   * Convert pid in a face stream into idMap[pid]. The face stream is of format
   * [nCellFaces, nFace0Pts, i, j, k, nFace1Pts, i, j, k, ...]. The user is
   * responsible to make sure all the Ids in faceStream do not exceed the
   * range of idMap.
   */
  static void ConvertFaceStreamPointIds(vtkIdList* faceStream, vtkIdType* idMap);

  /**
   * Convert pid in a face stream into idMap[pid]. The face stream is of format
   * [nFace0Pts, i, j, k, nFace1Pts, i, j, k, ...]. The user is responsible to
   * make sure all the Ids in faceStream do not exceed the range of idMap.
   */
  static void ConvertFaceStreamPointIds(vtkIdType nfaces, vtkIdType* faceStream, vtkIdType* idMap);

  //====================== Begin Legacy Methods ================================

  /**
   * Get the array of all the starting indices of cell definitions
   * in the cell array.
   *
   * @warning vtkCellArray supports random access now. This array is no
   * longer used.
   */
  vtkIdTypeArray* GetCellLocationsArray();

  //@{
  /**
   * Special methods specific to vtkUnstructuredGrid for defining the cells
   * composing the dataset. Most cells require just arrays of cellTypes,
   * cellLocations and cellConnectivities which implicitly define the set of
   * points in each cell and their ordering. In those cases the
   * cellConnectivities are of the format
   * (numFace0Pts, id1, id2, id3, numFace1Pts, id1, id2, id3...). However, some
   * cells like vtkPolyhedron require points plus a list of faces. To handle
   * vtkPolyhedron, SetCells() support a special input cellConnectivities format
   * (numCellFaces, numFace0Pts, id1, id2, id3, numFace1Pts,id1, id2, id3, ...)
   * The functions use vtkPolyhedron::DecomposeAPolyhedronCell() to convert
   * polyhedron cells into standard format.
   *
   * @warning The cellLocations array is no longer used; this information
   * is stored in vtkCellArray. Use the other SetCells overloads.
   */
  void SetCells(
    vtkUnsignedCharArray* cellTypes, vtkIdTypeArray* cellLocations, vtkCellArray* cells);
  void SetCells(vtkUnsignedCharArray* cellTypes, vtkIdTypeArray* cellLocations, vtkCellArray* cells,
    vtkIdTypeArray* faceLocations, vtkIdTypeArray* faces);
  //@}

  //====================== End Legacy Methods ==================================

protected:
  vtkUnstructuredGrid();
  ~vtkUnstructuredGrid() override;

  // These are all the cells that vtkUnstructuredGrid can represent. Used by
  // GetCell() (and similar) methods.
  vtkVertex* Vertex;
  vtkPolyVertex* PolyVertex;
  vtkBezierCurve* BezierCurve;
  vtkBezierQuadrilateral* BezierQuadrilateral;
  vtkBezierHexahedron* BezierHexahedron;
  vtkBezierTriangle* BezierTriangle;
  vtkBezierTetra* BezierTetra;
  vtkBezierWedge* BezierWedge;
  vtkLagrangeCurve* LagrangeCurve;
  vtkLagrangeQuadrilateral* LagrangeQuadrilateral;
  vtkLagrangeHexahedron* LagrangeHexahedron;
  vtkLagrangeTriangle* LagrangeTriangle;
  vtkLagrangeTetra* LagrangeTetra;
  vtkLagrangeWedge* LagrangeWedge;
  vtkLine* Line;
  vtkPolyLine* PolyLine;
  vtkTriangle* Triangle;
  vtkTriangleStrip* TriangleStrip;
  vtkPixel* Pixel;
  vtkQuad* Quad;
  vtkPolygon* Polygon;
  vtkTetra* Tetra;
  vtkVoxel* Voxel;
  vtkHexahedron* Hexahedron;
  vtkWedge* Wedge;
  vtkPyramid* Pyramid;
  vtkPentagonalPrism* PentagonalPrism;
  vtkHexagonalPrism* HexagonalPrism;
  vtkQuadraticEdge* QuadraticEdge;
  vtkQuadraticTriangle* QuadraticTriangle;
  vtkQuadraticQuad* QuadraticQuad;
  vtkQuadraticPolygon* QuadraticPolygon;
  vtkQuadraticTetra* QuadraticTetra;
  vtkQuadraticHexahedron* QuadraticHexahedron;
  vtkQuadraticWedge* QuadraticWedge;
  vtkQuadraticPyramid* QuadraticPyramid;
  vtkQuadraticLinearQuad* QuadraticLinearQuad;
  vtkBiQuadraticQuad* BiQuadraticQuad;
  vtkTriQuadraticHexahedron* TriQuadraticHexahedron;
  vtkQuadraticLinearWedge* QuadraticLinearWedge;
  vtkBiQuadraticQuadraticWedge* BiQuadraticQuadraticWedge;
  vtkBiQuadraticQuadraticHexahedron* BiQuadraticQuadraticHexahedron;
  vtkBiQuadraticTriangle* BiQuadraticTriangle;
  vtkCubicLine* CubicLine;
  vtkConvexPointSet* ConvexPointSet;
  vtkPolyhedron* Polyhedron;
  vtkEmptyCell* EmptyCell;

  // Points derived from vtkPointSet.
  // Attribute data (i.e., point and cell data (i.e., scalars, vectors, normals, tcoords)
  // derived from vtkDataSet.

  // The heart of the data represention. The points are managed by the
  // superclass vtkPointSet. A cell is defined by its connectivity (i.e., the
  // point ids that define the cell) and the cell type, represented by the
  // Connectivity and Types arrays.
  // Finally, when certain topological information is needed (e.g.,
  // all the cells that use a point), the cell links array is built.
  vtkSmartPointer<vtkCellArray> Connectivity;
  vtkSmartPointer<vtkAbstractCellLinks> Links;
  vtkSmartPointer<vtkUnsignedCharArray> Types;

  // Set of all cell types present in the grid. All entries are unique.
  vtkSmartPointer<vtkCellTypes> DistinctCellTypes;

  // The DistinctCellTypes is cached, so we keep track of the last time it was
  // updated so we can compare it to the modified time of the Types array.
  vtkMTimeType DistinctCellTypesUpdateMTime;

  // Special support for polyhedra/cells with explicit face representations.
  // The Faces class represents polygonal faces using a modified vtkCellArray
  // structure. Each cell face list begins with the total number of faces in
  // the cell, followed by a vtkCellArray data organization
  // (n,i,j,k,n,i,j,k,...).
  vtkSmartPointer<vtkIdTypeArray> Faces;
  vtkSmartPointer<vtkIdTypeArray> FaceLocations;

  // Legacy support -- stores the old-style cell array locations.
  vtkSmartPointer<vtkIdTypeArray> CellLocations;

  vtkIdType InternalInsertNextCell(int type, vtkIdType npts, const vtkIdType ptIds[]) override;
  vtkIdType InternalInsertNextCell(int type, vtkIdList* ptIds) override;
  vtkIdType InternalInsertNextCell(int type, vtkIdType npts, const vtkIdType ptIds[],
    vtkIdType nfaces, const vtkIdType faces[]) override;
  void InternalReplaceCell(vtkIdType cellId, int npts, const vtkIdType pts[]) override;

private:
  // Hide these from the user and the compiler.
  vtkUnstructuredGrid(const vtkUnstructuredGrid&) = delete;
  void operator=(const vtkUnstructuredGrid&) = delete;

  void Cleanup();
};

#endif
