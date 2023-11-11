// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkStructuredCellArray
 * @brief   implicit object to represent cell connectivity
 *
 * vtkStructuredCellArray stores dataset topologies as an structured connectivity table
 * listing the point ids that make up each cell.
 *
 * Internally, the connectivity is stored as a vtkImplicitArray that is constructed
 * using the SetData function by providing the dimensions of the dataset and a flag
 * indicating whether the data should use voxel/pixel orientation.
 *
 * This class was designed as a more performant alternative to vtkStructuredData::GetCellPoints.
 *
 * @sa
 * vtkCellArray vtkAbstractCellArray
 */
#ifndef vtkStructuredCellArray_h
#define vtkStructuredCellArray_h

#include "vtkAbstractCellArray.h"
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkSmartPointer.h"          // For vtkSmartPointer

VTK_ABI_NAMESPACE_BEGIN

template <typename T>
class vtkImplicitArray;

class VTKCOMMONDATAMODEL_EXPORT vtkStructuredCellArray : public vtkAbstractCellArray
{
public:
  static vtkStructuredCellArray* New();
  vtkTypeMacro(vtkStructuredCellArray, vtkAbstractCellArray);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Free any memory and reset to an empty state.
   */
  void Initialize() override;

  /**
   * Get the number of cells in the array.
   */
  vtkIdType GetNumberOfCells() const override;

  /**
   * Get the number of elements in the offsets array. This will be the number of
   * cells + 1.
   */
  vtkIdType GetNumberOfOffsets() const override;

  /**
   * Get the offset (into the connectivity) for a specified cell id.
   */
  vtkIdType GetOffset(vtkIdType cellId) override;

  /**
   * Get the size of the connectivity array that stores the point ids.
   * @note Do not confuse this with the deprecated
   * GetNumberOfConnectivityEntries(), which refers to the legacy memory
   * layout.
   */
  vtkIdType GetNumberOfConnectivityIds() const override;

  /**
   * Create a new cell array given extent and a flag indicating whether
   * the data should be stored in a voxel or pixel orientation.
   */
  void SetData(int extent[6], bool usePixelVoxelOrientation);

  /**
   * @return True if the internal storage can be shared as a
   * pointer to vtkIdType, i.e., the type and organization of internal
   * storage is such that copying of data can be avoided, and instead
   * a pointer to vtkIdType can be used.
   */
  bool IsStorageShareable() const override { return false; }

  /**
   * Check if all cells have the same number of vertices.
   *
   * The return value is coded as:
   * * -1 = heterogeneous
   * * 0 = Cell array empty
   * * n (positive integer) = homogeneous array of cell size n
   */
  vtkIdType IsHomogeneous() override;

  /**
   * Return the point ids for the cell at @a cellId.
   *
   * Subsequent calls to this method may invalidate previous call
   * results if the internal storage type is not the same as vtkIdType and
   * cannot be shared through the @a cellPoints pointer. If that occurs,
   * the method will use ptIds, which is an object that is created by each thread,
   * to guarantee thread safety.
   */
  void GetCellAtId(vtkIdType cellId, vtkIdType& cellSize, vtkIdType const*& cellPoints,
    vtkIdList* ptIds) VTK_SIZEHINT(cellPoints, cellSize)
    VTK_EXPECTS(0 <= cellId && cellId < GetNumberOfCells()) override;

  /**
   * Return the point ids for the cell at @a cellId. This always copies
   * the cell ids (i.e., the list of points @a pts into the supplied
   * vtkIdList). This method is thread safe.
   */
  void GetCellAtId(vtkIdType cellId, vtkIdList* ptIds)
    VTK_EXPECTS(0 <= cellId && cellId < GetNumberOfCells()) override;

  /**
   * Return the point ids for the cell at @a cellId. This always copies
   * the cell ids into cellSize and cellPoints. This method is thread safe.
   *
   * Note: the cellPoints need to have the correct size already allocated otherwise memory
   * issues can occur.
   */
  void GetCellAtId(vtkIdType cellId, vtkIdType& cellSize, vtkIdType* cellPoints) VTK_SIZEHINT(
    cellPoints, cellSize) VTK_EXPECTS(0 <= cellId && cellId < GetNumberOfCells()) override;

  /**
   * Return the point ids for the cell at @a ijk. This always copies
   * the cell ids (i.e., the list of points @a pts into the supplied
   * vtkIdList). This method is thread safe.
   */
  void GetCellAtId(int ijk[3], vtkIdList* ptIds);

  /**
   * Return the point ids for the cell at @a ijk. This always copies
   * the cell ids into cellSize cellPoints. This method is thread safe.
   *
   * Note: the cellPoints need to have the correct size already allocated otherwise memory
   * issues can occur.
   */
  void GetCellAtId(int ijk[3], vtkIdType& cellSize, vtkIdType* cellPoints);

  /**
   * Return the size of the cell at @a cellId.
   */
  vtkIdType GetCellSize(vtkIdType cellId) const override;

  /**
   * Returns the size of the largest cell. The size is the number of points
   * defining the cell.
   */
  int GetMaxCellSize() override;

  /**
   * Perform a deep copy (no reference counting) of the given cell array.
   */
  void DeepCopy(vtkAbstractCellArray* ca) override;

  /**
   * Shallow copy @a ca into this cell array.
   */
  void ShallowCopy(vtkAbstractCellArray* ca) override;

protected:
  vtkStructuredCellArray();
  ~vtkStructuredCellArray() override;

  /**
   * Implicit cell back end for vtkStructuredCellArray.
   */
  struct vtkStructuredCellBackend;
  template <int DataDescription, bool UsePixelVoxelOrientation>
  struct vtkStructuredTCellBackend;
  vtkSmartPointer<vtkImplicitArray<vtkStructuredCellBackend>> Connectivity;

private:
  vtkStructuredCellArray(const vtkStructuredCellArray&) = delete;
  void operator=(const vtkStructuredCellArray&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkStructuredCellArray_h
