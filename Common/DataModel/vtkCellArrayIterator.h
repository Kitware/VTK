/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellArrayIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkCellArrayIterator
 * @brief   Encapsulate traversal logic for vtkCellArray.
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

/**
 * @brief The vtkCellArrayIterator class provides thread-safe iteration of a
 * vtkCellArray.
 *
 * See the vtkCellArray class documentation for more details.
 */
class VTKCOMMONDATAMODEL_EXPORT vtkCellArrayIterator : public vtkObject
{
public:
  vtkTypeMacro(vtkCellArrayIterator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkCellArrayIterator* New();

  /** The vtkCellArray object being iterated. */
  vtkCellArray* GetCellArray() { return this->CellArray; }

  /**
   * Initialize the iterator. This will revalidate the iterator if the
   * underlying vtkCellArray has been modified.
   */
  void GoToFirstCell()
  {
    this->CurrentCellId = 0;
    this->NumberOfCells = this->CellArray->GetNumberOfCells();
  }

  /** Advance the iterator to the next cell. */
  void GoToNextCell() { ++this->CurrentCellId; }

  /**
   * Intialize the iterator to a specific cell. This will revalidate the
   * iterator if the underlying vtkCellArray has been modified.
   */
  void GoToCell(vtkIdType cellId)
  {
    this->CurrentCellId = cellId;
    this->NumberOfCells = this->CellArray->GetNumberOfCells();
    assert(cellId <= this->NumberOfCells);
  }

  /**
   * Returns true if the iterator has completed the traversal.
   */
  bool IsDoneWithTraversal() { return this->CurrentCellId >= this->NumberOfCells; }

  /** Returns the id of the current cell. */
  vtkIdType GetCurrentCellId() const { return this->CurrentCellId; }

  /**
   * Returns the definition of the current cell. Note that cellPoints is a
   * ref-to-const-pointer, and should be used as:
   *
   * ```
   * vtkIdType cellSize;
   * const vtkIdType *cellPoints;
   * iter->GetCurrentCell(cellSize, cellPoints):
   * ```
   *
   * @warning Subsequent calls to this method may invalidate previous call
   * results.
   */
  void GetCurrentCell(vtkIdType& cellSize, vtkIdType const*& cellPoints)
  {
    assert(this->CurrentCellId < this->NumberOfCells);
    // The vtkCellArray::GetCellAtId that returns a pointer is not thread-safe,
    // since it may refer to an internal buffer. Use our internal buffer
    // instead, which won't be clobbered by another thread.
    this->CellArray->GetCellAtId(this->CurrentCellId, this->TempCell);
    cellSize = this->TempCell->GetNumberOfIds();
    cellPoints = this->TempCell->GetPointer(0);
  }

  /** Returns the definition of the current cell. */
  void GetCurrentCell(vtkIdList* ids)
  {
    assert(this->CurrentCellId < this->NumberOfCells);
    this->CellArray->GetCellAtId(this->CurrentCellId, ids);
  }

  /** Returns the definition of the current cell. */
  vtkIdList* GetCurrentCell()
  {
    assert(this->CurrentCellId < this->NumberOfCells);
    this->CellArray->GetCellAtId(this->CurrentCellId, this->TempCell);
    return this->TempCell;
  }

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

#endif // vtkCellArrayIterator_h
