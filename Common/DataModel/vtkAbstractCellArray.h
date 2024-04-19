// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAbstractCellArray
 * @brief   abstract object to represent cell connectivity
 *
 * vtkAbstractCellArray is an abstract base class for storing a connectivity table
 * listing the point ids that make up each cell.
 *
 * @sa
 * vtkCellArray vtkStructuredCellArray
 */
#ifndef vtkAbstractCellArray_h
#define vtkAbstractCellArray_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNew.h"                   // for vtkNew
#include "vtkObject.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

#include <initializer_list> // for std::initializer_list

VTK_ABI_NAMESPACE_BEGIN

class vtkDataArray;
class vtkIdList;

class VTKCOMMONDATAMODEL_EXPORT VTK_MARSHALAUTO vtkAbstractCellArray : public vtkObject
{
public:
  vtkTypeMacro(vtkAbstractCellArray, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Free any memory and reset to an empty state.
   */
  virtual void Initialize() = 0;

  /**
   * Get the number of cells in the array.
   */
  virtual vtkIdType GetNumberOfCells() const = 0;

  /**
   * Get the number of elements in the offsets array. This will be the number of
   * cells + 1.
   */
  virtual vtkIdType GetNumberOfOffsets() const = 0;

  /**
   * Get the offset (into the connectivity) for a specified cell id.
   */
  virtual vtkIdType GetOffset(vtkIdType cellId) = 0;

  /**
   * Get the size of the connectivity array that stores the point ids.
   * @note Do not confuse this with the deprecated
   * GetNumberOfConnectivityEntries(), which refers to the legacy memory
   * layout.
   */
  virtual vtkIdType GetNumberOfConnectivityIds() const = 0;

  /**
   * @return True if the internal storage can be shared as a
   * pointer to vtkIdType, i.e., the type and organization of internal
   * storage is such that copying of data can be avoided, and instead
   * a pointer to vtkIdType can be used.
   */
  virtual bool IsStorageShareable() const = 0;

  /**
   * Check if all cells have the same number of vertices.
   *
   * The return value is coded as:
   * * -1 = heterogeneous
   * * 0 = Cell array empty
   * * n (positive integer) = homogeneous array of cell size n
   */
  virtual vtkIdType IsHomogeneous() = 0;

  /**
   * Return the point ids for the cell at @a cellId.
   *
   * @warning Subsequent calls to this method may invalidate previous call
   * results if the internal storage type is not the same as vtkIdType and
   * cannot be shared through the @a cellPoints pointer. In other words, the
   * method may not be thread safe. Check if shareable (using
   * IsStorageShareable()), or use a vtkCellArrayIterator to guarantee thread
   * safety.
   */
  void GetCellAtId(vtkIdType cellId, vtkIdType& cellSize, vtkIdType const*& cellPoints)
    VTK_SIZEHINT(cellPoints, cellSize) VTK_EXPECTS(0 <= cellId && cellId < GetNumberOfCells());

  /**
   * Return the point ids for the cell at @a cellId.
   *
   * Subsequent calls to this method may invalidate previous call
   * results if the internal storage type is not the same as vtkIdType and
   * cannot be shared through the @a cellPoints pointer. If that occurs,
   * the method will use ptIds, which is an object that is created by each thread,
   * to guarantee thread safety.
   */
  virtual void GetCellAtId(
    vtkIdType cellId, vtkIdType& cellSize, vtkIdType const*& cellPoints, vtkIdList* ptIds)
    VTK_SIZEHINT(cellPoints, cellSize) VTK_EXPECTS(0 <= cellId && cellId < GetNumberOfCells()) = 0;

  /**
   * Return the point ids for the cell at @a cellId. This always copies
   * the cell ids (i.e., the list of points @a pts into the supplied
   * vtkIdList). This method is thread safe.
   */
  virtual void GetCellAtId(vtkIdType cellId, vtkIdList* pts)
    VTK_EXPECTS(0 <= cellId && cellId < GetNumberOfCells()) = 0;

  /**
   * Return the point ids for the cell at @a cellId. This always copies
   * the cell ids into cellSize and cellPoints. This method is thread safe.
   *
   * Note: the cellPoints need to have the correct size already allocated otherwise memory
   * issues can occur.
   */
  virtual void GetCellAtId(vtkIdType cellId, vtkIdType& cellSize, vtkIdType* cellPoints)
    VTK_SIZEHINT(cellPoints, cellSize) VTK_EXPECTS(0 <= cellId && cellId < GetNumberOfCells()) = 0;

  /**
   * Return the size of the cell at @a cellId.
   */
  virtual vtkIdType GetCellSize(vtkIdType cellId) const = 0;

  /**
   * Returns the size of the largest cell. The size is the number of points
   * defining the cell.
   */
  virtual int GetMaxCellSize() = 0;

  /**
   * Perform a deep copy (no reference counting) of the given cell array.
   */
  virtual void DeepCopy(vtkAbstractCellArray* ca) = 0;

  /**
   * Shallow copy @a ca into this cell array.
   */
  virtual void ShallowCopy(vtkAbstractCellArray* ca) = 0;

protected:
  vtkAbstractCellArray();
  ~vtkAbstractCellArray() override;

  vtkNew<vtkIdList> TempCell;

private:
  vtkAbstractCellArray(const vtkAbstractCellArray&) = delete;
  void operator=(const vtkAbstractCellArray&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkAbstractCellArray_h
