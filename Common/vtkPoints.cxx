/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPoints.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPoints.h"
#include "vtkObjectFactory.h"
#include "vtkBitArray.h"
#include "vtkCharArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkShortArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkIntArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkLongArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"

vtkCxxRevisionMacro(vtkPoints, "1.44");

//----------------------------------------------------------------------------
vtkPoints* vtkPoints::New(int dataType)
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPoints");
  if(ret)
    {
    return (vtkPoints*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPoints(dataType);
}

vtkPoints* vtkPoints::New()
{
  return vtkPoints::New(VTK_FLOAT);
}

// Construct object with an initial data array of type float.
vtkPoints::vtkPoints(int dataType)
{
  this->Data = vtkFloatArray::New();
  this->Data->Register(this);
  this->Data->Delete();
  this->SetDataType(dataType);

  this->Data->SetNumberOfComponents(3);

  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = 0.0;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = 1.0;
}

vtkPoints::~vtkPoints()
{
  this->Data->UnRegister(this);
}

vtkPoints *vtkPoints::MakeObject()
{
  vtkPoints *p = vtkPoints::New();
  p->SetDataType(this->GetDataType());
  return p;
}

// Given a list of pt ids, return an array of points.
void vtkPoints::GetPoints(vtkIdList *ptIds, vtkPoints *fp)
{
  vtkIdType num = ptIds->GetNumberOfIds();

  for (vtkIdType i=0; i < num; i++)
    {
    fp->InsertPoint(i, this->GetPoint(ptIds->GetId(i)));
    }
}

// Determine (xmin,xmax, ymin,ymax, zmin,zmax) bounds of points.
void vtkPoints::ComputeBounds()
{
  vtkIdType i;
  int j;
  float *x;

  if ( this->GetMTime() > this->ComputeTime )
    {
    this->Bounds[0] = this->Bounds[2] = this->Bounds[4] =  VTK_LARGE_FLOAT;
    this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_LARGE_FLOAT;
    for (i=0; i<this->GetNumberOfPoints(); i++)
      {
      x = this->GetPoint(i);
      for (j=0; j<3; j++)
        {
        if ( x[j] < this->Bounds[2*j] )
          {
          this->Bounds[2*j] = x[j];
          }
        if ( x[j] > this->Bounds[2*j+1] )
          {
          this->Bounds[2*j+1] = x[j];
          }
        }
      }

    this->ComputeTime.Modified();
    }
}

// Return the bounds of the points.
float *vtkPoints::GetBounds()
{
  this->ComputeBounds();
  return this->Bounds;
}

// Return the bounds of the points.
void vtkPoints::GetBounds(float bounds[6])
{
  this->ComputeBounds();
  for (int i=0; i<6; i++)
    {
    bounds[i] = this->Bounds[i];
    }
}

int vtkPoints::Allocate(const vtkIdType sz, const vtkIdType ext)
{
  int numComp=this->Data->GetNumberOfComponents();
  return this->Data->Allocate(sz*numComp,ext*numComp);
}

void vtkPoints::Initialize()
{
  this->Data->Initialize();
}

int vtkPoints::GetDataType()
{
  return this->Data->GetDataType();
}

// Specify the underlying data type of the object.
void vtkPoints::SetDataType(int dataType)
{
  if ( dataType == this->Data->GetDataType() )
    {
    return;
    }
  
  this->Modified();
  
  switch (dataType)
    {
    case VTK_BIT:
      this->Data->Delete();
      this->Data = vtkBitArray::New();
      break;

    case VTK_CHAR:
      this->Data->Delete();
      this->Data = vtkCharArray::New();
      break;

    case VTK_UNSIGNED_CHAR:
      this->Data->Delete();
      this->Data = vtkUnsignedCharArray::New();
      break;

    case VTK_SHORT:
      this->Data->Delete();
      this->Data = vtkShortArray::New();
      break;

    case VTK_UNSIGNED_SHORT:
      this->Data->Delete();
      this->Data = vtkUnsignedShortArray::New();
      break;

    case VTK_INT:
      this->Data->Delete();
      this->Data = vtkIntArray::New();
      break;

    case VTK_UNSIGNED_INT:
      this->Data->Delete();
      this->Data = vtkUnsignedIntArray::New();
      break;

    case VTK_LONG:
      this->Data->Delete();
      this->Data = vtkLongArray::New();
      break;

    case VTK_UNSIGNED_LONG:
      this->Data->Delete();
      this->Data = vtkUnsignedLongArray::New();
      break;

    case VTK_FLOAT:
      this->Data->Delete();
      this->Data = vtkFloatArray::New();
      break;

    case VTK_DOUBLE:
      this->Data->Delete();
      this->Data = vtkDoubleArray::New();
      break;

    case VTK_ID_TYPE:
      this->Data->Delete();
      this->Data = vtkIdTypeArray::New();
      break;

    default:
      vtkErrorMacro(<<"Unsupported data type! Setting to VTK_FLOAT");
      this->SetDataType(VTK_FLOAT);
    }
}

// Set the data for this object. The tuple dimension must be consistent with
// the object.
void vtkPoints::SetData(vtkDataArray *data)
{
  if ( data != this->Data && data != NULL )
    {
    if (data->GetNumberOfComponents() != this->Data->GetNumberOfComponents() )
      {
      vtkErrorMacro(<<"Number of components is different...can't set data");
      return;
      }
    this->Data->UnRegister(this);
    this->Data = data;
    this->Data->Register(this);
    this->Modified();
    }
}

// Deep copy of data. Checks consistency to make sure this operation
// makes sense.
void vtkPoints::DeepCopy(vtkPoints *da)
{
  if (da == NULL)
    {
    return;
    }
  if ( da->Data != this->Data && da->Data != NULL )
    {
    if (da->Data->GetNumberOfComponents() != this->Data->GetNumberOfComponents() )
      {
      vtkErrorMacro(<<"Number of components is different...can't copy");
      return;
      }
    this->Data->DeepCopy(da->Data);
    this->Modified();
    }
}

// Shallow copy of data (i.e. via reference counting). Checks 
// consistency to make sure this operation makes sense.
void vtkPoints::ShallowCopy(vtkPoints *da)
{
  this->SetData(da->GetData());
}

unsigned long vtkPoints::GetActualMemorySize()
{
  return this->Data->GetActualMemorySize();
}

void vtkPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  float *bounds;

  os << indent << "Data: " << this->Data << "\n";
  if ( this->Data )
    {
    if ( this->Data->GetName() )
      {
      os << indent << "Data Array Name: " << this->Data->GetName() << "\n";
      }
    else
      {
      os << indent << "Data Array Name: (none)\n";
      }
    }

  os << indent << "Number Of Points: " << this->GetNumberOfPoints() << "\n";
  bounds = this->GetBounds();
  os << indent << "Bounds: \n";
  os << indent << "  Xmin,Xmax: (" << bounds[0] << ", " << bounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << bounds[2] << ", " << bounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << bounds[4] << ", " << bounds[5] << ")\n";
}

