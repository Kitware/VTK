// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME vtkBridgeCellIteratorStrategy - Interface used by vtkBridgeCellIterator
// vtkBridgeCellIterator has different behaviors depending on the way it is
// initialized. vtkBridgeCellIteratorStrategy is the interface for one of those
// behaviors. Concrete classes are vtkBridgeCellIteratorOnDataSet,
// vtkBridgeCellIteratorOnDataSetBoundaries,
// vtkBridgeCellIteratorOnCellBoundaries,
// vtkBridgeCellIteratorOnCellNeighbors,
// .SECTION See Also
// vtkCellIterator, vtkBridgeCellIterator, vtkBridgeDataSet, vtkBridgeCellIteratorOnDataSet,
// vtkBridgeCellIteratorOnDataSetBoundaries, vtkBridgeCellIteratorOnCellBoundaries,
// vtkBridgeCellIteratorOnCellNeighbors

#include "vtkBridgeCellIteratorStrategy.h"

#include <cassert>

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
void vtkBridgeCellIteratorStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
// Description:
// Create an empty cell. NOT USED
// \post result_exists: result!=0
vtkGenericAdaptorCell* vtkBridgeCellIteratorStrategy::NewCell()
{
  assert("check: should not be called: see vtkBridgeCellIterator::NewCell()" && 0);
  return nullptr;
}
VTK_ABI_NAMESPACE_END
