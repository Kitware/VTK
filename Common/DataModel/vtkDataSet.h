// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDataSet
 * @brief   abstract class to specify dataset behavior
 *
 * vtkDataSet is an abstract class that specifies an interface for dataset
 * objects. vtkDataSet also provides methods to provide information about
 * the data, such as center, bounding box, and representative length.
 *
 * In vtk a dataset consists of a structure (geometry and topology) and
 * attribute data. The structure is defined implicitly or explicitly as
 * a collection of cells. The geometry of the structure is contained in
 * point coordinates plus the cell interpolation functions. The topology
 * of the dataset structure is defined by cell types and how the cells
 * share their defining points.
 *
 * Attribute data in vtk is either point data (data at points) or cell data
 * (data at cells). Typically filters operate on point data, but some may
 * operate on cell data, both cell and point data, either one, or none.
 *
 * @sa
 * vtkPointSet vtkStructuredPoints vtkStructuredGrid vtkUnstructuredGrid
 * vtkRectilinearGrid vtkPolyData vtkPointData vtkCellData
 * vtkDataObject vtkFieldData
 */

#ifndef vtkDataSet_h
#define vtkDataSet_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataObject.h"
#include "vtkNew.h"           // For vtkNew
#include "vtkSmartPointer.h"  // For vtkSmartPointer
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkCell;
class vtkCellData;
class vtkCellIterator;
class vtkCellTypes;
class vtkGenericCell;
class vtkIdList;
class vtkPointData;
class vtkPoints;
class vtkUnsignedCharArray;
class vtkCallbackCommand;

class VTKCOMMONDATAMODEL_EXPORT VTK_MARSHALAUTO vtkDataSet : public vtkDataObject
{
public:
  vtkTypeMacro(vtkDataSet, vtkDataObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Copy the geometric and topological structure of an object. Note that
   * the invoking object and the object pointed to by the parameter ds must
   * be of the same type.
   * THIS METHOD IS NOT THREAD SAFE.
   */
  virtual void CopyStructure(vtkDataSet* ds) = 0;

  /**
   * Copy the attributes associated with the specified dataset to this
   * instance of vtkDataSet.
   * THIS METHOD IS NOT THREAD SAFE.
   */
  virtual void CopyAttributes(vtkDataSet* ds);

  /**
   * Determine the number of points composing the dataset.
   * THIS METHOD IS THREAD SAFE
   */
  virtual vtkIdType GetNumberOfPoints() = 0;

  /**
   * Determine the number of cells composing the dataset.
   * THIS METHOD IS THREAD SAFE
   */
  virtual vtkIdType GetNumberOfCells() = 0;

  /**
   * If the subclass has (implicit/explicit) points, then return them.
   * Otherwise, create a vtkPoints object and return that.
   *
   * DO NOT MODIFY THE RETURNED POINTS OBJECT.
   */
  virtual vtkPoints* GetPoints();

  /**
   * Get point coordinates with ptId such that: 0 <= ptId < NumberOfPoints.
   * THIS METHOD IS NOT THREAD SAFE.
   */
  virtual double* GetPoint(vtkIdType ptId) VTK_SIZEHINT(3) = 0;

  /**
   * Copy point coordinates into user provided array x[3] for specified
   * point id.
   * THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
   * THE DATASET IS NOT MODIFIED
   */
  virtual void GetPoint(vtkIdType id, double x[3]);

  /**
   * Return an iterator that traverses the cells in this data set.
   */
  VTK_NEWINSTANCE
  virtual vtkCellIterator* NewCellIterator();

  /**
   * Get cell with cellId such that: 0 <= cellId < NumberOfCells. The returned
   * vtkCell is an object owned by this instance, hence the return value must not
   * be deleted by the caller.
   *
   * @warning Repeat calls to this function for different face ids will change
   * the data stored in the internal member object whose pointer is returned by
   * this function.
   *
   * @warning THIS METHOD IS NOT THREAD SAFE. For a thread-safe version, please use
   * void GetCell(vtkIdType cellId, vtkGenericCell* cell).
   */
  virtual vtkCell* GetCell(vtkIdType cellId) = 0;
  virtual vtkCell* GetCell(int vtkNotUsed(i), int vtkNotUsed(j), int vtkNotUsed(k))
  {
    vtkErrorMacro("ijk indices are only valid with structured data!");
    return nullptr;
  }

  void SetCellOrderAndRationalWeights(vtkIdType cellId, vtkGenericCell* cell);

  /**
   * Get cell with cellId such that: 0 <= cellId < NumberOfCells.
   * This is a thread-safe alternative to the previous GetCell()
   * method.
   * THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
   * THE DATASET IS NOT MODIFIED
   */
  virtual void GetCell(vtkIdType cellId, vtkGenericCell* cell) = 0;

  /**
   * Get the bounds of the cell with cellId such that:
   * 0 <= cellId < NumberOfCells.
   * A subclass may be able to determine the bounds of cell without using
   * an expensive GetCell() method. A default implementation is provided
   * that actually uses a GetCell() call.  This is to ensure the method
   * is available to all datasets.  Subclasses should override this method
   * to provide an efficient implementation.
   * THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
   * THE DATASET IS NOT MODIFIED
   */
  virtual void GetCellBounds(vtkIdType cellId, double bounds[6]);

  /**
   * Get type of cell with cellId such that: 0 <= cellId < NumberOfCells.
   * THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
   * THE DATASET IS NOT MODIFIED
   */
  virtual int GetCellType(vtkIdType cellId) = 0;

  /**
   * Get the size of cell with cellId such that: 0 <= cellId < NumberOfCells.
   * THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
   * THE DATASET IS NOT MODIFIED
   *
   * @warning This method MUST be overridden for performance reasons.
   * Default implementation is very inefficient.
   */
  virtual vtkIdType GetCellSize(vtkIdType cellId);

  /**
   * Get a list of types of cells in a dataset. The list consists of an array
   * of types (not necessarily in any order), with a single entry per type.
   * For example a dataset 5 triangles, 3 lines, and 100 hexahedra would
   * result a list of three entries, corresponding to the types VTK_TRIANGLE,
   * VTK_LINE, and VTK_HEXAHEDRON.
   * THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
   * THE DATASET IS NOT MODIFIED
   */
  virtual void GetCellTypes(vtkCellTypes* types);

  /**
   * Topological inquiry to get points defining cell.
   * THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
   * THE DATASET IS NOT MODIFIED
   */
  virtual void GetCellPoints(vtkIdType cellId, vtkIdList* ptIds) = 0;

  /**
   * Topological inquiry to get points defining cell.
   *
   * This function MAY use ptIds, which is an object that is created by each thread,
   * to guarantee thread safety.
   *
   * @warning Subsequent calls to this method may invalidate previous call
   * results.
   *
   * THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
   * THE DATASET IS NOT MODIFIED
   */
  virtual void GetCellPoints(vtkIdType cellId, vtkIdType& npts, vtkIdType const*& pts,
    vtkIdList* ptIds) VTK_SIZEHINT(pts, npts);

  /**
   * Topological inquiry to get cells using point.
   * THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
   * THE DATASET IS NOT MODIFIED
   */
  virtual void GetPointCells(vtkIdType ptId, vtkIdList* cellIds) = 0;

  /**
   * Topological inquiry to get all cells using list of points exclusive of
   * cell specified (e.g., cellId). Note that the list consists of only
   * cells that use ALL the points provided.
   * THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
   * THE DATASET IS NOT MODIFIED
   */
  virtual void GetCellNeighbors(vtkIdType cellId, vtkIdList* ptIds, vtkIdList* cellIds);

  /**
   * Get the number of faces of a cell.
   *
   * Most of the times extracting the number of faces requires only extracting
   * the cell type. However, for some cell types, the number of faces is not
   * constant. For example, a convex point set cell can have a different number of
   * faces for each cell. That's why this method requires the cell id and the dataset.
   */
  virtual int GetCellNumberOfFaces(vtkIdType cellId, unsigned char& cellType, vtkGenericCell* cell);

  ///@{
  /**
   * Locate the closest point to the global coordinate x. Return the
   * point id. If point id < 0; then no point found. (This may arise
   * when point is outside of dataset.)
   * THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
   * THE DATASET IS NOT MODIFIED
   */
  vtkIdType FindPoint(double x, double y, double z)
  {
    double xyz[3];
    xyz[0] = x;
    xyz[1] = y;
    xyz[2] = z;
    return this->FindPoint(xyz);
  }
  virtual vtkIdType FindPoint(double x[3]) = 0;
  ///@}

  /**
   * Locate cell based on global coordinate x and tolerance
   * squared. If cell and cellId is non-nullptr, then search starts from
   * this cell and looks at immediate neighbors.  Returns cellId >= 0
   * if inside, < 0 otherwise.  The parametric coordinates are
   * provided in pcoords[3]. The interpolation weights are returned in
   * weights[]. (The number of weights is equal to the number of
   * points in the found cell). Tolerance is used to control how close
   * the point is to be considered "in" the cell.
   * THIS METHOD IS NOT THREAD SAFE.
   */
  virtual vtkIdType FindCell(double x[3], vtkCell* cell, vtkIdType cellId, double tol2, int& subId,
    double pcoords[3], double* weights) = 0;

  /**
   * This is a version of the above method that can be used with
   * multithreaded applications. A vtkGenericCell must be passed in
   * to be used in internal calls that might be made to GetCell()
   * THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
   * THE DATASET IS NOT MODIFIED
   */
  virtual vtkIdType FindCell(double x[3], vtkCell* cell, vtkGenericCell* gencell, vtkIdType cellId,
    double tol2, int& subId, double pcoords[3], double* weights) = 0;

  /**
   * Locate the cell that contains a point and return the cell. Also returns
   * the subcell id, parametric coordinates and weights for subsequent
   * interpolation. This method combines the derived class methods
   * int FindCell and vtkCell *GetCell. Derived classes may provide a more
   * efficient implementation. See for example vtkStructuredPoints.
   * THIS METHOD IS NOT THREAD SAFE.
   */
  virtual vtkCell* FindAndGetCell(double x[3], vtkCell* cell, vtkIdType cellId, double tol2,
    int& subId, double pcoords[3], double* weights);

  /**
   * Datasets are composite objects and need to check each part for MTime
   * THIS METHOD IS THREAD SAFE
   */
  vtkMTimeType GetMTime() override;

  /**
   * Return a pointer to this dataset's cell data.
   * THIS METHOD IS THREAD SAFE
   */
  vtkCellData* GetCellData() { return this->CellData; }

  /**
   * Return a pointer to this dataset's point data.
   * THIS METHOD IS THREAD SAFE
   */
  vtkPointData* GetPointData() { return this->PointData; }

  /**
   * Reclaim any extra memory used to store data.
   * THIS METHOD IS NOT THREAD SAFE.
   */
  virtual void Squeeze();

  /**
   * Compute the data bounding box from data points.
   * THIS METHOD IS NOT THREAD SAFE.
   */
  virtual void ComputeBounds();

  /**
   * Return a pointer to the geometry bounding box in the form
   * (xmin,xmax, ymin,ymax, zmin,zmax).
   * THIS METHOD IS NOT THREAD SAFE.
   */
  double* GetBounds() VTK_SIZEHINT(6);

  /**
   * Return a pointer to the geometry bounding box in the form
   * (xmin,xmax, ymin,ymax, zmin,zmax).
   * THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
   * THE DATASET IS NOT MODIFIED
   */
  void GetBounds(double bounds[6]);

  /**
   * Get the center of the bounding box.
   * THIS METHOD IS NOT THREAD SAFE.
   */
  double* GetCenter() VTK_SIZEHINT(3);

  /**
   * Get the center of the bounding box.
   * THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
   * THE DATASET IS NOT MODIFIED
   */
  void GetCenter(double center[3]);

  /**
   * Return the length of the diagonal of the bounding box.
   * THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
   * THE DATASET IS NOT MODIFIED
   */
  double GetLength();

  /**
   * Return the squared length of the diagonal of the bounding box.
   * THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
   * THE DATASET IS NOT MODIFIED
   */
  double GetLength2();

  /**
   * Restore data object to initial state.
   * THIS METHOD IS NOT THREAD SAFE.
   */
  void Initialize() override;

  /**
   * Convenience method to get the range of the first component (and only
   * the first component) of any scalars in the data set.  If the data has
   * both point data and cell data, it returns the (min/max) range of
   * combined point and cell data.  If there are no point or cell scalars
   * the method will return (0,1).  Note: It might be necessary to call
   * Update to create or refresh the scalars before calling this method.
   * THIS METHOD IS THREAD SAFE IF FIRST CALLED FROM A SINGLE THREAD AND
   * THE DATASET IS NOT MODIFIED
   */
  virtual void GetScalarRange(double range[2]);

  /**
   * Convenience method to get the range of the first component (and only
   * the first component) of any scalars in the data set.  If the data has
   * both point data and cell data, it returns the (min/max) range of
   * combined point and cell data.  If there are no point or cell scalars
   * the method will return (0,1).  Note: It might be necessary to call
   * Update to create or refresh the scalars before calling this method.
   * THIS METHOD IS NOT THREAD SAFE.
   */
  double* GetScalarRange() VTK_SIZEHINT(2);

  /**
   * Convenience method returns largest cell size in dataset. This is generally
   * used to allocate memory for supporting data structures.
   * THIS METHOD IS THREAD SAFE
   */
  virtual int GetMaxCellSize() = 0;

  ///@{
  /**
   * Get the maximum/minimum spatial dimensionality of the data
   * which is the maximum/minimum dimension of all cells.
   *
   * @warning This method MUST be overridden for performance reasons.
   * Default implementation is very inefficient.
   */
  virtual int GetMaxSpatialDimension();
  virtual int GetMinSpatialDimension();
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

  /**
   * Return the type of data object.
   */
  int GetDataObjectType() VTK_FUTURE_CONST override { return VTK_DATA_SET; }

  ///@{
  /**
   * Shallow and Deep copy.
   */
  void ShallowCopy(vtkDataObject* src) override;
  void DeepCopy(vtkDataObject* src) override;
  ///@}

  enum FieldDataType
  {
    DATA_OBJECT_FIELD = 0,
    POINT_DATA_FIELD = 1,
    CELL_DATA_FIELD = 2
  };

  /**
   * This method checks to see if the cell and point attributes
   * match the geometry.  Many filters will crash if the number of
   * tuples in an array is less than the number of points/cells.
   * This method returns 1 if there is a mismatch,
   * and 0 if everything is ok.  It prints an error if an
   * array is too short, and a warning if an array is too long.
   */
  int CheckAttributes();

  ///@{
  /**
   * Normally called by pipeline executives or algorithms only. This method
   * computes the ghost arrays for a given dataset. The zeroExt argument
   * specifies the extent of the region which ghost type = 0.
   */
  virtual void GenerateGhostArray(int zeroExt[6]) { this->GenerateGhostArray(zeroExt, false); }
  virtual void GenerateGhostArray(int zeroExt[6], bool cellOnly);
  ///@}

  ///@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  static vtkDataSet* GetData(vtkInformation* info);
  static vtkDataSet* GetData(vtkInformationVector* v, int i = 0);
  ///@}

  /**
   * Returns the attributes of the data object as a vtkFieldData.
   * This returns non-null values in all the same cases as GetAttributes,
   * in addition to the case of FIELD, which will return the field data
   * for any vtkDataObject subclass.
   */
  vtkFieldData* GetAttributesAsFieldData(int type) override;

  /**
   * Get the number of elements for a specific attribute type (POINT, CELL, etc.).
   */
  vtkIdType GetNumberOfElements(int type) override;

  /**
   * Abstract method which return the mesh (geometry/topology) modification time.
   * This time is different from the usual MTime which also takes into
   * account the modification of data arrays. This function can be used to
   * track the changes on the mesh separately from the data arrays
   * (eg. static mesh over time with transient data).
   * The default implementation returns the MTime. It is up to subclasses
   * to provide a better approach.
   */
  virtual vtkMTimeType GetMeshMTime();

  /**
   * Returns 1 if there are any ghost cells
   * 0 otherwise.
   */
  bool HasAnyGhostCells();
  /**
   * Returns 1 if there are any ghost points
   * 0 otherwise.
   */
  bool HasAnyGhostPoints();
  /**
   * Returns 1 if there are any blanking cells
   * 0 otherwise. Blanking is supported only for vtkStructuredGrid
   * and vtkUniformGrid
   */
  virtual bool HasAnyBlankCells() { return false; }
  /**
   * Returns 1 if there are any blanking points
   * 0 otherwise. Blanking is supported only for vtkStructuredGrid
   * and vtkUniformGrid
   */
  virtual bool HasAnyBlankPoints() { return false; }

  /**
   * Gets the array that defines the ghost type of each point.
   * We cache the pointer to the array to save a lookup involving string comparisons
   */
  vtkUnsignedCharArray* GetPointGhostArray();

  /**
   * Allocate ghost array for points.
   */
  vtkUnsignedCharArray* AllocatePointGhostArray();

  /**
   * Get the array that defines the ghost type of each cell.
   * We cache the pointer to the array to save a lookup involving string comparisons
   */
  vtkUnsignedCharArray* GetCellGhostArray();

  /**
   * Allocate ghost array for cells.
   */
  vtkUnsignedCharArray* AllocateCellGhostArray();
  /**
   * Returns the ghost array for the given type (point or cell).
   * Takes advantage of the cache with the pointer to the array to save a string
   * comparison.
   */
  vtkUnsignedCharArray* GetGhostArray(int type) override;

  /**
   * Returns true for POINT or CELL, false otherwise
   */
  bool SupportsGhostArray(int type) override;

protected:
  // Constructor with default bounds (0,1, 0,1, 0,1).
  vtkDataSet();
  ~vtkDataSet() override;

  vtkNew<vtkGenericCell> GenericCell; // used by GetCell()

  /**
   * Return the MTime of the ghost cells array.
   * Return 0 if no such array.
   */
  vtkMTimeType GetGhostCellsTime();

  /**
   * Compute the range of the scalars and cache it into ScalarRange
   * only if the cache became invalid (ScalarRangeComputeTime).
   */
  virtual void ComputeScalarRange();

  vtkCellData* CellData;            // Scalars, vectors, etc. associated w/ each cell
  vtkPointData* PointData;          // Scalars, vectors, etc. associated w/ each point
  vtkCallbackCommand* DataObserver; // Observes changes to cell/point data
  vtkTimeStamp ComputeTime;         // Time at which bounds, center, etc. computed
  double Bounds[6];                 // (xmin,xmax, ymin,ymax, zmin,zmax) geometric bounds
  double Center[3];

  // Cached scalar range
  double ScalarRange[2];

  // Time at which scalar range is computed
  vtkTimeStamp ScalarRangeComputeTime;

private:
  void InternalDataSetCopy(vtkDataSet* src);

  // This should only be used if a vtkDataSet subclass don't define GetPoints()
  vtkSmartPointer<vtkPoints> TempPoints;

  vtkDataSet(const vtkDataSet&) = delete;
  void operator=(const vtkDataSet&) = delete;
};

inline void vtkDataSet::GetPoint(vtkIdType id, double x[3])
{
  double* pt = this->GetPoint(id);
  x[0] = pt[0];
  x[1] = pt[1];
  x[2] = pt[2];
}

VTK_ABI_NAMESPACE_END
#endif
