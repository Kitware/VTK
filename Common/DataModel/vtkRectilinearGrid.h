// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkRectilinearGrid
 * @brief   a dataset that is topologically regular with variable spacing in the three coordinate
 * directions
 *
 * vtkRectilinearGrid is a data object that is a concrete implementation of
 * vtkDataSet. vtkRectilinearGrid represents a geometric structure that is
 * topologically regular with variable spacing in the three coordinate
 * directions x-y-z.
 *
 * To define a vtkRectilinearGrid, you must specify the dimensions of the
 * data and provide three arrays of values specifying the coordinates
 * along the x-y-z axes. The coordinate arrays are specified using three
 * vtkDataArray objects (one for x, one for y, one for z).
 *
 * @warning
 * Make sure that the dimensions of the grid match the number of coordinates
 * in the x-y-z directions. If not, unpredictable results (including
 * program failure) may result. Also, you must supply coordinates in all
 * three directions, even if the dataset topology is 2D, 1D, or 0D.
 */

#ifndef vtkRectilinearGrid_h
#define vtkRectilinearGrid_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataSet.h"
#include "vtkSmartPointer.h"   // For vtkSmartPointer
#include "vtkStructuredData.h" // For inline methods

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
class vtkStructuredCellArray;
class vtkPoints;

class VTKCOMMONDATAMODEL_EXPORT vtkRectilinearGrid : public vtkDataSet
{
public:
  static vtkRectilinearGrid* New();
  static vtkRectilinearGrid* ExtendedNew();

  vtkTypeMacro(vtkRectilinearGrid, vtkDataSet);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return what type of dataset this is.
   */
  int GetDataObjectType() VTK_FUTURE_CONST override { return VTK_RECTILINEAR_GRID; }

  /**
   * Copy the geometric and topological structure of an input rectilinear grid
   * object.
   */
  void CopyStructure(vtkDataSet* ds) override;

  /**
   * Restore object to initial state. Release memory back to system.
   */
  void Initialize() override;

  ///@{
  /**
   * Standard vtkDataSet API methods. See vtkDataSet for more information.
   */
  vtkIdType GetNumberOfCells() override;
  vtkIdType GetNumberOfPoints() override;
  vtkPoints* GetPoints() override;
  double* GetPoint(vtkIdType ptId) VTK_SIZEHINT(3) override;
  void GetPoint(vtkIdType id, double x[3]) override;
  vtkCell* GetCell(vtkIdType cellId) override;
  vtkCell* GetCell(int i, int j, int k) override;
  void GetCell(vtkIdType cellId, vtkGenericCell* cell) override;
  void GetCellBounds(vtkIdType cellId, double bounds[6]) override;
  vtkIdType FindPoint(double x[3]) override;
  vtkIdType FindCell(double x[3], vtkCell* cell, vtkIdType cellId, double tol2, int& subId,
    double pcoords[3], double* weights) override;
  vtkIdType FindCell(double x[3], vtkCell* cell, vtkGenericCell* gencell, vtkIdType cellId,
    double tol2, int& subId, double pcoords[3], double* weights) override;
  vtkCell* FindAndGetCell(double x[3], vtkCell* cell, vtkIdType cellId, double tol2, int& subId,
    double pcoords[3], double* weights) override;
  int GetCellType(vtkIdType cellId) override;
  vtkIdType GetCellSize(vtkIdType cellId) override;
  void GetCellPoints(vtkIdType cellId, vtkIdType& npts, vtkIdType const*& pts, vtkIdList* ptIds)
    VTK_SIZEHINT(pts, npts) override;
  void GetCellPoints(vtkIdType cellId, vtkIdList* ptIds) override;
  void GetPointCells(vtkIdType ptId, vtkIdList* cellIds) override
  {
    vtkStructuredData::GetPointCells(ptId, cellIds, this->Dimensions);
  }
  void ComputeBounds() override;
  int GetMaxCellSize() override { return 8; } // voxel is the largest
  int GetMaxSpatialDimension() override;
  int GetMinSpatialDimension() override;
  void GetCellNeighbors(vtkIdType cellId, vtkIdList* ptIds, vtkIdList* cellIds) override;
  void GetCellNeighbors(vtkIdType cellId, vtkIdList* ptIds, vtkIdList* cellIds, int* seedLoc);
  ///@}

  /**
   * Return the rectilinear grid connectivity array.
   *
   * NOTE: the returned object should not be modified.
   */
  vtkStructuredCellArray* GetCells();

  /**
   * Get the array of all cell types in the rectilinear grid. Each single-component
   * integer value is the same. The array is of size GetNumberOfCells().
   *
   * NOTE: the returned object should not be modified.
   */
  vtkConstantArray<int>* GetCellTypesArray();

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
   * Returns 1 if there is any visibility constraint on the points,
   * 0 otherwise.
   */
  bool HasAnyBlankPoints() override;
  /**
   * Returns 1 if there is any visibility constraint on the cells,
   * 0 otherwise.
   */
  bool HasAnyBlankCells() override;

  /**
   * Get the data description of the rectilinear grid.
   */
  vtkGetMacro(DataDescription, int);

  /**
   * Given the node dimensions of this grid instance, this method computes the
   * node dimensions. The value in each dimension can will have a lowest value
   * of "1" such that computing the total number of cells can be achieved by
   * simply by cellDims[0]*cellDims[1]*cellDims[2].
   */
  void GetCellDims(int cellDims[3]);

  ///@{
  /**
   * Set dimensions of rectilinear grid dataset.
   * This also sets the extent.
   */
  void SetDimensions(int i, int j, int k);
  void SetDimensions(const int dim[3]);
  ///@}

  ///@{
  /**
   * Get dimensions of this rectilinear grid dataset.
   */
  vtkGetVectorMacro(Dimensions, int, 3);
  ///@}

  /**
   * Return the dimensionality of the data.
   */
  int GetDataDimension();

  /**
   * Convenience function computes the structured coordinates for a point x[3].
   * The cell is specified by the array ijk[3], and the parametric coordinates
   * in the cell are specified with pcoords[3]. The function returns a 0 if the
   * point x is outside of the grid, and a 1 if inside the grid.
   */
  int ComputeStructuredCoordinates(double x[3], int ijk[3], double pcoords[3]);

  /**
   * Given a location in structured coordinates (i-j-k), return the point id.
   */
  vtkIdType ComputePointId(int ijk[3]);

  /**
   * Given a location in structured coordinates (i-j-k), return the cell id.
   */
  vtkIdType ComputeCellId(int ijk[3]);

  /**
   * Given the IJK-coordinates of the point, it returns the corresponding
   * xyz-coordinates. The xyz coordinates are stored in the user-supplied
   * array p.
   */
  void GetPoint(int i, int j, int k, double p[3]);

  ///@{
  /**
   * Specify the grid coordinates in the x-direction.
   */
  virtual void SetXCoordinates(vtkDataArray*);
  vtkGetObjectMacro(XCoordinates, vtkDataArray);
  ///@}

  ///@{
  /**
   * Specify the grid coordinates in the y-direction.
   */
  virtual void SetYCoordinates(vtkDataArray*);
  vtkGetObjectMacro(YCoordinates, vtkDataArray);
  ///@}

  ///@{
  /**
   * Specify the grid coordinates in the z-direction.
   */
  virtual void SetZCoordinates(vtkDataArray*);
  vtkGetObjectMacro(ZCoordinates, vtkDataArray);
  ///@}

  ///@{
  /**
   * Different ways to set the extent of the data array.  The extent
   * should be set before the "Scalars" are set or allocated.
   * The Extent is stored in the order (X, Y, Z).
   */
  void SetExtent(int extent[6]);
  void SetExtent(int xMin, int xMax, int yMin, int yMax, int zMin, int zMax);
  vtkGetVector6Macro(Extent, int);
  ///@}

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
   * Structured extent. The extent type is a 3D extent
   */
  int GetExtentType() VTK_FUTURE_CONST override { return VTK_3D_EXTENT; }

  /**
   * Reallocates and copies to set the Extent to the UpdateExtent.
   * This is used internally when the exact extent is requested,
   * and the source generated more than the update extent.
   */
  void Crop(const int* updateExtent) override;

  ///@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  static vtkRectilinearGrid* GetData(vtkInformation* info);
  static vtkRectilinearGrid* GetData(vtkInformationVector* v, int i = 0);
  ///@}

  ///@{
  /**
   * Set/Get the scalar data type for the points. This is setting pipeline info.
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
   */
  static void SetNumberOfScalarComponents(int n, vtkInformation* meta_data);
  static int GetNumberOfScalarComponents(vtkInformation* meta_data);
  static bool HasNumberOfScalarComponents(vtkInformation* meta_data);
  int GetNumberOfScalarComponents();
  ///@}

protected:
  vtkRectilinearGrid();
  ~vtkRectilinearGrid() override;

  int Dimensions[3];
  int DataDescription;

  int Extent[6];

  vtkDataArray* XCoordinates;
  vtkDataArray* YCoordinates;
  vtkDataArray* ZCoordinates;

  // Hang on to some space for returning points when GetPoint(id) is called.
  double Point[3];

  vtkSmartPointer<vtkPoints> StructuredPoints;
  vtkSmartPointer<vtkStructuredCellArray> StructuredCells;
  vtkSmartPointer<vtkConstantArray<int>> StructuredCellTypes;

  void BuildImplicitStructures();
  void BuildPoints();
  void BuildCells();
  void BuildCellTypes();

private:
  void Cleanup();

  vtkRectilinearGrid(const vtkRectilinearGrid&) = delete;
  void operator=(const vtkRectilinearGrid&) = delete;
};

//----------------------------------------------------------------------------
inline double* vtkRectilinearGrid::GetPoint(vtkIdType id)
{
  this->GetPoint(id, this->Point);
  return this->Point;
}

//----------------------------------------------------------------------------
inline vtkIdType vtkRectilinearGrid::GetNumberOfPoints()
{
  return vtkStructuredData::GetNumberOfPoints(this->Extent);
}

//----------------------------------------------------------------------------
inline vtkIdType vtkRectilinearGrid::GetNumberOfCells()
{
  return vtkStructuredData::GetNumberOfCells(this->Extent);
}

//----------------------------------------------------------------------------
inline int vtkRectilinearGrid::GetDataDimension()
{
  return vtkStructuredData::GetDataDimension(this->DataDescription);
}

//----------------------------------------------------------------------------
inline int vtkRectilinearGrid::GetMaxSpatialDimension()
{
  return vtkStructuredData::GetDataDimension(this->DataDescription);
}

//----------------------------------------------------------------------------
inline int vtkRectilinearGrid::GetMinSpatialDimension()
{
  return vtkStructuredData::GetDataDimension(this->DataDescription);
}

//----------------------------------------------------------------------------
inline vtkIdType vtkRectilinearGrid::ComputePointId(int ijk[3])
{
  return vtkStructuredData::ComputePointId(this->Dimensions, ijk);
}

//----------------------------------------------------------------------------
inline vtkIdType vtkRectilinearGrid::ComputeCellId(int ijk[3])
{
  return vtkStructuredData::ComputeCellId(this->Dimensions, ijk);
}

VTK_ABI_NAMESPACE_END
#endif
