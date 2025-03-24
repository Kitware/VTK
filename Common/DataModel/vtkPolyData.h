// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPolyData
 * @brief   concrete dataset represents vertices, lines, polygons, and triangle strips
 *
 * vtkPolyData is a data object that is a concrete implementation of
 * vtkDataSet. vtkPolyData represents a geometric structure consisting of
 * vertices, lines, polygons, and/or triangle strips. Point and cell
 * attribute values (e.g., scalars, vectors, etc.) also are represented.
 *
 * The actual cell types (vtkCellType.h) supported by vtkPolyData are:
 * vtkVertex, vtkPolyVertex, vtkLine, vtkPolyLine, vtkTriangle, vtkQuad,
 * vtkPolygon, and vtkTriangleStrip.
 *
 * One important feature of vtkPolyData objects is that special traversal and
 * data manipulation methods are available to process data. These methods are
 * generally more efficient than vtkDataSet methods and should be used
 * whenever possible. For example, traversing the cells in a dataset we would
 * use GetCell(). To traverse cells with vtkPolyData we would retrieve the
 * cell array object representing polygons (for example using GetPolys()) and
 * then use vtkCellArray's InitTraversal() and GetNextCell() methods.
 *
 * @warning
 * Because vtkPolyData is implemented with four separate instances of
 * vtkCellArray to represent 0D vertices, 1D lines, 2D polygons, and 2D
 * triangle strips, it is possible to create vtkPolyData instances that
 * consist of a mixture of cell types. Because of the design of the class,
 * there are certain limitations on how mixed cell types are inserted into
 * the vtkPolyData, and in turn the order in which they are processed and
 * rendered. To preserve the consistency of cell ids, and to ensure that
 * cells with cell data are rendered properly, users must insert mixed cells
 * in the order of vertices (vtkVertex and vtkPolyVertex), lines (vtkLine and
 * vtkPolyLine), polygons (vtkTriangle, vtkQuad, vtkPolygon), and triangle
 * strips (vtkTriangleStrip).
 *
 * @warning
 * Some filters when processing vtkPolyData with mixed cell types may process
 * the cells in differing ways. Some will convert one type into another
 * (e.g., vtkTriangleStrip into vtkTriangles) or expect a certain type
 * (vtkDecimatePro expects triangles or triangle strips; vtkTubeFilter
 * expects lines). Read the documentation for each filter carefully to
 * understand how each part of vtkPolyData is processed.
 *
 * @warning
 * Some of the methods specified here function properly only when the dataset
 * has been specified as "Editable". They are documented as such.
 */

#ifndef vtkPolyData_h
#define vtkPolyData_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkPointSet.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

#include "vtkCellArray.h"         // Needed for inline methods
#include "vtkCellLinks.h"         // Needed for inline methods
#include "vtkPolyDataInternals.h" // Needed for inline methods

VTK_ABI_NAMESPACE_BEGIN
struct vtkPolyDataDummyContainter;
class vtkIncrementalPointLocator;

class VTKCOMMONDATAMODEL_EXPORT VTK_MARSHALAUTO vtkPolyData : public vtkPointSet
{
public:
  static vtkPolyData* New();
  static vtkPolyData* ExtendedNew();

  vtkTypeMacro(vtkPolyData, vtkPointSet);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return what type of dataset this is.
   */
  int GetDataObjectType() VTK_FUTURE_CONST override { return VTK_POLY_DATA; }

  /**
   * Copy the geometric and topological structure of an input poly data object.
   */
  void CopyStructure(vtkDataSet* ds) override;

  ///@{
  /**
   * Standard vtkDataSet interface.
   */
  vtkIdType GetNumberOfCells() override;
  using vtkDataSet::GetCell;
  vtkCell* GetCell(vtkIdType cellId) override;
  void GetCell(vtkIdType cellId, vtkGenericCell* cell) override;
  int GetCellType(vtkIdType cellId) override;
  vtkIdType GetCellSize(vtkIdType cellId) override;
  void GetCellBounds(vtkIdType cellId, double bounds[6]) override;
  void GetCellNeighbors(vtkIdType cellId, vtkIdList* ptIds, vtkIdList* cellIds) override;
  ///@}

  /**
   * Copy cells listed in idList from pd, including points, point data,
   * and cell data.  This method assumes that point and cell data have
   * been allocated.  If you pass in a point locator, then the points
   * won't be duplicated in the output. This requires the use of an
   * incremental point locator.
   */
  void CopyCells(vtkPolyData* pd, vtkIdList* idList, vtkIncrementalPointLocator* locator = nullptr);

  /**
   * Copy a cells point ids into list provided. (Less efficient.)
   */
  void GetCellPoints(vtkIdType cellId, vtkIdList* ptIds) override;

  /**
   * Efficient method to obtain cells using a particular point. Make sure that
   * routine BuildLinks() has been called.
   */
  void GetPointCells(vtkIdType ptId, vtkIdList* cellIds) override;

  /**
   * Compute the (X, Y, Z)  bounds of the data. Note that the method only considers
   * points that are used by cells.
   * This is done for usability and historical reasons.
   *
   * IMPORTANT
   *
   * Until vtk 9.0.1, vtkPolyData::ComputeBounds() used to ignore points that do not belong
   * to any cell.
   * That was not consistent with other vtkPointSet subclasses and thus was error prone.
   * See this ParaView issue https://gitlab.kitware.com/paraview/paraview/-/issues/20354
   * Now it defers to vtkPointSet::ComputeBounds() so vtkPolyData::GetBounds() may
   * not return the same bounds as before. This behavior is probably the one you want
   * when using bounds.
   *
   * The previous behavior is still available through vtkPolyData::ComputeCellsBounds()
   * and vtkPolyData::GetCellsBounds(). This is mainly used for rendering purpose.
   */
  void ComputeCellsBounds();

  /**
   * Get the cells bounds.
   * Internally calls ComputeCellsBounds().
   * @sa ComputeCellsBounds()
   */
  void GetCellsBounds(double bounds[6]);

  /**
   * Recover extra allocated memory when creating data whose initial size
   * is unknown. Examples include using the InsertNextCell() method, or
   * when using the CellArray::EstimateSize() method to create vertices,
   * lines, polygons, or triangle strips.
   */
  void Squeeze() override;

  /**
   * Return the maximum cell size in this poly data.
   */
  int GetMaxCellSize() override;

  ///@{
  /**
   * Get the maximum/minimum spatial dimensionality of the data
   * which is the maximum/minimum  dimension of all cells.
   */
  int GetMaxSpatialDimension() override;
  int GetMinSpatialDimension() override;
  ///@}

  /**
   * Maps the cell at position `cellId` inside the `vtkPolyData` to its location in the
   * corresponding cell array. For instance, if cell `cellId` is a line, then this method returns
   * the position of this cell in the `Lines` cell array.
   */
  vtkIdType GetCellIdRelativeToCellArray(vtkIdType cellId);

  /**
   * Set the cell array defining vertices.
   */
  void SetVerts(vtkCellArray* v);

  /**
   * Get the cell array defining vertices. If there are no vertices, an
   * empty array will be returned (convenience to simplify traversal).
   */
  vtkCellArray* GetVerts();

  /**
   * Set the cell array defining lines.
   */
  void SetLines(vtkCellArray* l);

  /**
   * Get the cell array defining lines. If there are no lines, an
   * empty array will be returned (convenience to simplify traversal).
   */
  vtkCellArray* GetLines();

  /**
   * Set the cell array defining polygons.
   */
  void SetPolys(vtkCellArray* p);

  /**
   * Get the cell array defining polygons. If there are no polygons, an
   * empty array will be returned (convenience to simplify traversal).
   */
  vtkCellArray* GetPolys();

  /**
   * Set the cell array defining triangle strips.
   */
  void SetStrips(vtkCellArray* s);

  /**
   * Get the cell array defining triangle strips. If there are no
   * triangle strips, an empty array will be returned (convenience to
   * simplify traversal).
   */
  vtkCellArray* GetStrips();

  ///@{
  /**
   * Return the number of primitives of a particular type held.
   */
  vtkIdType GetNumberOfVerts() { return (this->Verts ? this->Verts->GetNumberOfCells() : 0); }
  vtkIdType GetNumberOfLines() { return (this->Lines ? this->Lines->GetNumberOfCells() : 0); }
  vtkIdType GetNumberOfPolys() { return (this->Polys ? this->Polys->GetNumberOfCells() : 0); }
  vtkIdType GetNumberOfStrips() { return (this->Strips ? this->Strips->GetNumberOfCells() : 0); }
  ///@}

  /**
   * Preallocate memory for the internal cell arrays. Each of the internal
   * cell arrays (verts, lines, polys, and strips) will be resized to hold
   * @a numCells cells of size @a maxCellSize.
   *
   * Existing data is not preserved and the number of cells is set to zero.
   *
   * @return True if allocation succeeds.
   */
  bool AllocateEstimate(vtkIdType numCells, vtkIdType maxCellSize);

  /**
   * Preallocate memory for the internal cell arrays. Each of the internal
   * cell arrays (verts, lines, polys, and strips) will be resized to hold
   * the indicated number of cells of the specified cell size.
   *
   * Existing data is not preserved and the number of cells is set to zero.
   *
   * @return True if allocation succeeds.
   */
  bool AllocateEstimate(vtkIdType numVerts, vtkIdType maxVertSize, vtkIdType numLines,
    vtkIdType maxLineSize, vtkIdType numPolys, vtkIdType maxPolySize, vtkIdType numStrips,
    vtkIdType maxStripSize);

  /**
   * Preallocate memory for the internal cell arrays. Each of the internal
   * cell arrays (verts, lines, polys, and strips) will be resized to hold
   * @a numCells cells and @a connectivitySize pointIds.
   *
   * Existing data is not preserved and the number of cells is set to zero.
   *
   * @return True if allocation succeeds.
   */
  bool AllocateExact(vtkIdType numCells, vtkIdType connectivitySize);

  /**
   * Preallocate memory for the internal cell arrays. Each of the internal
   * cell arrays (verts, lines, polys, and strips) will be resized to hold
   * the indicated number of cells and the specified number of point ids
   * (ConnSize).
   *
   * Existing data is not preserved and the number of cells is set to zero.
   *
   * @return True if allocation succeeds.
   */
  bool AllocateExact(vtkIdType numVerts, vtkIdType vertConnSize, vtkIdType numLines,
    vtkIdType lineConnSize, vtkIdType numPolys, vtkIdType polyConnSize, vtkIdType numStrips,
    vtkIdType stripConnSize);

  /**
   * Preallocate memory for the internal cell arrays such that they are the
   * same size as those in @a pd.
   *
   * Existing data is not preserved and the number of cells is set to zero.
   *
   * @return True if allocation succeeds.
   */
  bool AllocateCopy(vtkPolyData* pd);

  /**
   * Preallocate memory for the internal cell arrays such that they are
   * proportional to those in @a pd by a factor of @a ratio (for instance,
   * @a ratio = 2 allocates twice as many cells).
   *
   * Existing data is not preserved and the number of cells is set to zero.
   *
   * @return True if allocation succeeds.
   */
  bool AllocateProportional(vtkPolyData* pd, double ratio);

  /**
   * Method allocates initial storage for vertex, line, polygon, and
   * triangle strip arrays. Use this method before the method
   * PolyData::InsertNextCell(). (Or, provide vertex, line, polygon, and
   * triangle strip cell arrays). @a extSize is no longer used.
   */
  void Allocate(vtkIdType numCells = 1000, int vtkNotUsed(extSize) = 1000)
  {
    this->AllocateExact(numCells, numCells);
  }

  /**
   * Similar to the method above, this method allocates initial storage for
   * vertex, line, polygon, and triangle strip arrays. It does this more
   * intelligently, examining the supplied inPolyData to determine whether to
   * allocate the verts, lines, polys, and strips arrays.  (These arrays are
   * allocated only if there is data in the corresponding arrays in the
   * inPolyData.)  Caution: if the inPolyData has no verts, and after
   * allocating with this method an PolyData::InsertNextCell() is invoked
   * where a vertex is inserted, bad things will happen.
   */
  void Allocate(vtkPolyData* inPolyData, vtkIdType numCells = 1000, int vtkNotUsed(extSize) = 1000)
  {
    this->AllocateProportional(
      inPolyData, static_cast<double>(numCells) / inPolyData->GetNumberOfCells());
  }

  /**
   * Insert a cell of type VTK_VERTEX, VTK_POLY_VERTEX, VTK_LINE, VTK_POLY_LINE,
   * VTK_TRIANGLE, VTK_QUAD, VTK_POLYGON, or VTK_TRIANGLE_STRIP.  Make sure that
   * the PolyData::Allocate() function has been called first or that vertex,
   * line, polygon, and triangle strip arrays have been supplied.
   * Note: will also insert VTK_PIXEL, but converts it to VTK_QUAD.
   */
  vtkIdType InsertNextCell(int type, int npts, const vtkIdType pts[]) VTK_SIZEHINT(pts, npts);

  /**
   * Insert a cell of type VTK_VERTEX, VTK_POLY_VERTEX, VTK_LINE, VTK_POLY_LINE,
   * VTK_TRIANGLE, VTK_QUAD, VTK_POLYGON, or VTK_TRIANGLE_STRIP.  Make sure that
   * the PolyData::Allocate() function has been called first or that vertex,
   * line, polygon, and triangle strip arrays have been supplied.
   * Note: will also insert VTK_PIXEL, but converts it to VTK_QUAD.
   */
  vtkIdType InsertNextCell(int type, vtkIdList* pts);

  /**
   * Begin inserting data all over again. Memory is not freed but otherwise
   * objects are returned to their initial state.
   */
  void Reset();

  /**
   * Create data structure that allows random access of cells. BuildCells is
   * expensive but necessary to make use of the faster non-virtual implementations
   * of GetCell/GetCellPoints. One may check if cells need to be built via
   * NeedToBuilds before invoking. Cells always need to be built/re-built after
   * low level direct modifications to verts, lines, polys or strips cell arrays.
   */
  void BuildCells();

  /**
   * Check if BuildCells is needed.
   */
  bool NeedToBuildCells() { return this->Cells == nullptr; }

  /**
   * Create upward links from points to cells that use each point. Enables
   * topologically complex queries. Normally the links array is allocated
   * based on the number of points in the vtkPolyData. The optional
   * initialSize parameter can be used to allocate a larger size initially.
   */
  void BuildLinks(int initialSize = 0);

  ///@{
  /**
   * Set/Get the links that were created possibly without using BuildLinks.
   */
  vtkSetSmartPointerMacro(Links, vtkAbstractCellLinks);
  vtkGetSmartPointerMacro(Links, vtkAbstractCellLinks);
  ///@}

  /**
   * Release data structure that allows random access of the cells. This must
   * be done before a 2nd call to BuildLinks(). DeleteCells implicitly deletes
   * the links as well since they are no longer valid.
   */
  void DeleteCells();

  /**
   * Release the upward links from point to cells that use each point.
   */
  void DeleteLinks();

  ///@{
  /**
   * Special (efficient) operations on poly data. Use carefully (i.e., make
   * sure that BuildLinks() has been called).
   */
  void GetPointCells(vtkIdType ptId, vtkIdType& ncells, vtkIdType*& cells)
    VTK_SIZEHINT(cells, ncells);
  ///@}

  /**
   * Get the neighbors at an edge. More efficient than the general
   * GetCellNeighbors(). Assumes links have been built (with BuildLinks()),
   * and looks specifically for edge neighbors.
   */
  void GetCellEdgeNeighbors(vtkIdType cellId, vtkIdType p1, vtkIdType p2, vtkIdList* cellIds);

  /**
   * Get a list of point ids that define a cell. The cell type is
   * returned. Requires the the cells have been built with BuildCells.
   *
   * @warning Subsequent calls to this method may invalidate previous call
   * results.
   *
   * The @a pts pointer must not be modified.
   *
   * Note: This method MAY NOT be thread-safe. (See GetCellAtId at vtkCellArray)
   */
  unsigned char GetCellPoints(vtkIdType cellId, vtkIdType& npts, vtkIdType const*& pts)
    VTK_SIZEHINT(pts, npts);

  /**
   * Get a list of point ids that define a cell.
   * Requires the the cells have been built with BuildCells.
   *
   * This function MAY use ptIds, which is an object that is created by each thread,
   * to guarantee thread safety.
   *
   * @warning Subsequent calls to this method may invalidate previous call
   * results.
   *
   * The @a pts pointer must not be modified.
   *
   * Note: This method is thread-safe.
   */
  void GetCellPoints(vtkIdType cellId, vtkIdType& npts, vtkIdType const*& pts, vtkIdList* ptIds)
    VTK_SIZEHINT(pts, npts) override;

  /**
   * Given three vertices, determine whether it's a triangle. Make sure
   * BuildLinks() has been called first.
   */
  int IsTriangle(int v1, int v2, int v3);

  /**
   * Determine whether two points form an edge. If they do, return non-zero.
   * By definition PolyVertex and PolyLine have no edges since 1-dimensional
   * edges are only found on cells 2D and higher.
   * Edges are defined as 1-D boundary entities to cells.
   * Make sure BuildLinks() has been called first.
   */
  int IsEdge(vtkIdType p1, vtkIdType p2);

  /**
   * Determine whether a point is used by a particular cell. If it is, return
   * non-zero. Make sure BuildCells() has been called first.
   */
  int IsPointUsedByCell(vtkIdType ptId, vtkIdType cellId);

  /**
   * Replace the points defining cell "cellId" with a new set of points. This
   * operator is (typically) used when links from points to cells have not been
   * built (i.e., BuildLinks() has not been executed). Use the operator
   * ReplaceLinkedCell() to replace a cell when cell structure has been built. Use this
   * method only when the dataset is set as Editable.
   * @{
   */
  void ReplaceCell(vtkIdType cellId, vtkIdList* ids);
  void ReplaceCell(vtkIdType cellId, int npts, const vtkIdType pts[]) VTK_SIZEHINT(pts, npts);
  /**@}*/

  ///@{
  /**
   * Replace a point in the cell connectivity list with a different point. Use this
   * method only when the dataset is set as Editable.
   *
   * The version with cellPointIds avoids allocating/deallocating a vtkIdList at each call
   * internally.
   *
   * THIS METHOD IS THREAD SAFE IF BuildCells() IS FIRST CALLED FROM A SINGLE THREAD.
   */
  void ReplaceCellPoint(vtkIdType cellId, vtkIdType oldPtId, vtkIdType newPtId);
  void ReplaceCellPoint(
    vtkIdType cellId, vtkIdType oldPtId, vtkIdType newPtId, vtkIdList* cellPointIds);
  ///@}

  /**
   * Reverse the order of point ids defining the cell. Use this
   * method only when the dataset is set as Editable.
   */
  void ReverseCell(vtkIdType cellId);

  ///@{
  /**
   * Mark a point/cell as deleted from this vtkPolyData. Use this
   * method only when the dataset is set as Editable.
   */
  void DeletePoint(vtkIdType ptId);
  void DeleteCell(vtkIdType cellId);
  ///@}

  /**
   * The cells marked by calls to DeleteCell are stored in the Cell Array
   * VTK_EMPTY_CELL, but they still exist in the cell arrays.  Calling
   * RemoveDeletedCells will traverse the cell arrays and remove/compact the
   * cell arrays as well as any cell data thus truly removing the cells from
   * the polydata object. Use this method only when the dataset is set as
   * Editable.
   */
  void RemoveDeletedCells();

  ///@{
  /**
   * Add a point to the cell data structure (after cell pointers have been
   * built). This method adds the point and then allocates memory for the
   * links to the cells.  (To use this method, make sure points are available
   * and BuildLinks() has been invoked.) Of the two methods below, one inserts
   * a point coordinate and the other just makes room for cell links. Use this
   * method only when the dataset is set as Editable.
   */
  vtkIdType InsertNextLinkedPoint(int numLinks);
  vtkIdType InsertNextLinkedPoint(double x[3], int numLinks);
  ///@}

  /**
   * Add a new cell to the cell data structure (after cell pointers have been
   * built). This method adds the cell and then updates the links from the
   * points to the cells. (Memory is allocated as necessary.) Use this method
   * only when the dataset is set as Editable.
   */
  vtkIdType InsertNextLinkedCell(int type, int npts, const vtkIdType pts[]) VTK_SIZEHINT(pts, npts);

  /**
   * Replace one cell with another in cell structure. This operator updates
   * the connectivity list and the point's link list. It does not delete
   * references to the old cell in the point's link list. Use the operator
   * RemoveCellReference() to delete all references from points to (old)
   * cell.  You may also want to consider using the operator ResizeCellList()
   * if the link list is changing size. Use this method only when the dataset
   * is set as Editable.
   */
  void ReplaceLinkedCell(vtkIdType cellId, int npts, const vtkIdType pts[]) VTK_SIZEHINT(pts, npts);

  /**
   * Remove all references to cell in cell structure. This means the links
   * from the cell's points to the cell are deleted. Memory is not
   * reclaimed. Use the method ResizeCellList() to resize the link list from
   * a point to its using cells. (This operator assumes BuildLinks() has been
   * called.) Use this method only when the dataset is set as Editable.
   */
  void RemoveCellReference(vtkIdType cellId);

  /**
   * Add references to cell in cell structure. This means the links from
   * the cell's points to the cell are modified. Memory is not extended. Use the
   * method ResizeCellList() to resize the link list from a point to its using
   * cells. (This operator assumes BuildLinks() has been called.) Use this
   * method only when the dataset is set as Editable.
   */
  void AddCellReference(vtkIdType cellId);

  /**
   * Remove a reference to a cell in a particular point's link list. You may
   * also consider using RemoveCellReference() to remove the references from
   * all the cell's points to the cell. This operator does not reallocate
   * memory; use the operator ResizeCellList() to do this if necessary. Use
   * this method only when the dataset is set as Editable.
   */
  void RemoveReferenceToCell(vtkIdType ptId, vtkIdType cellId);

  /**
   * Add a reference to a cell in a particular point's link list. (You may also
   * consider using AddCellReference() to add the references from all the
   * cell's points to the cell.) This operator does not realloc memory; use the
   * operator ResizeCellList() to do this if necessary. Use this
   * method only when the dataset is set as Editable.
   */
  void AddReferenceToCell(vtkIdType ptId, vtkIdType cellId);

  /**
   * Resize the list of cells using a particular point. (This operator
   * assumes that BuildLinks() has been called.) Use this method only when
   * the dataset is set as Editable.
   */
  void ResizeCellList(vtkIdType ptId, int size);

  /**
   * Restore object to initial state. Release memory back to system.
   */
  void Initialize() override;

  ///@{
  /**
   * Get the piece and the number of pieces. Similar to extent in 3D.
   */
  virtual int GetPiece();
  virtual int GetNumberOfPieces();
  ///@}

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

  ///@{
  /**
   * Shallow and Deep copy.
   */
  void ShallowCopy(vtkDataObject* src) override;
  void DeepCopy(vtkDataObject* src) override;
  ///@}

  /**
   * This method will remove any cell that is marked as ghost
   * (has the vtkDataSetAttributes::DUPLICATECELL or
   * the vtkDataSetAttributes::HIDDENCELL bit set).
   * It does not remove unused points.
   */
  void RemoveGhostCells();

  ///@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  static vtkPolyData* GetData(vtkInformation* info);
  static vtkPolyData* GetData(vtkInformationVector* v, int i = 0);
  ///@}

  /**
   * Scalar field critical point classification (for manifold 2D meshes).
   * Reference: J. Milnor "Morse Theory", Princeton University Press, 1963.

   * Given a pointId and an attribute representing a scalar field, this member
   * returns the index of the critical point:
   * vtkPolyData::MINIMUM (index 0): local minimum;
   * vtkPolyData::SADDLE  (index 1): local saddle;
   * vtkPolyData::MAXIMUM (index 2): local maximum.

   * Other returned values are:
   * vtkPolyData::REGULAR_POINT: regular point (the gradient does not vanish);
   * vtkPolyData::ERR_NON_MANIFOLD_STAR: the star of the considered vertex is
   * not manifold (could not evaluate the index)
   * vtkPolyData::ERR_INCORRECT_FIELD: the number of entries in the scalar field
   * array is different form the number of vertices in the mesh.
   * vtkPolyData::ERR_NO_SUCH_FIELD: the specified scalar field does not exist.
   */
  enum
  {
    ERR_NO_SUCH_FIELD = -4,
    ERR_INCORRECT_FIELD = -3,
    ERR_NON_MANIFOLD_STAR = -2,
    REGULAR_POINT = -1,
    MINIMUM = 0,
    SADDLE = 1,
    MAXIMUM = 2
  };

  int GetScalarFieldCriticalIndex(vtkIdType pointId, vtkDataArray* scalarField);
  int GetScalarFieldCriticalIndex(vtkIdType pointId, int fieldId);
  int GetScalarFieldCriticalIndex(vtkIdType pointId, const char* fieldName);

  /**
   * Return the mesh (geometry/topology) modification time.
   * This time is different from the usual MTime which also takes into
   * account the modification of data arrays. This function can be used to
   * track the changes on the mesh separately from the data arrays
   * (eg. static mesh over time with transient data).
   */
  vtkMTimeType GetMeshMTime() override;

  /**
   * Get MTime which also considers its cell array MTime.
   */
  vtkMTimeType GetMTime() override;

  /**
   * Get a pointer to the cell, ie [npts pid1 .. pidn]. The cell type is
   * returned. Requires the the cells have been built with BuildCells.
   * The @a pts pointer must not be modified.
   *
   * @warning Internal cell storage has changed, and cell size is no longer
   * stored with the cell point ids. The `pts` array returned here no longer
   * exists in memory.
   */
  unsigned char GetCell(vtkIdType cellId, const vtkIdType*& pts);

protected:
  vtkPolyData();
  ~vtkPolyData() override;

  void ReportReferences(vtkGarbageCollector*) override;

  using TaggedCellId = vtkPolyData_detail::TaggedCellId;
  using CellMap = vtkPolyData_detail::CellMap;

  vtkCellArray* GetCellArrayInternal(TaggedCellId tag);

  // points inherited
  // point data (i.e., scalars, vectors, normals, tcoords) inherited
  vtkSmartPointer<vtkCellArray> Verts;
  vtkSmartPointer<vtkCellArray> Lines;
  vtkSmartPointer<vtkCellArray> Polys;
  vtkSmartPointer<vtkCellArray> Strips;

  // supporting structures for more complex topological operations
  // built only when necessary
  vtkSmartPointer<CellMap> Cells;
  vtkSmartPointer<vtkAbstractCellLinks> Links;

  vtkNew<vtkIdList> LegacyBuffer;

  // dummy static member below used as a trick to simplify traversal
  static vtkPolyDataDummyContainter DummyContainer;

  // Take into account only points that belong to at least one cell.
  double CellsBounds[6];

  vtkTimeStamp CellsBoundsTime;

private:
  void Cleanup();

  vtkPolyData(const vtkPolyData&) = delete;
  void operator=(const vtkPolyData&) = delete;
};

//------------------------------------------------------------------------------
inline vtkIdType vtkPolyData::GetNumberOfCells()
{
  return (this->GetNumberOfVerts() + this->GetNumberOfLines() + this->GetNumberOfPolys() +
    this->GetNumberOfStrips());
}

//------------------------------------------------------------------------------
inline int vtkPolyData::GetCellType(vtkIdType cellId)
{
  if (!this->Cells)
  {
    this->BuildCells();
  }
  return static_cast<int>(this->Cells->GetTag(cellId).GetCellType());
}

//------------------------------------------------------------------------------
inline vtkIdType vtkPolyData::GetCellSize(vtkIdType cellId)
{
  if (!this->Cells)
  {
    this->BuildCells();
  }
  switch (this->GetCellType(cellId))
  {
    case VTK_EMPTY_CELL:
      return 0;
    case VTK_VERTEX:
      return 1;
    case VTK_LINE:
      return 2;
    case VTK_TRIANGLE:
      return 3;
    case VTK_QUAD:
      return 4;
    case VTK_POLY_VERTEX:
      return this->Verts ? this->Verts->GetCellSize(this->GetCellIdRelativeToCellArray(cellId)) : 0;
    case VTK_POLY_LINE:
      return this->Lines ? this->Lines->GetCellSize(this->GetCellIdRelativeToCellArray(cellId)) : 0;
    case VTK_POLYGON:
      return this->Polys ? this->Polys->GetCellSize(this->GetCellIdRelativeToCellArray(cellId)) : 0;
    case VTK_TRIANGLE_STRIP:
      return this->Strips ? this->Strips->GetCellSize(this->GetCellIdRelativeToCellArray(cellId))
                          : 0;
  }
  vtkWarningMacro(<< "Cell type not supported.");
  return 0;
}

//------------------------------------------------------------------------------
inline int vtkPolyData::IsPointUsedByCell(vtkIdType ptId, vtkIdType cellId)
{
  vtkIdType npts;
  const vtkIdType* pts;

  this->GetCellPoints(cellId, npts, pts);
  for (vtkIdType i = 0; i < npts; i++)
  {
    if (pts[i] == ptId)
    {
      return 1;
    }
  }

  return 0;
}

//------------------------------------------------------------------------------
inline void vtkPolyData::DeletePoint(vtkIdType ptId)
{
  static_cast<vtkCellLinks*>(this->Links.Get())->DeletePoint(ptId);
}

//------------------------------------------------------------------------------
inline void vtkPolyData::DeleteCell(vtkIdType cellId)
{
  this->Cells->GetTag(cellId).MarkDeleted();
}

//------------------------------------------------------------------------------
inline void vtkPolyData::RemoveCellReference(vtkIdType cellId)
{
  const vtkIdType* pts;
  vtkIdType npts;

  this->GetCellPoints(cellId, npts, pts);
  auto links = static_cast<vtkCellLinks*>(this->Links.Get());
  for (vtkIdType i = 0; i < npts; i++)
  {
    links->RemoveCellReference(cellId, pts[i]);
  }
}

//------------------------------------------------------------------------------
inline void vtkPolyData::AddCellReference(vtkIdType cellId)
{
  const vtkIdType* pts;
  vtkIdType npts;

  this->GetCellPoints(cellId, npts, pts);
  auto links = static_cast<vtkCellLinks*>(this->Links.Get());
  for (vtkIdType i = 0; i < npts; i++)
  {
    links->AddCellReference(cellId, pts[i]);
  }
}

//------------------------------------------------------------------------------
inline void vtkPolyData::ResizeCellList(vtkIdType ptId, int size)
{
  static_cast<vtkCellLinks*>(this->Links.Get())->ResizeCellList(ptId, size);
}

//------------------------------------------------------------------------------
inline vtkCellArray* vtkPolyData::GetCellArrayInternal(vtkPolyData::TaggedCellId tag)
{
  switch (tag.GetTarget())
  {
    case vtkPolyData_detail::Target::Verts:
      return this->Verts;
    case vtkPolyData_detail::Target::Lines:
      return this->Lines;
    case vtkPolyData_detail::Target::Polys:
      return this->Polys;
    case vtkPolyData_detail::Target::Strips:
      return this->Strips;
  }
  return nullptr; // unreachable
}

//------------------------------------------------------------------------------
inline void vtkPolyData::ReplaceCellPoint(vtkIdType cellId, vtkIdType oldPtId, vtkIdType newPtId)
{
  vtkNew<vtkIdList> ids;
  this->ReplaceCellPoint(cellId, oldPtId, newPtId, ids);
}

//------------------------------------------------------------------------------
inline void vtkPolyData::ReplaceCellPoint(
  vtkIdType cellId, vtkIdType oldPtId, vtkIdType newPtId, vtkIdList* cellPointIds)
{
  if (!this->Cells)
  {
    this->BuildCells();
  }
  vtkIdType npts;
  const vtkIdType* pts;
  this->GetCellPoints(cellId, npts, pts, cellPointIds);
  for (vtkIdType i = 0; i < npts; i++)
  {
    if (pts[i] == oldPtId)
    {
      const TaggedCellId tag = this->Cells->GetTag(cellId);
      vtkCellArray* cells = this->GetCellArrayInternal(tag);
      cells->ReplaceCellPointAtId(tag.GetCellId(), i, newPtId);
      break;
    }
  }
}

//------------------------------------------------------------------------------
inline unsigned char vtkPolyData::GetCellPoints(
  vtkIdType cellId, vtkIdType& npts, vtkIdType const*& pts)
{
  if (!this->Cells)
  {
    this->BuildCells();
  }

  const TaggedCellId tag = this->Cells->GetTag(cellId);
  if (tag.IsDeleted())
  {
    npts = 0;
    pts = nullptr;
    return VTK_EMPTY_CELL;
  }

  vtkCellArray* cells = this->GetCellArrayInternal(tag);
  cells->GetCellAtId(tag.GetCellId(), npts, pts);
  return tag.GetCellType();
}

//------------------------------------------------------------------------------
inline void vtkPolyData::GetCellPoints(
  vtkIdType cellId, vtkIdType& npts, vtkIdType const*& pts, vtkIdList* ptIds)
{
  if (!this->Cells)
  {
    this->BuildCells();
  }

  const TaggedCellId tag = this->Cells->GetTag(cellId);
  if (tag.IsDeleted())
  {
    npts = 0;
    pts = nullptr;
  }
  else
  {
    vtkCellArray* cells = this->GetCellArrayInternal(tag);
    cells->GetCellAtId(tag.GetCellId(), npts, pts, ptIds);
  }
}

VTK_ABI_NAMESPACE_END
#endif
