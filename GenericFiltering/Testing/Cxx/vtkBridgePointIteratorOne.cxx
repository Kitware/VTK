/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBridgePointIteratorOne.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBridgePointIteratorOne - Implementation of vtkGenericPointIterator.
// .SECTION Description
// It is just an example that show how to implement the Generic. It is also
// used for testing and evaluating the Generic.
// .SECTION See Also
// vtkGenericPointIterator, vtkBridgeDataSet

#include "vtkBridgePointIteratorOne.h"

#include <assert.h>

#include "vtkObjectFactory.h"
#include "vtkBridgeDataSet.h"
#include "vtkDataSet.h"

vtkStandardNewMacro(vtkBridgePointIteratorOne);

//-----------------------------------------------------------------------------
// Description:
// Default constructor.
vtkBridgePointIteratorOne::vtkBridgePointIteratorOne()
{
  this->DataSet=0;
  this->cIsAtEnd=1;
}

//-----------------------------------------------------------------------------
// Description:
// Destructor.
vtkBridgePointIteratorOne::~vtkBridgePointIteratorOne()
{
   vtkSetObjectBodyMacro(DataSet,vtkBridgeDataSet,0);
}

//-----------------------------------------------------------------------------
void vtkBridgePointIteratorOne::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//-----------------------------------------------------------------------------
// Description:
// Move iterator to first position if any (loop initialization).
void vtkBridgePointIteratorOne::Begin()
{
  this->cIsAtEnd=0;
}

//-----------------------------------------------------------------------------
// Description:
// Is there no point at iterator position? (exit condition).
int vtkBridgePointIteratorOne::IsAtEnd()
{
  return this->cIsAtEnd;
}

//-----------------------------------------------------------------------------
// Description:
// Move iterator to next position. (loop progression).
// \pre not_off: !IsAtEnd()
void vtkBridgePointIteratorOne::Next()
{
  assert("pre: not_off" && !IsAtEnd());
  this->cIsAtEnd=1;
}
 
//-----------------------------------------------------------------------------
// Description:
// Point at iterator position.
// \pre not_off: !IsAtEnd()
// \post result_exists: result!=0
double *vtkBridgePointIteratorOne::GetPosition()
{
  assert("pre: not_off" && !IsAtEnd());
  
  double *result=this->DataSet->Implementation->GetPoint(this->Id);
  
  assert("post: result_exists" && result!=0);
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Point at iterator position.
// \pre not_off: !IsAtEnd()
// \pre x_exists: x!=0
void vtkBridgePointIteratorOne::GetPosition(double x[3])
{
  assert("pre: not_off" && !IsAtEnd());
  assert("pre: x_exists" && x!=0);
  this->DataSet->Implementation->GetPoint(this->Id,x);
}
  
//-----------------------------------------------------------------------------
// Description:
// Unique identifier for the point, could be non-contiguous
// \pre not_off: !IsAtEnd()
vtkIdType vtkBridgePointIteratorOne::GetId()
{
  assert("pre: not_off" && !IsAtEnd());
  
  return this->Id;
}

//-----------------------------------------------------------------------------
// Description:
// Used internally by vtkBridgeDataSet.
// Iterate over one point of identifier `id' on dataset `ds'.
// \pre ds_can_be_null: ds!=0 || ds==0
// \pre valid_id: vtkImplies(ds!=0,(id>=0)&&(id<=ds->GetNumberOfCells()))
void vtkBridgePointIteratorOne::InitWithOnePoint(vtkBridgeDataSet *ds,
                                                 vtkIdType id)
{
  assert("pre: valid_id" && 
         ((!ds!=0)|| ((id>=0)&&(id<=ds->GetNumberOfCells())))); // A=>B: !A||B
  
  vtkSetObjectBodyMacro(DataSet,vtkBridgeDataSet,ds);
  this->Id=id;
}
