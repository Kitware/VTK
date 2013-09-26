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

#include "vtkObjectFactory.h"
#include "vtkBridgeCell.h"
#include "vtkBridgeDataSet.h"
#include "vtkDataSet.h"
#include "vtkCell.h"
#include "vtkVertex.h"
#include "vtkPoints.h"

vtkStandardNewMacro(vtkBridgeCellIteratorOnCellList);

//-----------------------------------------------------------------------------
void vtkBridgeCellIteratorOnCellList::PrintSelf(ostream& os,
                                                vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//-----------------------------------------------------------------------------
vtkBridgeCellIteratorOnCellList::vtkBridgeCellIteratorOnCellList()
{
  this->DataSet=0;
  this->Cells=0;
  this->Cell=vtkBridgeCell::New();
  this->Id=0;
//  this->DebugOn();
}

//-----------------------------------------------------------------------------
vtkBridgeCellIteratorOnCellList::~vtkBridgeCellIteratorOnCellList()
{
  if(this->DataSet!=0)
    {
    this->DataSet->Delete();
    this->DataSet=0;
    }

  if(this->Cells!=0)
    {
    this->Cells->Delete();
    this->Cells=0;
    }

  this->Cell->Delete();
  this->Cell=0;
}

//-----------------------------------------------------------------------------
// Description:
// Move iterator to first position if any (loop initialization).
void vtkBridgeCellIteratorOnCellList::Begin()
{
  this->Id=0; // first id of the current dimension
}

//-----------------------------------------------------------------------------
// Description:
// Is there no cell at iterator position? (exit condition).
int vtkBridgeCellIteratorOnCellList::IsAtEnd()
{
  return this->Id>=this->Cells->GetNumberOfIds();
}

//-----------------------------------------------------------------------------
// Description:
// Cell at current position
// \pre not_at_end: !IsAtEnd()
// \pre c_exists: c!=0
// THREAD SAFE
void vtkBridgeCellIteratorOnCellList::GetCell(vtkGenericAdaptorCell *c)
{
  assert("pre: not_at_end" && !IsAtEnd());
  assert("pre: c_exists" && c!=0);

  vtkBridgeCell *c2=static_cast<vtkBridgeCell *>(c);
  c2->Init(this->DataSet,this->Cells->GetId(this->Id));
}

//-----------------------------------------------------------------------------
// Description:
// Cell at current position.
// NOT THREAD SAFE
// \pre not_at_end: !IsAtEnd()
// \post result_exits: result!=0
vtkGenericAdaptorCell *vtkBridgeCellIteratorOnCellList::GetCell()
{
  assert("pre: not_at_end" && !IsAtEnd());

  this->Cell->Init(this->DataSet,this->Cells->GetId(this->Id));
  vtkGenericAdaptorCell *result=this->Cell;

  assert("post: result_exits" && result!=0);
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
void vtkBridgeCellIteratorOnCellList::InitWithCells(vtkIdList *cells,
                                                    vtkBridgeDataSet *ds)
{
  assert("pre: cells_exist" && cells!=0);
  assert("pre: ds_exists" && ds!=0);

  vtkSetObjectBodyMacro(DataSet,vtkBridgeDataSet,ds);
  vtkSetObjectBodyMacro(Cells,vtkIdList,cells);
}
