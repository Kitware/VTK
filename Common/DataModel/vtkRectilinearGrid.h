// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkRectilinearGrid
 * @brief   a dataset that is topologically regular with variable spacing in the three coordinate
 * directions
 *
 * vtkRectilinearGrid is a data object that is a concrete implementation of
 * vtkCartesianGrid. vtkRectilinearGrid represents a geometric structure that is
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

#include "vtkCartesianGrid.h"
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDeprecation.h"           // for VTK_DEPRECATED_IN_9_6_0
#include "vtkSmartPointer.h"          // For vtkSmartPointer
#include "vtkStructuredData.h"        // For inline methods
#include "vtkWrappingHints.h"         // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
class vtkStructuredCellArray;
class vtkPoints;

class VTKCOMMONDATAMODEL_EXPORT VTK_MARSHALAUTO vtkRectilinearGrid : public vtkCartesianGrid
{
public:
  static vtkRectilinearGrid* New();
  static vtkRectilinearGrid* ExtendedNew();

  vtkTypeMacro(vtkRectilinearGrid, vtkCartesianGrid);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return what type of dataset this is.
   */
  int GetDataObjectType() VTK_FUTURE_CONST override { return VTK_RECTILINEAR_GRID; }

  ///@{
  /**
   * Shallow and Deep copy.
   */
  void ShallowCopy(vtkDataObject* src) override;
  void DeepCopy(vtkDataObject* src) override;
  ///@}

  /**
   * Copy the geometric and topological structure of an input rectilinear grid
   * object.
   */
  void CopyStructure(vtkDataSet* ds) override;

  /**
   * Restore object to initial state. Release memory back to system.
   */
  void Initialize() override;

  using Superclass::FindCell;
  using Superclass::GetCell;
  ///@{
  /**
   * Standard vtkDataSet API methods. See vtkCartesianGrid for more information.
   */
  void GetCell(vtkIdType cellId, vtkGenericCell* cell) override;
  void GetCellBounds(vtkIdType cellId, double bounds[6]) override;
  vtkIdType FindPoint(double x[3]) override;
  vtkIdType FindCell(double x[3], vtkCell* cell, vtkIdType cellId, double tol2, int& subId,
    double pcoords[3], double* weights) override;
  void ComputeBounds() override;
  ///@}

  /**
   * Computes the structured coordinates for a point x[3].
   * The cell is specified by the array ijk[3], and the parametric coordinates
   * in the cell are specified with pcoords[3]. The function returns a 0 if the
   * point x is outside of the grid, and a 1 if inside the grid.
   */
  int ComputeStructuredCoordinates(const double x[3], int ijk[3], double pcoords[3]) override;

  VTK_DEPRECATED_IN_9_6_0("Use the const version instead")
  int ComputeStructuredCoordinates(double x[3], int ijk[3], double pcoords[3])
  {
    const double* pt = x;
    return this->ComputeStructuredCoordinates(pt, ijk, pcoords);
  };

  /**
   * Given a location in structured coordinates (i-j-k), return the point id.
   * Rely on vtkStructuredData::ComputePointId
   * Unifying this behavior with vtkImageData could be considered.
   */
  vtkIdType ComputePointId(int ijk[3]) override;

  /**
   * Given a location in structured coordinates (i-j-k), return the cell id.
   * Rely on vtkStructuredData::ComputeCellId
   * Unifying this behavior with vtkImageData could be considered.
   */
  vtkIdType ComputeCellId(int ijk[3]) override;

  using Superclass::GetPoint;
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

protected:
  vtkRectilinearGrid();
  ~vtkRectilinearGrid() override;

  vtkDataArray* XCoordinates;
  vtkDataArray* YCoordinates;
  vtkDataArray* ZCoordinates;

  void BuildPoints() override;

private:
  void Cleanup();

  vtkRectilinearGrid(const vtkRectilinearGrid&) = delete;
  void operator=(const vtkRectilinearGrid&) = delete;
};

//----------------------------------------------------------------------------
inline vtkIdType vtkRectilinearGrid::ComputePointId(int ijk[3])
{
  return vtkStructuredData::ComputePointId(this->GetDimensions(), ijk);
}

//----------------------------------------------------------------------------
inline vtkIdType vtkRectilinearGrid::ComputeCellId(int ijk[3])
{
  return vtkStructuredData::ComputeCellId(this->GetDimensions(), ijk);
}

VTK_ABI_NAMESPACE_END
#endif
