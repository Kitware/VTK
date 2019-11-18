/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBridgeCellIteratorOnCellList.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBridgeCellIteratorOnCellList - Iterate over cells of a dataset.
// .SECTION See Also
// vtkBridgeCellIterator, vtkBridgeDataSet, vtkBridgeCellIteratorStrategy

#include "vtkBridgeCellIteratorOnCellList.h"

#include <cassert>

#include "vtkBridgeCell.h"
#include "vtkBridgeDataSet.h"
#include "vtkCell.h"
#include "vtkDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkVertex.h"

vtkStandardNewMacro(vtkBridgeCellIteratorOnCellList);

//-----------------------------------------------------------------------------
void vtkBridgeCellIteratorOnCellList::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
vtkBridgeCellIteratorOnCellList::vtkBridgeCellIteratorOnCellList()
{
  this->DataSet = nullptr;
  this->Cells = nullptr;
  this->Cell = vtkBridgeCell::New();
  this->Id = 0;
  //  this->DebugOn();
}

//-----------------------------------------------------------------------------
vtkBridgeCellIteratorOnCellList::~vtkBridgeCellIteratorOnCellList()
{
  if (this->DataSet != nullptr)
  {
    this->DataSet->Delete();
    this->DataSet = nullptr;
  }

  if (this->Cells != nullptr)
  {
    this->Cells->Delete();
    this->Cells = nullptr;
  }

  this->Cell->Delete();
  this->Cell = nullptr;
}

//-----------------------------------------------------------------------------
// Description:
// Move iterator to first position if any (loop initialization).
void vtkBridgeCellIteratorOnCellList::Begin()
{
  this->Id = 0; // first id of the current dimension
}

//-----------------------------------------------------------------------------
// Description:
// Is there no cell at iterator position? (exit condition).
vtkTypeBool vtkBridgeCellIteratorOnCellList::IsAtEnd()
{
  return this->Id >= this->Cells->GetNumberOfIds();
}

//-----------------------------------------------------------------------------
// Description:
// Cell at current position
// \pre not_at_end: !IsAtEnd()
// \pre c_exists: c!=0
// THREAD SAFE
void vtkBridgeCellIteratorOnCellList::GetCell(vtkGenericAdaptorCell* c)
{
  assert("pre: not_at_end" && !IsAtEnd());
  assert("pre: c_exists" && c != nullptr);

  vtkBridgeCell* c2 = static_cast<vtkBridgeCell*>(c);
  c2->Init(this->DataSet, this->Cells->GetId(this->Id));
}

//-----------------------------------------------------------------------------
// Description:
// Cell at current position.
// NOT THREAD SAFE
// \pre not_at_end: !IsAtEnd()
// \post result_exits: result!=0
vtkGenericAdaptorCell* vtkBridgeCellIteratorOnCellList::GetCell()
{
  assert("pre: not_at_end" && !IsAtEnd());

  this->Cell->Init(this->DataSet, this->Cells->GetId(this->Id));
  vtkGenericAdaptorCell* result = this->Cell;

  assert("post: result_exits" && result != nullptr);
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Move iterator to next position. (loop progression).
// \pre not_at_end: !IsAtEnd()
void vtkBridgeCellIteratorOnCellList::Next()
{
  assert("pre: not_off" && !IsAtEnd());
  this->Id++; // next id of the current dimension
}

//-----------------------------------------------------------------------------
// Description:
// Used internally by vtkBridgeCell.
// Iterate on neighbors defined by `cells' over the dataset `ds'.
// \pre cells_exist: cells!=0
// \pre ds_exists: ds!=0
void vtkBridgeCellIteratorOnCellList::InitWithCells(vtkIdList* cells, vtkBridgeDataSet* ds)
{
  assert("pre: cells_exist" && cells != nullptr);
  assert("pre: ds_exists" && ds != nullptr);

  vtkSetObjectBodyMacro(DataSet, vtkBridgeDataSet, ds);
  vtkSetObjectBodyMacro(Cells, vtkIdList, cells);
}
