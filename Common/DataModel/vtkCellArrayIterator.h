// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkCellArrayIterator
 * @brief   Encapsulate traversal logic for vtkCellArray.
 *
 * This is iterator for thread-safe traversal of a vtkCellArray. It provides
 * random access and forward iteration. Typical usage for forward iteration
 * looks like:
 *
 * ```
 * auto iter = vtk::TakeSmartPointer(cellArray->NewIterator());
 * for (iter->GoToFirstCell(); !iter->IsDoneWithTraversal(); iter->GoToNextCell())
 * {
 *   // do work with iter
 *   iter->GetCurrentCell(numCellPts, cellPts);
 * }
 * ```
 *
 * Typical usage for random access looks like:
 *
 * ```
 * auto iter = vtk::TakeSmartPointer(cellArray->NewIterator());
 * iter->GetCellAtId(cellId, numCellPts, cellPts);
 * ```
 *
 * Here @a cellId is the id of the ith cell in the vtkCellArray;
 * @a numCellPts is the number of points defining the cell represented
 * as vtkIdType; and @a cellPts is a pointer to the point ids defined
 * as vtkIdType const*&.
 *
 * Internally the iterator may copy data from the vtkCellArray, or reference
 * the internal vtkCellArray storage. This depends on the relationship of
 * vtkIdType to the type and structure of internal storage. If the type of
 * storage is the same as vtkIdType, and the storage is a single-component
 * AOS array (i.e., a 1D array), then shared access to the vtkCellArray
 * storage is provided. Otherwise, the data from storage is copied into an
 * internal iterator buffer. (Of course copying is slower and can result in
 * 3-4x reduction in traversal performance. On the other hand, the
 * vtkCellArray can use the appropriate storage to save memory, perform
 * zero-copy, and/or efficiently represent the cell connectivity
 * information.) Note that referencing internal vtkCellArray storage has
 * implications on the validity of the iterator. If the underlying
 * vtkCellArray storage changes while iterating, and the iterator is
 * referencing this storage, unpredictable and catastrophic results are
 * likely - hence do not modify the vtkCellArray while iterating.
 *
 * @sa
 * vtkCellArray
 */

#ifndef vtkCellArrayIterator_h
#define vtkCellArrayIterator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

#include "vtkCellArray.h"    // Needed for inline methods
#include "vtkIdList.h"       // Needed for inline methods
#include "vtkSmartPointer.h" // For vtkSmartPointer

#include <cassert>     // for assert
#include <type_traits> // for std::enable_if

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONDATAMODEL_EXPORT vtkCellArrayIterator : public vtkObject
{
public:
  ///@{
  /**
   * Standard methods for instantiation, type information, and printing.
   */
  vtkTypeMacro(vtkCellArrayIterator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkCellArrayIterator* New();
  ///@}

  /**
   * Return the vtkCellArray object over which iteration is occurring.
   */
  vtkCellArray* GetCellArray() { return this->CellArray; }

  /**
   * Initialize the iterator to a specific cell. This will revalidate the
   * iterator if the underlying vtkCellArray has been modified. This method
   * can always be used to set the starting location for forward iteration,
   * and it is also used to support random access.
   */
  void GoToCell(vtkIdType cellId)
  {
    this->CurrentCellId = cellId;
    this->NumberOfCells = this->CellArray->GetNumberOfCells();
    assert(cellId <= this->NumberOfCells);
  }

  /**
   * The following are methods supporting random access iteration.
   */

  ///@{
  /**
   * Initialize the iterator to a specific cell and return the cell. Note
   * that methods passing vtkIdLists always copy data from the vtkCellArray
   * storage buffer into the vtkIdList. Otherwise, a fastpath returning
   * (numCellPts,cellPts) which may return a pointer to internal vtkCellArray
   * storage is possible, if vtkIdType is the same as the vtkCellArray buffer
   * (which is typical).
   */
  void GetCellAtId(vtkIdType cellId, vtkIdType& numCellPts, vtkIdType const*& cellPts)
  {
    this->GoToCell(cellId);
    this->GetCurrentCell(numCellPts, cellPts);
  }
  void GetCellAtId(vtkIdType cellId, vtkIdList* cellIds)
  {
    this->GoToCell(cellId);
    this->GetCurrentCell(cellIds);
  }
  vtkIdList* GetCellAtId(vtkIdType cellId)
  {
    this->GoToCell(cellId);
    return this->GetCurrentCell();
  }
  ///@}

  /**
   * The following are methods supporting forward iteration.
   */

  /**
   * Initialize the iterator for forward iteration. This will revalidate the
   * iterator if the underlying vtkCellArray has been modified.
   */
  void GoToFirstCell()
  {
    this->CurrentCellId = 0;
    this->NumberOfCells = this->CellArray->GetNumberOfCells();
  }

  /**
   * Advance the forward iterator to the next cell.
   */
  void GoToNextCell() { ++this->CurrentCellId; }

  /**
   * Returns true if the iterator has completed the traversal.
   */
  bool IsDoneWithTraversal() { return this->CurrentCellId >= this->NumberOfCells; }

  /**
   * Returns the id of the current cell during forward iteration.
   */
  vtkIdType GetCurrentCellId() const { return this->CurrentCellId; }

  ///@{
  /**
   * Returns the definition of the current cell during forward
   * traversal. Note that methods passing vtkIdLists always copy data from
   * the vtkCellArray storage buffer into the vtkIdList. Otherwise, a
   * fastpath returning (numCellPts,cellPts) - which may return a pointer to
   * internal vtkCellArray storage - is possible, if vtkIdType is the same as
   * the vtkCellArray storage (which is typical).
   */
  void GetCurrentCell(vtkIdType& cellSize, vtkIdType const*& cellPoints)
  {
    assert(this->CurrentCellId < this->NumberOfCells);
    // Either refer to vtkCellArray storage buffer, or copy into local buffer
    if (this->CellArray->IsStorageShareable())
    {
      this->CellArray->GetCellAtId(this->CurrentCellId, cellSize, cellPoints);
    }
    else // or copy into local iterator buffer.
    {
      this->CellArray->GetCellAtId(this->CurrentCellId, this->TempCell);
      cellSize = this->TempCell->GetNumberOfIds();
      cellPoints = this->TempCell->GetPointer(0);
    }
  }
  void GetCurrentCell(vtkIdList* ids)
  {
    assert(this->CurrentCellId < this->NumberOfCells);
    this->CellArray->GetCellAtId(this->CurrentCellId, ids);
  }
  vtkIdList* GetCurrentCell()
  {
    assert(this->CurrentCellId < this->NumberOfCells);
    this->CellArray->GetCellAtId(this->CurrentCellId, this->TempCell);
    return this->TempCell;
  }
  ///@}

  /**
   * Specialized methods for performing operations on the vtkCellArray.
   */

  /**
   * Replace the current cell with the ids in `list`. Note that this method
   * CANNOT change the number of points in the cell, it can only redefine the
   * ids (e.g. `list` must contain the same number of entries as the current
   * cell's points).
   */
  void ReplaceCurrentCell(vtkIdList* list)
  {
    assert(this->CurrentCellId < this->NumberOfCells);
    this->CellArray->ReplaceCellAtId(this->CurrentCellId, list);
  }

  /**
   * Replace the current cell with the ids in `pts`. Note that this method
   * CANNOT change the number of points in the cell, it can only redefine the
   * ids (e.g. `npts` must equal the current cell's number of points).
   */
  void ReplaceCurrentCell(vtkIdType npts, const vtkIdType* pts)
  {
    assert(this->CurrentCellId < this->NumberOfCells);
    this->CellArray->ReplaceCellAtId(this->CurrentCellId, npts, pts);
  }

  /**
   * Reverses the order of the point ids in the current cell.
   */
  void ReverseCurrentCell()
  {
    assert(this->CurrentCellId < this->NumberOfCells);
    this->CellArray->ReverseCellAtId(this->CurrentCellId);
  }

  friend class vtkCellArray;

protected:
  vtkCellArrayIterator() = default;
  ~vtkCellArrayIterator() override = default;

  vtkSetMacro(CellArray, vtkCellArray*);

  vtkSmartPointer<vtkCellArray> CellArray;
  vtkNew<vtkIdList> TempCell;
  vtkIdType CurrentCellId;
  vtkIdType NumberOfCells;

private:
  vtkCellArrayIterator(const vtkCellArrayIterator&) = delete;
  void operator=(const vtkCellArrayIterator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCellArrayIterator_h
