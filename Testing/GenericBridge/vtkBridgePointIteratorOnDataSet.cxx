/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBridgePointIteratorOnDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBridgePointIteratorOnDataSet - Implementation of vtkGenericPointIterator.
// .SECTION Description
// It is just an example that show how to implement the Generic. It is also
// used for testing and evaluating the Generic.
// .SECTION See Also
// vtkGenericPointIterator, vtkBridgeDataSet

#include "vtkBridgePointIteratorOnDataSet.h"

#include <assert.h>

#include "vtkObjectFactory.h"
#include "vtkBridgeDataSet.h"
#include "vtkDataSet.h"

vtkStandardNewMacro(vtkBridgePointIteratorOnDataSet);

//-----------------------------------------------------------------------------
// Description:
// Default constructor.
vtkBridgePointIteratorOnDataSet::vtkBridgePointIteratorOnDataSet()
{
  this->DataSet=0;
  this->Size=0;
}

//-----------------------------------------------------------------------------
// Description:
// Destructor.
vtkBridgePointIteratorOnDataSet::~vtkBridgePointIteratorOnDataSet()
{
   vtkSetObjectBodyMacro(DataSet,vtkBridgeDataSet,0);
}

//-----------------------------------------------------------------------------
void vtkBridgePointIteratorOnDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//-----------------------------------------------------------------------------
// Description:
// Move iterator to first position if any (loop initialization).
void vtkBridgePointIteratorOnDataSet::Begin()
{
  this->Id=0;
}

//-----------------------------------------------------------------------------
// Description:
// Is there no point at iterator position? (exit condition).
int vtkBridgePointIteratorOnDataSet::IsAtEnd()
{
  return (this->Id<0) || (this->Id>=this->Size);
}

//-----------------------------------------------------------------------------
// Description:
// Move iterator to next position. (loop progression).
// \pre not_off: !IsAtEnd()
void vtkBridgePointIteratorOnDataSet::Next()
{
  assert("pre: not_off" && !IsAtEnd());
  this->Id++;
}

//-----------------------------------------------------------------------------
// Description:
// Point at iterator position.
// \pre not_off: !IsAtEnd()
// \post result_exists: result!=0
double *vtkBridgePointIteratorOnDataSet::GetPosition()
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
void vtkBridgePointIteratorOnDataSet::GetPosition(double x[3])
{
  assert("pre: not_off" && !IsAtEnd());
  assert("pre: x_exists" && x!=0);
  this->DataSet->Implementation->GetPoint(this->Id,x);
}

//-----------------------------------------------------------------------------
// Description:
// Unique identifier for the point, could be non-contiguous
// \pre not_off: !IsAtEnd()
vtkIdType vtkBridgePointIteratorOnDataSet::GetId()
{
  assert("pre: not_off" && !IsAtEnd());

  return this->Id;
}

//-----------------------------------------------------------------------------
// Description:
// Used internally by vtkBridgeDataSet.
// Iterate over points of `ds'.
// \pre ds_exists: ds!=0
void vtkBridgePointIteratorOnDataSet::InitWithDataSet(vtkBridgeDataSet *ds)
{
  assert("pre: ds_exists" && ds!=0);

  vtkSetObjectBodyMacro(DataSet,vtkBridgeDataSet,ds);
  this->Size=ds->GetNumberOfPoints();
}
