/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBridgePointIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBridgePointIterator - Implementation of vtkGenericPointIterator.
// .SECTION Description
// It is just an example that show how to implement the Generic. It is also
// used for testing and evaluating the Generic.
// .SECTION See Also
// vtkGenericPointIterator, vtkBridgeDataSet

#include "vtkBridgePointIterator.h"

#include <assert.h>

#include "vtkObjectFactory.h"
#include "vtkBridgeDataSet.h"
#include "vtkDataSet.h"

#include "vtkBridgePointIteratorOnDataSet.h"
#include "vtkBridgePointIteratorOne.h"
#include "vtkBridgePointIteratorOnCell.h"

vtkStandardNewMacro(vtkBridgePointIterator);

//-----------------------------------------------------------------------------
// Description:
// Default constructor.
vtkBridgePointIterator::vtkBridgePointIterator()
{
  this->CurrentIterator=0;
  this->IteratorOnDataSet=vtkBridgePointIteratorOnDataSet::New();
  this->IteratorOne=vtkBridgePointIteratorOne::New();
  this->IteratorOnCell=vtkBridgePointIteratorOnCell::New();
}

//-----------------------------------------------------------------------------
// Description:
// Destructor.
vtkBridgePointIterator::~vtkBridgePointIterator()
{
  this->IteratorOnDataSet->Delete();
  this->IteratorOne->Delete();
  this->IteratorOnCell->Delete();
}

//-----------------------------------------------------------------------------
void vtkBridgePointIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//-----------------------------------------------------------------------------
// Description:
// Move iterator to first position if any (loop initialization).
void vtkBridgePointIterator::Begin()
{
  if(this->CurrentIterator!=0)
    {
    this->CurrentIterator->Begin();
    }
}

//-----------------------------------------------------------------------------
// Description:
// Is there no point at iterator position? (exit condition).
int vtkBridgePointIterator::IsAtEnd()
{
  int result=1;
  
  if(this->CurrentIterator!=0)
    {
    result=this->CurrentIterator->IsAtEnd();
    }
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Move iterator to next position. (loop progression).
// \pre not_off: !IsAtEnd()
void vtkBridgePointIterator::Next()
{
  assert("pre: not_off" && !IsAtEnd());
  this->CurrentIterator->Next();
}
 
//-----------------------------------------------------------------------------
// Description:
// Point at iterator position.
// \pre not_off: !IsAtEnd()
// \post result_exists: result!=0
double *vtkBridgePointIterator::GetPosition()
{
  assert("pre: not_off" && !IsAtEnd());
  
  double *result=this->CurrentIterator->GetPosition();
  
  assert("post: result_exists" && result!=0);
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Point at iterator position.
// \pre not_off: !IsAtEnd()
// \pre x_exists: x!=0
void vtkBridgePointIterator::GetPosition(double x[3])
{
  assert("pre: not_off" && !IsAtEnd());
  assert("pre: x_exists" && x!=0);
  this->CurrentIterator->GetPosition(x);
}
  
//-----------------------------------------------------------------------------
// Description:
// Unique identifier for the point, could be non-contiguous
// \pre not_off: !IsAtEnd()
vtkIdType vtkBridgePointIterator::GetId()
{
  assert("pre: not_off" && !IsAtEnd());
  
  return this->CurrentIterator->GetId();
}
  
//-----------------------------------------------------------------------------
// Description:
// Used internally by vtkBridgeDataSet.
// Iterate over points of `ds'.
// \pre ds_exists: ds!=0
void vtkBridgePointIterator::InitWithDataSet(vtkBridgeDataSet *ds)
{
  assert("pre: ds_exists" && ds!=0);
  
  this->IteratorOnDataSet->InitWithDataSet(ds);
  this->CurrentIterator=this->IteratorOnDataSet;
}

//-----------------------------------------------------------------------------
// Description:
// Used internally by vtkBridgeDataSet.
// Iterate over one point of identifier `id' on dataset `ds'.
// \pre ds_can_be_null: ds!=0 || ds==0
// \pre valid_id: vtkImplies(ds!=0,(id>=0)&&(id<=ds->GetNumberOfCells()))
void vtkBridgePointIterator::InitWithOnePoint(vtkBridgeDataSet *ds,
                                              vtkIdType id)
{
  assert("pre: valid_id" && 
         ((!ds!=0)|| ((id>=0)&&(id<=ds->GetNumberOfCells())))); // A=>B: !A||B
  
  this->IteratorOne->InitWithOnePoint(ds,id);
  this->CurrentIterator=this->IteratorOne;
}

//-----------------------------------------------------------------------------
// Description:
// The iterator will iterate over the point of a cell
// \pre cell_exists: cell!=0
void vtkBridgePointIterator::InitWithCell(vtkBridgeCell *cell)
{
  assert("pre: cell_exists" && cell!=0);
  
  this->IteratorOnCell->InitWithCell(cell);
  this->CurrentIterator=this->IteratorOnCell;
}
