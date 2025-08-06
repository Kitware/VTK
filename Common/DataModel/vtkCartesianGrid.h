// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCartesianGrid
 * @brief   Abstract API for vtkImageData and vtkRectilinearGrid
 *
 * vtkCartesianGrid is an abstract class that provide a common API for both
 * vtkImageData and vtkRectilinearGrid.
 *
 * It contains the logic related to the handling of extents, dimensions and data description,
 * As well as many methods that behave the same for image data and rectilinear grid.
 */

#ifndef vtkCartesianGrid_h
#define vtkCartesianGrid_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataSet.h"
#include "vtkStructuredData.h" // For inline methods

VTK_ABI_NAMESPACE_BEGIN
class vtkStructuredCellArray;
class VTKCOMMONDATAMODEL_EXPORT VTK_MARSHALAUTO vtkCartesianGrid : public vtkDataSet
{
public:
  ///@{
  /**
   * Standard vtkObject API methods. See vtkObject for more information.
   */
  vtkTypeMacro(vtkCartesianGrid, vtkDataSet);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  void ShallowCopy(vtkDataObject* src) override;
  void DeepCopy(vtkDataObject* src) override;
  void CopyStructure(vtkDataSet* ds) override;
  ///@}

  using Superclass::FindCell;
  using Superclass::GetCell;
  using Superclass::GetPoint;

  ///@{
  /**
   * Standard vtkDataSet API methods. See vtkDataSet for more information.
   */
  vtkPoints* GetPoints() override;
  void GetPoint(vtkIdType id, double x[3]) override;
  double* GetPoint(vtkIdType ptId) VTK_SIZEHINT(3) override;
  vtkIdType GetNumberOfCells() override;
  vtkIdType GetNumberOfPoints() override;
  int GetMaxSpatialDimension() override;
  int GetMinSpatialDimension() override;
  int GetCellType(vtkIdType cellId) override;
  vtkIdType GetCellSize(vtkIdType cellId) override;
  void GetCellPoints(vtkIdType cellId, vtkIdType& npts, vtkIdType const*& pts, vtkIdList* ptIds)
    VTK_SIZEHINT(pts, npts) override;
  void GetCellPoints(vtkIdType cellId, vtkIdList* ptIds) override;
  vtkIdType FindCell(double x[3], vtkCell* cell, vtkGenericCell* gencell, vtkIdType cellId,
    double tol2, int& subId, double pcoords[3], double* weights) override;
  vtkCell* FindAndGetCell(double x[3], vtkCell* cell, vtkIdType cellId, double tol2, int& subId,
    double pcoords[3], double* weights) override;
  void GetPointCells(vtkIdType ptId, vtkIdList* cellIds) override
  {
    vtkStructuredData::GetPointCells(ptId, cellIds, this->GetDimensions());
  }
  int GetMaxCellSize() override { return 8; } // voxel is the largest
  void GetCellNeighbors(vtkIdType cellId, vtkIdList* ptIds, vtkIdList* cellIds) override;
  vtkCell* GetCell(vtkIdType cellId) override;
  vtkCell* GetCell(int i, int j, int k) override;
  bool HasAnyBlankPoints() override;
  bool HasAnyBlankCells() override;
  void Initialize() override;
  ///@}

  /**
   * Computes the structured coordinates for a point x[3].
   * The cell is specified by the array ijk[3], and the parametric coordinates
   * in the cell are specified with pcoords[3]. The function returns a 0 if the
   * point x is outside of the grid, and a 1 if inside the grid.
   */
  virtual int ComputeStructuredCoordinates(const double x[3], int ijk[3], double pcoords[3]) = 0;

  /**
   * Given a location in structured coordinates (i-j-k), return the point id.
   */
  virtual vtkIdType ComputePointId(int ijk[3]) = 0;

  /**
   * Given a location in structured coordinates (i-j-k), return the cell id.
   */
  virtual vtkIdType ComputeCellId(int ijk[3]) = 0;

  /**
   * Get cell neighbors around cell located at `seedloc`, except cell of id `cellId`.
   *
   * @warning `seedloc` is the position in the grid with the origin shifted to (0, 0, 0).
   * This is because the backend of this method is shared with `vtkRectilinearGrid` and
   * `vtkStructuredGrid`.
   */
  void GetCellNeighbors(vtkIdType cellId, vtkIdList* ptIds, vtkIdList* cellIds, int* seedLoc);

  /**
   * Return the dimensionality of the data.
   */
  int GetDataDimension();

  /**
   * Return the image data connectivity array.
   *
   * NOTE: the returned object should not be modified.
   */
  vtkStructuredCellArray* GetCells();

  /**
   * Get the array of all cell types in the image data. Each single-component
   * integer value is the same. The array is of size GetNumberOfCells().
   *
   * NOTE: the returned object should not be modified.
   */
  vtkConstantArray<int>* GetCellTypesArray();

  /**
   * Given the node dimensions of this grid instance, this method computes the
   * node dimensions. The value in each dimension can will have a lowest value
   * of "1" such that computing the total number of cells can be achieved by
   * simply by cellDims[0]*cellDims[1]*cellDims[2].
   */
  void GetCellDims(int cellDims[3]);

  ///@{
  /**
   * Methods for supporting blanking of cells. Blanking turns on or off
   * points in the structured grid, and hence the cells connected to them.
   * These methods should be called only after the dimensions of the
   * grid are set.
   */
  virtual void BlankPoint(vtkIdType ptId);
  virtual void UnBlankPoint(vtkIdType ptId);
  virtual void BlankPoint(int i, int j, int k);
  virtual void UnBlankPoint(int i, int j, int k);
  ///@}

  ///@{
  /**
   * Methods for supporting blanking of cells. Blanking turns on or off
   * cells in the structured grid.
   * These methods should be called only after the dimensions of the
   * grid are set.
   */
  virtual void BlankCell(vtkIdType ptId);
  virtual void UnBlankCell(vtkIdType ptId);
  virtual void BlankCell(int i, int j, int k);
  virtual void UnBlankCell(int i, int j, int k);
  ///@}

  /**
   * Return non-zero value if specified point is visible.
   * These methods should be called only after the dimensions of the
   * grid are set.
   */
  unsigned char IsPointVisible(vtkIdType ptId);

  /**
   * Return non-zero value if specified point is visible.
   * These methods should be called only after the dimensions of the
   * grid are set.
   */
  unsigned char IsCellVisible(vtkIdType cellId);

  /**
   * Get the data description of the rectilinear grid.
   */
  vtkGetMacro(DataDescription, int);

  ///@{
  /**
   * Set dimensions of rectilinear grid dataset.
   * This also sets the extent.
   */
  void SetDimensions(int i, int j, int k) { this->SetExtent(0, i - 1, 0, j - 1, 0, k - 1); };
  void SetDimensions(const int dim[3])
  {
    this->SetExtent(0, dim[0] - 1, 0, dim[1] - 1, 0, dim[2] - 1);
  };
  ///@}

  /**
   * Get dimensions of this structured points dataset.
   * It is the number of points on each axis.
   * Dimensions are computed from Extents during this call.
   * \warning Non thread-safe, use second signature if you want it to be.
   */
  int* GetDimensions() VTK_SIZEHINT(3);

  /**
   * Get dimensions of this structured points dataset.
   * It is the number of points on each axis.
   * This method is thread-safe.
   * \warning The Dimensions member variable is not updated during this call.
   */
  void GetDimensions(int dims[3]);
#if VTK_ID_TYPE_IMPL != VTK_INT
  void GetDimensions(vtkIdType dims[3]);
#endif

  /**
   * Structured extent. The extent type is a 3D extent
   */
  int GetExtentType() VTK_FUTURE_CONST override { return VTK_3D_EXTENT; }

  ///@{
  /**
   * Set/Get the extent. On each axis, the extent is defined by the index
   * of the first point and the index of the last point.  The extent should
   * be set before the "Scalars" are set or allocated.  The Extent is
   * stored in the order (X, Y, Z).
   * The dataset extent does not have to start at (0,0,0). (0,0,0) is just the
   * extent of the origin.
   * The first point (the one with Id=0) is at extent
   * (Extent[0],Extent[2],Extent[4]). As for any dataset, a data array on point
   * data starts at Id=0.
   */
  void SetExtent(int extent[6]);
  void SetExtent(int xMin, int xMax, int yMin, int yMax, int zMin, int zMax);
  vtkGetVector6Macro(Extent, int);
  ///@}

  ///@{
  /**
   * Set/Get the scalar data type for the points. This is setting pipeline info.
   * Assume VTK_DOUBLE if not set or empty.
   */
  static void SetScalarType(int, vtkInformation* meta_data);
  static int GetScalarType(vtkInformation* meta_data);
  static bool HasScalarType(vtkInformation* meta_data);
  int GetScalarType();
  const char* GetScalarTypeAsString() { return vtkImageScalarTypeNameMacro(this->GetScalarType()); }
  ///@}

  ///@{
  /**
   * Set/Get the number of scalar components for points. As with the
   * SetScalarType method this is setting pipeline info.
   * 1 if not set or empty.
   */
  static void SetNumberOfScalarComponents(int n, vtkInformation* meta_data);
  static int GetNumberOfScalarComponents(vtkInformation* meta_data);
  static bool HasNumberOfScalarComponents(vtkInformation* meta_data);
  int GetNumberOfScalarComponents();
  ///@}

protected:
  vtkCartesianGrid();
  ~vtkCartesianGrid() override = default;

  /**
   * Set the internally built structured points
   */
  void SetStructuredPoints(vtkPoints*);

  /**
   * Pure abstract method responsible to build and set internal points
   */
  virtual void BuildPoints() = 0;

private:
  // Build internals fields
  void BuildImplicitStructures();
  void BuildCells();
  void BuildCellTypes();

  // API members
  int DataDescription = vtkStructuredData::VTK_STRUCTURED_EMPTY;
  int Dimensions[3] = { 0, 0, 0 };
  int Extent[6] = { 0, -1, 0, -1, 0, -1 };

  // For the GetPoint method
  double Point[3] = { 0, 0, 0 };

  // Internals fields
  vtkSmartPointer<vtkPoints> StructuredPoints;
  vtkSmartPointer<vtkStructuredCellArray> StructuredCells;
  vtkSmartPointer<vtkConstantArray<int>> StructuredCellTypes;
};

//----------------------------------------------------------------------------
inline double* vtkCartesianGrid::GetPoint(vtkIdType id)
{
  this->GetPoint(id, this->Point);
  return this->Point;
}

//----------------------------------------------------------------------------
inline vtkIdType vtkCartesianGrid::GetNumberOfPoints()
{
  return vtkStructuredData::GetNumberOfPoints(this->Extent);
}

//----------------------------------------------------------------------------
inline vtkIdType vtkCartesianGrid::GetNumberOfCells()
{
  return vtkStructuredData::GetNumberOfCells(this->Extent);
}

//----------------------------------------------------------------------------
inline int vtkCartesianGrid::GetDataDimension()
{
  return vtkStructuredData::GetDataDimension(this->DataDescription);
}

//----------------------------------------------------------------------------
inline int vtkCartesianGrid::GetMaxSpatialDimension()
{
  return vtkStructuredData::GetDataDimension(this->DataDescription);
}

//----------------------------------------------------------------------------
inline int vtkCartesianGrid::GetMinSpatialDimension()
{
  return vtkStructuredData::GetDataDimension(this->DataDescription);
}

VTK_ABI_NAMESPACE_END
#endif
