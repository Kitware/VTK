/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBridgePointIteratorOnCell.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBridgePointIteratorOnCell - Implementation of vtkGenericPointIterator.
// .SECTION Description
// It is just an example that show how to implement the Generic. It is also
// used for testing and evaluating the Generic.
// .SECTION See Also
// vtkGenericPointIterator, vtkBridgeDataSet

#include "vtkBridgePointIteratorOnCell.h"

#include <cassert>

#include "vtkObjectFactory.h"
#include "vtkBridgeDataSet.h"
#include "vtkDataSet.h"
#include "vtkIdList.h"
#include "vtkBridgeCell.h"
#include "vtkCell.h"

vtkStandardNewMacro(vtkBridgePointIteratorOnCell);

//-----------------------------------------------------------------------------
// Description:
// Default constructor.
vtkBridgePointIteratorOnCell::vtkBridgePointIteratorOnCell()
{
  this->DataSet=0;
  this->Cursor=0;
  this->PtIds=0;
}

//-----------------------------------------------------------------------------
// Description:
// Destructor.
vtkBridgePointIteratorOnCell::~vtkBridgePointIteratorOnCell()
{
   vtkSetObjectBodyMacro(DataSet,vtkBridgeDataSet,0);
}

//-----------------------------------------------------------------------------
void vtkBridgePointIteratorOnCell::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//-----------------------------------------------------------------------------
// Description:
// Move iterator to first position if any (loop initialization).
void vtkBridgePointIteratorOnCell::Begin()
{
  if(this->PtIds!=0)
  {
    this->Cursor=0;
  }
}

//-----------------------------------------------------------------------------
// Description:
// Is there no point at iterator position? (exit condition).
int vtkBridgePointIteratorOnCell::IsAtEnd()
{
  return (this->PtIds==0)||(this->Cursor>=this->PtIds->GetNumberOfIds());
}

//-----------------------------------------------------------------------------
// Description:
// Move iterator to next position. (loop progression).
// \pre not_off: !IsAtEnd()
void vtkBridgePointIteratorOnCell::Next()
{
  assert("pre: not_off" && !IsAtEnd());
  this->Cursor++;
}

//-----------------------------------------------------------------------------
// Description:
// Point at iterator position.
// \pre not_off: !IsAtEnd()
// \post result_exists: result!=0
double *vtkBridgePointIteratorOnCell::GetPosition()
{
  assert("pre: not_off" && !IsAtEnd());

  double *result=this->DataSet->Implementation->GetPoint(this->PtIds->GetId(this->Cursor));

  assert("post: result_exists" && result!=0);
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Point at iterator position.
// \pre not_off: !IsAtEnd()
// \pre x_exists: x!=0
void vtkBridgePointIteratorOnCell::GetPosition(double x[3])
{
  assert("pre: not_off" && !IsAtEnd());
  assert("pre: x_exists" && x!=0);
  this->DataSet->Implementation->GetPoint(this->PtIds->GetId(this->Cursor),x);
}

//-----------------------------------------------------------------------------
// Description:
// Unique identifier for the point, could be non-contiguous
// \pre not_off: !IsAtEnd()
vtkIdType vtkBridgePointIteratorOnCell::GetId()
{
  assert("pre: not_off" && !IsAtEnd());

  return this->PtIds->GetId(this->Cursor);
}

//-----------------------------------------------------------------------------
// Description:
// The iterator will iterate over the point of a cell
// \pre cell_exists: cell!=0
void vtkBridgePointIteratorOnCell::InitWithCell(vtkBridgeCell *cell)
{
  assert("pre: cell_exists" && cell!=0);

  vtkSetObjectBodyMacro(DataSet,vtkBridgeDataSet,cell->DataSet);
  this->PtIds = cell->Cell->GetPointIds();
}
