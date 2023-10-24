// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkUnstructuredGridCellIterator
 * @brief   Implementation of vtkCellIterator
 * specialized for vtkUnstructuredGrid.
 */

#ifndef vtkUnstructuredGridCellIterator_h
#define vtkUnstructuredGridCellIterator_h

#include "vtkCellArrayIterator.h" // Accessing cell array
#include "vtkCellIterator.h"
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkSmartPointer.h"          // For vtkSmartPointer

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArray;
class vtkIdTypeArray;
class vtkUnsignedCharArray;
class vtkUnstructuredGrid;
class vtkPoints;

class VTKCOMMONDATAMODEL_EXPORT vtkUnstructuredGridCellIterator : public vtkCellIterator
{
public:
  ///@{
  /**
   * Standard methods for instantiation, type information, and printing.
   */
  static vtkUnstructuredGridCellIterator* New();
  vtkTypeMacro(vtkUnstructuredGridCellIterator, vtkCellIterator);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Override superclass methods.
   */
  bool IsDoneWithTraversal() override;
  vtkIdType GetCellId() override;
  ///@}

  /**
   * A method used to provide random access into cells. The iterator is
   * initialized to a specific cell. This method can always be used to set
   * the starting location for forward iteration, and it is also used to
   * support random access.
   */
  void GoToCell(vtkIdType cellId) { this->Cells->GoToCell(cellId); }

protected:
  vtkUnstructuredGridCellIterator();
  ~vtkUnstructuredGridCellIterator() override;

  void ResetToFirstCell() override;
  void IncrementToNextCell() override;
  void FetchCellType() override;
  void FetchPointIds() override;
  void FetchPoints() override;
  void FetchFaces() override;

  friend class vtkUnstructuredGrid;
  void SetUnstructuredGrid(vtkUnstructuredGrid* ug);

  vtkSmartPointer<vtkCellArrayIterator> Cells;
  vtkSmartPointer<vtkUnsignedCharArray> Types;
  vtkSmartPointer<vtkCellArray> PolyFaceConn;
  vtkSmartPointer<vtkCellArray> PolyFaceLocs;
  vtkSmartPointer<vtkPoints> Coords;

private:
  vtkUnstructuredGridCellIterator(const vtkUnstructuredGridCellIterator&) = delete;
  void operator=(const vtkUnstructuredGridCellIterator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkUnstructuredGridCellIterator_h
