/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBridgeAttribute.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBridgeAttribute - Implementation of vtkGenericAttribute.
// .SECTION Description
// It is just an example that show how to implement the Generic. It is also
// used for testing and evaluating the Generic.
// .SECTION See Also
// vtkGenericAttribute, vtkBridgeDataSet

#include "vtkBridgeAttribute.h"

#include "vtkBridgeCell.h"
#include "vtkBridgeCellIterator.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkGenericCell.h"
#include "vtkGenericPointIterator.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSetGet.h"

#include <assert.h>

vtkStandardNewMacro(vtkBridgeAttribute);

void vtkBridgeAttribute::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//-----------------------------------------------------------------------------
// Description:
// Name of the attribute. (e.g. "velocity")
// \post result_may_not_exist: result!=0 || result==0
const char *vtkBridgeAttribute::GetName()
{
  return this->Data->GetArray(this->AttributeNumber)->GetName();
}

//-----------------------------------------------------------------------------
// Description:
// Dimension of the attribute. (1 for scalar, 3 for velocity)
// \post positive_result: result>=0
int vtkBridgeAttribute::GetNumberOfComponents()
{
  int result=this->Data->GetArray(this->AttributeNumber)->GetNumberOfComponents();
  assert("post: positive_result" && result>=0);
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Is the attribute centered either on points, cells or boundaries?
// \post valid_result: (result==vtkPointCentered) ||
//            (result==vtkCellCentered) || (result==vtkBoundaryCentered)
int vtkBridgeAttribute::GetCentering()
{
  int result;
  if(this->Pd!=0)
    {
    result=vtkPointCentered;
    }
  else
    {
    result=vtkCellCentered;
    }
  assert("post: valid_result" && ((result==vtkPointCentered) || (result==vtkCellCentered) || (result==vtkBoundaryCentered)));
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Type of the attribute: scalar, vector, normal, texture coordinate, tensor
// \post valid_result: (result==vtkDataSetAttributes::SCALARS)
//                   ||(result==vtkDataSetAttributes::VECTORS)
//                   ||(result==vtkDataSetAttributes::NORMALS)
//                   ||(result==vtkDataSetAttributes::TCOORDS)
//                   ||(result==vtkDataSetAttributes::TENSORS)
int vtkBridgeAttribute::GetType()
{
  int result=this->Data->IsArrayAnAttribute(this->AttributeNumber);
  if(result==-1)
    {
    switch(this->GetNumberOfComponents())
      {
      case 1:
        result=vtkDataSetAttributes::SCALARS;
        break;
      case 3:
        result=vtkDataSetAttributes::VECTORS;
        break;
      case 9:
        result=vtkDataSetAttributes::TENSORS;
        break;
      default:
        assert("check: unknown attribute type" && 0);
        break;
      }
    }
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Type of the components of the attribute: int, float, double
// \post valid_result: (result==VTK_BIT)           ||(result==VTK_CHAR)
//                   ||(result==VTK_UNSIGNED_CHAR) ||(result==VTK_SHORT)
//                   ||(result==VTK_UNSIGNED_SHORT)||(result==VTK_INT)
//                   ||(result==VTK_UNSIGNED_INT)  ||(result==VTK_LONG)
//                   ||(result==VTK_UNSIGNED_LONG) ||(result==VTK_FLOAT)
//                   ||(result==VTK_DOUBLE)        ||(result==VTK_ID_TYPE)
int vtkBridgeAttribute::GetComponentType()
{
  return this->Data->GetArray(this->AttributeNumber)->GetDataType();
}

//-----------------------------------------------------------------------------
// Description:
// Number of tuples.
// \post valid_result: result>=0
vtkIdType vtkBridgeAttribute::GetSize()
{
  vtkIdType result=this->Data->GetArray(this->AttributeNumber)->GetNumberOfTuples();
  assert("post: valid_result" && result>=0);
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Size in kilobytes taken by the attribute.
unsigned long vtkBridgeAttribute::GetActualMemorySize()
{
  return this->Data->GetArray(this->AttributeNumber)->GetActualMemorySize();
}

//-----------------------------------------------------------------------------
// Description:
// Range of the attribute component `component'. It returns double, even if
// GetType()==VTK_INT.
// NOT THREAD SAFE
// \pre valid_component: (component>=-1)&&(component<GetNumberOfComponents())
// \post result_exists: result!=0
double *vtkBridgeAttribute::GetRange(int component)
{
  assert("pre: valid_component" && (component>=-1)&&(component<this->GetNumberOfComponents()));
  double *result=this->Data->GetArray(this->AttributeNumber)->GetRange(component);
  assert("post: result_exists" && result!=0);
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Range of the attribute component `component'.
// THREAD SAFE
// \pre valid_component: (component>=-1)&&(component<GetNumberOfComponents())
void vtkBridgeAttribute::GetRange(int component,
                                  double range[2])
{
   assert("pre: valid_component" && (component>=-1)&&(component<this->GetNumberOfComponents()));
   this->Data->GetArray(this->AttributeNumber)->GetRange(range,component);
}

//-----------------------------------------------------------------------------
// Description:
// Return the maximum euclidean norm for the tuples.
// \post positive_result: result>=0
double vtkBridgeAttribute::GetMaxNorm()
{
  double result=this->Data->GetArray(this->AttributeNumber)->GetMaxNorm();
  assert("post: positive_result" && result>=0);
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Attribute at all points of cell `c'.
// \pre c_exists: c!=0
// \pre c_valid: !c->IsAtEnd()
// \post result_exists: result!=0
// \post valid_result: sizeof(result)==GetNumberOfComponents()*c->GetCell()->GetNumberOfPoints()
double *vtkBridgeAttribute::GetTuple(vtkGenericAdaptorCell *c)
{
  assert("pre: c_exists" && c!=0);

  this->AllocateInternalTuple(c->GetNumberOfPoints()*this->GetNumberOfComponents());
  this->GetTuple(c,this->InternalTuple);

  assert("post: result_exists" && this->InternalTuple!=0);
  return this->InternalTuple;
}

//-----------------------------------------------------------------------------
// Description:
// Put attribute at all points of cell `c' in `tuple'.
// \pre c_exists: c!=0
// \pre c_valid: !c->IsAtEnd()
// \pre tuple_exists: tuple!=0
// \pre valid_tuple: sizeof(tuple)>=GetNumberOfComponents()*c->GetCell()->GetNumberOfPoints()
void vtkBridgeAttribute::GetTuple(vtkGenericAdaptorCell *c, double *tuple)
{
  assert("pre: c_exists" && c!=0);
  assert("pre: tuple_exists" && tuple!=0);

  double *p=tuple;
  int i;
  int j;
  int size;
  vtkBridgeCell *c2=static_cast<vtkBridgeCell *>(c);


  if(this->Pd!=0)
    {
    i=0;
    size=c2->GetNumberOfPoints();
    while(i<size)
      {
      j=c2->Cell->GetPointId(i);
      this->Data->GetArray(this->AttributeNumber)->GetTuple(j,p);
      ++i;
      p=p+this->GetNumberOfComponents();
      }
    }
  else
    {
    this->Data->GetArray(this->AttributeNumber)->GetTuple(c2->GetId(),tuple);
    // duplicate:
    size=c2->GetNumberOfPoints();
    i=1;
    p=p+this->GetNumberOfComponents();
    while(i<size)
      {
      memcpy(p,tuple,sizeof(double)*this->GetNumberOfComponents());
      p=p+this->GetNumberOfComponents();
      ++i;
      }
    }
}

//-----------------------------------------------------------------------------
// Description:
// Attribute at all points of cell `c'.
// \pre c_exists: c!=0
// \pre c_valid: !c->IsAtEnd()
// \post result_exists: result!=0
// \post valid_result: sizeof(result)==GetNumberOfComponents()*c->GetCell()->GetNumberOfPoints()
double *vtkBridgeAttribute::GetTuple(vtkGenericCellIterator *c)
{
  assert("pre: c_exists" && c!=0);
  assert("pre: c_valid" && !c->IsAtEnd());

  return this->GetTuple(c->GetCell());
}

//-----------------------------------------------------------------------------
// Description:
// Put attribute at all points of cell `c' in `tuple'.
// \pre c_exists: c!=0
// \pre c_valid: !c->IsAtEnd()
// \pre tuple_exists: tuple!=0
// \pre valid_tuple: sizeof(tuple)>=GetNumberOfComponents()*c->GetCell()->GetNumberOfPoints()
void vtkBridgeAttribute::GetTuple(vtkGenericCellIterator *c, double *tuple)
{
  assert("pre: c_exists" && c!=0);
  assert("pre: c_valid" && !c->IsAtEnd());
  assert("pre: tuple_exists" && tuple!=0);

  this->GetTuple(c->GetCell(),tuple);
}

//-----------------------------------------------------------------------------
// Description:
// Value of the attribute at position `p'.
// \pre p_exists: p!=0
// \pre p_valid: !p->IsAtEnd()
// \post result_exists: result!=0
// \post valid_result_size: sizeof(result)==GetNumberOfComponents()
double *vtkBridgeAttribute::GetTuple(vtkGenericPointIterator *p)
{
  assert("pre: p_exists" && p!=0);
  assert("pre: p_valid" && !p->IsAtEnd());

  this->AllocateInternalTuple(this->GetNumberOfComponents());

  this->Data->GetArray(this->AttributeNumber)->GetTuple(p->GetId(),this->InternalTuple);

  assert("post: result_exists" && this->InternalTuple!=0);
  return this->InternalTuple;
}

//-----------------------------------------------------------------------------
// Description:
// Put the value of the attribute at position `p' into `tuple'.
// \pre p_exists: p!=0
// \pre p_valid: !p->IsAtEnd()
// \pre tuple_exists: tuple!=0
// \pre valid_tuple_size: sizeof(tuple)>=GetNumberOfComponents()
void vtkBridgeAttribute::GetTuple(vtkGenericPointIterator *p, double *tuple)
{
  assert("pre: p_exists" && p!=0);
  assert("pre: p_valid" && !p->IsAtEnd());
  assert("pre: tuple_exists" && tuple!=0);
  this->Data->GetArray(this->AttributeNumber)->GetTuple(p->GetId(),tuple);
}

//-----------------------------------------------------------------------------
// Description:
// Put component `i' of the attribute at all points of cell `c' in `values'.
// \pre valid_component: (i>=0) && (i<GetNumberOfComponents())
// \pre c_exists: c!=0
// \pre c_valid: !c->IsAtEnd()
// \pre values_exist: values!=0
// \pre valid_values: sizeof(values)>=c->GetCell()->GetNumberOfPoints()
void vtkBridgeAttribute::GetComponent(int i,vtkGenericCellIterator *c, double *values)
{
  assert("pre: c_exists" && c!=0);
  assert("pre: c_valid" && !c->IsAtEnd());

  int j;
  int id;
  int size;
  vtkBridgeCellIterator *c2=static_cast<vtkBridgeCellIterator *>(c);

  if(this->Pd!=0)
    {
    j=0;
    size=c2->GetCell()->GetNumberOfPoints();
    while(j<size)
      {
      id=static_cast<vtkBridgeCell *>(c2->GetCell())->Cell->GetPointId(j);
      values[j]=this->Data->GetArray(this->AttributeNumber)->GetComponent(id,i);
      ++j;
      }
    }
  else
    {
    values[0]=this->Data->GetArray(this->AttributeNumber)->GetComponent(c2->GetCell()->GetId(),i);
    // duplicate:
    size=c2->GetCell()->GetNumberOfPoints();
    j=1;
    while(j<size)
      {
      values[j]=values[0];
      ++j;
      }
    }
}

//-----------------------------------------------------------------------------
// Description:
// Value of the component `i' of the attribute at position `p'.
// \pre valid_component: (i>=0) && (i<GetNumberOfComponents())
// \pre p_exists: p!=0
// \pre p_valid: !p->IsAtEnd()
double vtkBridgeAttribute::GetComponent(int i,vtkGenericPointIterator *p)
{
  assert("pre: p_exists" && p!=0);
  assert("pre: p_valid" && !p->IsAtEnd());
  // Only relevant if GetCentering()==vtkCenteringPoint?
  return this->Data->GetArray(this->AttributeNumber)->GetComponent(p->GetId(),i);
}

//-----------------------------------------------------------------------------
// Description:
// Recursive duplication of `other' in `this'.
// \pre other_exists: other!=0
// \pre not_self: other!=this
void vtkBridgeAttribute::DeepCopy(vtkGenericAttribute *other)
{
  assert("pre: other_exists" && other!=0);
  assert("pre: not_self" && other!=this);
  vtkBridgeAttribute *o=static_cast<vtkBridgeAttribute *>(other);

  vtkSetObjectBodyMacro(Pd,vtkPointData,o->Pd);
  vtkSetObjectBodyMacro(Cd,vtkCellData,o->Cd);
  this->Data=o->Data;
  this->AttributeNumber=o->AttributeNumber;
  this->AllocateInternalTuple(this->GetNumberOfComponents());
}

//-----------------------------------------------------------------------------
// Description:
// Update `this' using fields of `other'.
// \pre other_exists: other!=0
// \pre not_self: other!=this
void vtkBridgeAttribute::ShallowCopy(vtkGenericAttribute *other)
{
  assert("pre: other_exists" && other!=0);
  assert("pre: not_self" && other!=this);
  vtkBridgeAttribute *o=static_cast<vtkBridgeAttribute *>(other);

  vtkSetObjectBodyMacro(Pd,vtkPointData,o->Pd);
  vtkSetObjectBodyMacro(Cd,vtkCellData,o->Cd);
  this->Data=o->Data;
  this->AttributeNumber=o->AttributeNumber;
  this->AllocateInternalTuple(this->GetNumberOfComponents());
}

//-----------------------------------------------------------------------------
// Description:
// Set the current attribute to be centered on points with attribute `i' of
// `d'.
// \pre d_exists: d!=0
// \pre valid_range: (i>=0) && (i<d->GetNumberOfArrays())
void vtkBridgeAttribute::InitWithPointData(vtkPointData *d,
                                           int i)
{
  assert("pre: d_exists" && d!=0);
  assert("pre: valid_range" && (i>=0) && (i<d->GetNumberOfArrays()));
  vtkSetObjectBodyMacro(Cd,vtkCellData,0);
  vtkSetObjectBodyMacro(Pd,vtkPointData,d);
  this->Data=d;
  this->AttributeNumber=i;
  this->AllocateInternalTuple(this->GetNumberOfComponents());
}

//-----------------------------------------------------------------------------
// Description:
// Set the current attribute to be centered on cells with attribute `i' of `d'.
// \pre d_exists: d!=0
// \pre valid_range: (i>=0) && (i<d->GetNumberOfArrays())
void vtkBridgeAttribute::InitWithCellData(vtkCellData *d,
                                          int i)
{
  assert("pre: d_exists" && d!=0);
  assert("pre: valid_range" && (i>=0) && (i<d->GetNumberOfArrays()));
  vtkSetObjectBodyMacro(Pd,vtkPointData,0);
  vtkSetObjectBodyMacro(Cd,vtkCellData,d);
  this->Data=d;
  this->AttributeNumber=i;
  this->AllocateInternalTuple(this->GetNumberOfComponents());
}

//-----------------------------------------------------------------------------
// Description:
// Default constructor: empty attribute, not valid
vtkBridgeAttribute::vtkBridgeAttribute()
{
  this->Pd=0;
  this->Cd=0;
  this->Data=0;
  this->AttributeNumber=0;
  this->InternalTuple=0;
  this->InternalTupleCapacity=0;
}

//-----------------------------------------------------------------------------
// Description:
// Destructor.
vtkBridgeAttribute::~vtkBridgeAttribute()
{
  if(this->Pd!=0)
    {
    this->Pd->Delete();
    }
  else
    {
    if(this->Cd!=0)
      {
      this->Cd->Delete();
      }
    }
  if(this->InternalTuple!=0)
    {
    delete[] this->InternalTuple;
    }
}

//-----------------------------------------------------------------------------
// Description:
// If size>InternalTupleCapacity, allocate enough memory.
// \pre positive_size: size>0
void vtkBridgeAttribute::AllocateInternalTuple(int size)
{
  // size=this->GetNumberOfComponents()
  assert("pre: positive_size" && size>0);

  if(this->InternalTuple==0)
    {
    this->InternalTupleCapacity = size;
    this->InternalTuple = new double[this->InternalTupleCapacity];
    }
  else
    {
    if(InternalTupleCapacity<size)
      {
      this->InternalTupleCapacity = size;
      delete [] this->InternalTuple;
      this->InternalTuple = new double[this->InternalTupleCapacity];
      }
    }
}
