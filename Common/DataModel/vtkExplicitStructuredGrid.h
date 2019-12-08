/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExplicitStructuredGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkExplicitStructuredGrid
 * @brief   structured grid with explicit topology and geometry
 *
 * vtkExplicitStructuredGrid is a data object that is a concrete implementation
 * of vtkDataSet. vtkExplicitStructuredGrid represents a geometric structure
 * that is a topologically regular array of hexahedron. The topology is that of
 * a cube that has been subdivided into a regular array of smaller cubes.
 * Each cell can be addressed with i-j-k indices, however neighbor hexahedrons
 * does not necessarily share a face and hexahedron can be blanked (turned-off).
 *
 * Like unstructured grid, vtkExplicitStructuredGrid has explicit point coordinates
 * and cell to point indexing.
 * Unlike unstructured grid, vtkExplicitStructuredGrid does not keep a cell type
 * list as all visible cells are known to be hexahedra.
 * vtkExplicitStructuredGrid can take advantage of its layout to perform operations
 * based on the i, j, k parameters, similar to structured grid. This makes some
 * operations faster on this class, without losing the flexibility of the
 * cell -> points mapping.
 * The most common use of this class would be in situations where you have all
 * hexahedra but the points used by the cells are not exactly defined by the
 * i, j, k parameters. One example of this is a structured grid with a half voxel
 * shift occurring in the middle of it such as with a geologic fault.
 *
 * The order and number of points is arbitrary.
 * The order and number of cells must match that specified by the dimensions
 * of the grid minus 1, because in vtk structured datasets the dimensions
 * correspond to the points.
 * The cells order increases in i fastest (from 0 <= i <= dims[0] - 2),
 * then j (0 <= j <= dims[1] - 2), then k ( 0 <= k <= dims[2] - 2) where dims[]
 * are the dimensions of the grid in the i-j-k topological directions.
 * The number of cells is (dims[0] - 1) * (dims[1] - 1) * (dims[2] - 1).
 */

#ifndef vtkExplicitStructuredGrid_h
#define vtkExplicitStructuredGrid_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNew.h"                   // for vtkNew
#include "vtkPointSet.h"
#include "vtkStructuredData.h" // For static method usage

class vtkCellArray;
class vtkAbstractCellLinks;
class vtkEmptyCell;
class vtkHexahedron;

class VTKCOMMONDATAMODEL_EXPORT vtkExplicitStructuredGrid : public vtkPointSet
{
public:
  //@{
  /**
   * Standard methods for instantiation, type information, and printing.
   */
  static vtkExplicitStructuredGrid* New();
  vtkTypeMacro(vtkExplicitStructuredGrid, vtkPointSet);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  /**
   * Return what type of dataset this is.
   */
  int GetDataObjectType() override { return VTK_EXPLICIT_STRUCTURED_GRID; }

  //@{
  /**
   * Standard vtkDataSet API methods. See vtkDataSet for more information.
   */
  void Initialize() override;
  using vtkDataSet::GetCell;
  vtkCell* GetCell(vtkIdType cellId) override;
  void GetCell(vtkIdType cellId, vtkGenericCell* cell) override;
  void GetCellBounds(vtkIdType cellId, double bounds[6]) override;
  int GetCellType(vtkIdType cellId) override;
  vtkIdType GetNumberOfCells() override;
  void GetCellPoints(vtkIdType cellId, vtkIdList* ptIds) override;
  void GetPointCells(vtkIdType ptId, vtkIdList* cellIds) override;
  int GetMaxCellSize() override { return 8; } // hexahedron is the largest
  void GetCellNeighbors(vtkIdType cellId, vtkIdList* ptIds, vtkIdList* cellIds) override;
  //@}

  /**
   * Copy the geometric and topological structure of an input poly data object.
   */
  void CopyStructure(vtkDataSet* ds) override;

  //@{
  /**
   * Shallow and Deep copy.
   */
  void ShallowCopy(vtkDataObject* src) override;
  void DeepCopy(vtkDataObject* src) override;
  //@}

  /**
   * Return the dimensionality of the data.
   */
  inline int GetDataDimension() { return 3; }

  //@{
  /**
   * Set/Get the dimensions of this structured dataset in term of number
   * of points along each direction.
   * This is just a convenience method which calls Set/GetExtent internally.
   */
  void SetDimensions(int i, int j, int k);
  void SetDimensions(int dim[3]);
  void GetDimensions(int dim[3]);
  //@}

  /**
   * Computes the cell dimensions according to internal point dimensions.
   * The total number of cells can be achieved simply by
   * cellDims[0] * cellDims[1] * cellDims[2].
   */
  void GetCellDims(int cellDims[3]);

  /**
   * The extent type is a 3D extent
   */
  int GetExtentType() override { return VTK_3D_EXTENT; }

  //@{
  /**
   * Set/Get the extent of this structured dataset in term of number
   * of points along each direction.
   * Setting the extent will reset the internal CellArray and Links
   * and a correctly sized cell array will be created.
   * The Extent is stored  in the order (X, Y, Z).
   */
  void SetExtent(int x0, int x1, int y0, int y1, int z0, int z1);
  void SetExtent(int extent[6]);
  vtkGetVector6Macro(Extent, int);
  //@}

  //@{
  /**
   * Set/Get the cell array defining hexahedron.
   */
  void SetCells(vtkCellArray* cells);
  vtkGetObjectMacro(Cells, vtkCellArray);
  //@}

  //@{
  /**
   * Create/Get upward links from points to cells that use each point.
   * Enables topologically complex queries.
   */
  void BuildLinks();
  vtkGetObjectMacro(Links, vtkAbstractCellLinks);
  //@}

  /**
   * Get direct raw pointer to the 8 points indices of an hexahedra.
   */
  vtkIdType* GetCellPoints(vtkIdType cellId);

  /**
   * More efficient method to obtain cell points.
   */
  void GetCellPoints(vtkIdType cellId, vtkIdType& npts, vtkIdType*& pts);

  /**
   * Get cell neighbors of the cell for every faces.
   */
  void GetCellNeighbors(vtkIdType cellId, vtkIdType neighbors[6], int* wholeExtent = nullptr);

  /**
   * Given a cellId, get the structured coordinates (i-j-k).
   * If adjustForExtent is true, (i,j,k) is computed as a position relative
   * to the beginning of the extent.
   * If adjustForExtent is false, (i,j,k) is computed regardless of the
   * extent beginning.
   * The default adjustForExtent is true.
   */
  void ComputeCellStructuredCoords(
    vtkIdType cellId, int& i, int& j, int& k, bool adjustForExtent = true);

  /**
   * Given a location in structured coordinates (i-j-k), return the cell id.
   * If adjustForExtent is true, (i,j,k) is interpreted as a position relative
   * to the beginning of the extent.
   * If adjustForExtent is false, (i,j,k) is interpreted literally
   * and the cell id is returned regardless of the extent beginning.
   * The default adjustForExtent is true.
   */
  vtkIdType ComputeCellId(int i, int j, int k, bool adjustForExtent = true);

  /**
   * Compute the faces connectivity flags array.
   */
  void ComputeFacesConnectivityFlagsArray();

  //@{
  /**
   * Set/Get the name of the faces connectivity flags array.
   */
  vtkSetStringMacro(FacesConnectivityFlagsArrayName);
  vtkGetStringMacro(FacesConnectivityFlagsArrayName);
  //@}

  //@{
  /**
   * Methods for supporting blanking of cells. Blanking turns on or off
   * cells in the structured grid.
   * These methods should be called only after the dimensions of the
   * grid are set.
   */
  void BlankCell(vtkIdType cellId);
  void UnBlankCell(vtkIdType cellId);
  //@}

  /**
   * Returns true if one or more cells are blanked, false otherwise.
   */
  bool HasAnyBlankCells() override;

  /**
   * Return non-zero value if specified cell is visible.
   * These methods should be called only after the dimensions of the
   * grid are set.
   */
  unsigned char IsCellVisible(vtkIdType cellId);

  /**
   * Return non-zero value if specified cell is a ghost cell.
   * These methods should be called only after the dimensions of the
   * grid are set.
   */
  unsigned char IsCellGhost(vtkIdType cellId);

  /**
   * Returns true if one or more cells are ghost, false otherwise.
   */
  bool HasAnyGhostCells();

  //@{
  /**
   * Reallocates and copies to set the Extent to the UpdateExtent.
   * This is used internally when the exact extent is requested,
   * and the source generated more than the update extent.
   */
  void Crop(const int* updateExtent) override;
  virtual void Crop(
    vtkExplicitStructuredGrid* input, const int* updateExtent, bool generateOriginalCellIds);
  //@}

  //@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  static vtkExplicitStructuredGrid* GetData(vtkInformation* info);
  static vtkExplicitStructuredGrid* GetData(vtkInformationVector* v, int i = 0);
  //@}

  /**
   * Return the actual size of the data in kilobytes. This number
   * is valid only after the pipeline has updated. The memory size
   * returned is guaranteed to be greater than or equal to the
   * memory required to represent the data (e.g., extra space in
   * arrays, etc. are not included in the return value). THIS METHOD
   * IS THREAD SAFE.
   */
  unsigned long GetActualMemorySize() override;

  /**
   * Check faces are numbered correctly regarding ijk numbering
   * If not this will reorganize cell points order
   * so face order is valid.
   * This is made in two pass, first it check that faces are on
   * the correct axis and corrects it
   * Then it check if faces are mirrored and corrects it.
   * Make sure cells and extent have been set before calling this method
   * and recompute face connectivity afterwards.
   */
  void CheckAndReorderFaces();

  //@{
  /**
   * Normally called by pipeline executives or algorithms only. This method
   * computes the ghost arrays for a given dataset. The zeroExt argument
   * specifies the extent of the region which ghost type = 0.
   */
  using vtkDataSet::GenerateGhostArray;
  void GenerateGhostArray(int zeroExt[6], bool cellOnly) override;
  //@}

protected:
  vtkExplicitStructuredGrid();
  ~vtkExplicitStructuredGrid() override;

  /**
   * Compute the range of the scalars and cache it into ScalarRange
   * only if the cache became invalid (ScalarRangeComputeTime).
   */
  virtual void ComputeScalarRange() override;

  /**
   * Internal method used by DeepCopy and ShallowCopy.
   */
  virtual void InternalCopy(vtkExplicitStructuredGrid* src);

  /**
   * Internal method used by GetCell.
   */
  void GetCell(vtkIdType, vtkCell*);

  /**
   * Internal method used by CheckAndReorderFaces
   */
  void InternalCheckAndReorderFaces(bool swap);

  /**
   * Find a connected face for each axis if any.
   * Return the number of found connected faces
   */
  int FindConnectedFaces(int foundFaces[3]);

  /**
   * Check a list of connected faces and remove invalid face or
   * extrapolate missing faces
   */
  static void CheckConnectedFaces(int& nFoundFaces, int foundFaces[3]);

  /**
   * Compute a swap flag based if a face have been found
   */
  static void ComputeSwapFlag(int foundFaces[3], int swap[3]);

  /**
   * Compute a mirror flag based if a face have been found
   */
  static void ComputeMirrorFlag(int foundFaces[3], int mirror[3]);

  /**
   * Reorder all cells points based on a transformFlag for each axis and a points map
   */
  void ReorderCellsPoints(const int* ptsMap, const int transformFlag[3]);

  // Used by GetCell method
  vtkNew<vtkHexahedron> Hexahedron;
  vtkNew<vtkEmptyCell> EmptyCell;

  vtkCellArray* Cells;
  vtkAbstractCellLinks* Links;
  int Extent[6];
  char* FacesConnectivityFlagsArrayName;

private:
  vtkExplicitStructuredGrid(const vtkExplicitStructuredGrid&) = delete;
  void operator=(const vtkExplicitStructuredGrid&) = delete;
};

//----------------------------------------------------------------------------
inline void vtkExplicitStructuredGrid::GetDimensions(int dim[3])
{
  vtkStructuredData::GetDimensionsFromExtent(this->Extent, dim);
}

//----------------------------------------------------------------------------
inline void vtkExplicitStructuredGrid::GetCellDims(int cellDims[3])
{
  vtkStructuredData::GetCellDimensionsFromExtent(this->Extent, cellDims);
}

//----------------------------------------------------------------------------
inline void vtkExplicitStructuredGrid::ComputeCellStructuredCoords(
  vtkIdType cellId, int& i, int& j, int& k, bool adjustForExtent)
{
  int ijk[3];
  if (adjustForExtent)
  {
    vtkStructuredData::ComputeCellStructuredCoordsForExtent(cellId, this->Extent, ijk);
  }
  else
  {
    int dims[3];
    this->GetDimensions(dims);
    vtkStructuredData::ComputeCellStructuredCoords(cellId, dims, ijk);
  }
  i = ijk[0];
  j = ijk[1];
  k = ijk[2];
}

//----------------------------------------------------------------------------
inline vtkIdType vtkExplicitStructuredGrid::ComputeCellId(int i, int j, int k, bool adjustForExtent)
{
  int ijk[] = { i, j, k };
  if (adjustForExtent)
  {
    return vtkStructuredData::ComputeCellIdForExtent(this->Extent, ijk);
  }
  else
  {
    int dims[3];
    this->GetDimensions(dims);
    return vtkStructuredData::ComputeCellId(dims, ijk);
  }
}
#endif
