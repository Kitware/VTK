//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtkmDataSet_h
#define vtkmDataSet_h

#include "vtkAcceleratorsVTKmModule.h" // For export macro
#include "vtkDataSet.h"

#include <memory> // for std::shared_ptr

namespace vtkm
{
namespace cont
{

class DataSet;

}
} // vtkm::cont

class vtkPoints;
class vtkCell;
class vtkGenericCell;

class VTKACCELERATORSVTKM_EXPORT vtkmDataSet : public vtkDataSet
{
public:
  vtkTypeMacro(vtkmDataSet, vtkDataSet);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkmDataSet* New();

  void SetVtkmDataSet(const vtkm::cont::DataSet& ds);
  vtkm::cont::DataSet GetVtkmDataSet() const;

  /**
   * Copy the geometric and topological structure of an object. Note that
   * the invoking object and the object pointed to by the parameter ds must
   * be of the same type.
   */
  void CopyStructure(vtkDataSet* ds) override;

  /**
   * Determine the number of points composing the dataset.
   */
  vtkIdType GetNumberOfPoints() override;

  /**
   * Determine the number of cells composing the dataset.
   */
  vtkIdType GetNumberOfCells() override;

  /**
   * Get point coordinates with ptId such that: 0 <= ptId < NumberOfPoints.
   */
  double* GetPoint(vtkIdType ptId) VTK_SIZEHINT(3) override;

  /**
   * Copy point coordinates into user provided array x[3] for specified
   * point id.
   */
  void GetPoint(vtkIdType id, double x[3]) override;

  using vtkDataSet::GetCell;
  /**
   * Get cell with cellId such that: 0 <= cellId < NumberOfCells.
   */
  vtkCell* GetCell(vtkIdType cellId) override;
  void GetCell(vtkIdType cellId, vtkGenericCell* cell) override;

  /**
   * Get the bounds of the cell with cellId such that:
   * 0 <= cellId < NumberOfCells.
   */
  void GetCellBounds(vtkIdType cellId, double bounds[6]) override;

  /**
   * Get type of cell with cellId such that: 0 <= cellId < NumberOfCells.
   */
  int GetCellType(vtkIdType cellId) override;

  /**
   * Topological inquiry to get points defining cell.
   */
  void GetCellPoints(vtkIdType cellId, vtkIdList* ptIds) override;

  /**
   * Topological inquiry to get cells using point.
   */
  void GetPointCells(vtkIdType ptId, vtkIdList* cellIds) override;

  //@{
  /**
   * Locate the closest point to the global coordinate x. Return the
   * point id. If point id < 0; then no point found. (This may arise
   * when point is outside of dataset.)
   */
  vtkIdType FindPoint(double x[3]) override;
  //@}

  /**
   * Locate cell based on global coordinate x and tolerance
   * squared. If cell and cellId is non-nullptr, then search starts from
   * this cell and looks at immediate neighbors.  Returns cellId >= 0
   * if inside, < 0 otherwise.  The parametric coordinates are
   * provided in pcoords[3]. The interpolation weights are returned in
   * weights[]. (The number of weights is equal to the number of
   * points in the found cell). Tolerance is used to control how close
   * the point is to be considered "in" the cell.
   */
  vtkIdType FindCell(double x[3], vtkCell* cell, vtkIdType cellId, double tol2, int& subId,
    double pcoords[3], double* weights) override;

  /**
   * This is a version of the above method that can be used with
   * multithreaded applications. A vtkGenericCell must be passed in
   * to be used in internal calls that might be made to GetCell()
   */
  vtkIdType FindCell(double x[3], vtkCell* cell, vtkGenericCell* gencell, vtkIdType cellId,
    double tol2, int& subId, double pcoords[3], double* weights) override;

  /**
   * Reclaim any extra memory used to store data.
   */
  void Squeeze() override;

  /**
   * Compute the data bounding box from data points.
   */
  void ComputeBounds() override;

  /**
   * Restore data object to initial state.
   * THIS METHOD IS NOT THREAD SAFE.
   */
  void Initialize() override;

  /**
   * Convenience method returns largest cell size in dataset. This is generally
   * used to allocate memory for supporting data structures.
   */
  int GetMaxCellSize() override;

  /**
   * Return the actual size of the data in kibibytes (1024 bytes). This number
   * is valid only after the pipeline has updated. The memory size
   * returned is guaranteed to be greater than or equal to the
   * memory required to represent the data (e.g., extra space in
   * arrays, etc. are not included in the return value).
   */
  unsigned long GetActualMemorySize() override;

  /**
   * Return the type of data object.
   */
  int GetDataObjectType() override { return VTK_DATA_SET; }

  //@{
  /**
   * Shallow and Deep copy.
   */
  void ShallowCopy(vtkDataObject* src) override;
  void DeepCopy(vtkDataObject* src) override;
  //@}

protected:
  vtkmDataSet();
  ~vtkmDataSet() override;

private:
  vtkmDataSet(const vtkmDataSet&) = delete;
  void operator=(const vtkmDataSet&) = delete;

  struct DataMembers;
  std::shared_ptr<DataMembers> Internals;
};

#endif // vtkmDataSet_h
// VTK-HeaderTest-Exclude: vtkmDataSet.h
