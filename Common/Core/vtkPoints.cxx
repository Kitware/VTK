/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPoints.h"

#include "vtkBitArray.h"
#include "vtkCharArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkObjectFactory.h"
#include "vtkShortArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"


//----------------------------------------------------------------------------
// Needed when we don't use the vtkStandardNewMacro.
vtkInstantiatorNewMacro(vtkPoints);

//----------------------------------------------------------------------------
vtkPoints* vtkPoints::New(int dataType)
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPoints");
  if(ret)
    {
    if(dataType != VTK_FLOAT)
      {
      static_cast<vtkPoints*>(ret)->SetDataType(dataType);
      }
    return static_cast<vtkPoints*>(ret);
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
  this->Data->SetName("Points");

  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = 0.0;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = 1.0;
}

vtkPoints::~vtkPoints()
{
  this->Data->UnRegister(this);
}

// Given a list of pt ids, return an array of points.
void vtkPoints::GetPoints(vtkIdList *ptIds, vtkPoints *outPoints)
{
  outPoints->Data->SetNumberOfTuples(ptIds->GetNumberOfIds());
  this->Data->GetTuples(ptIds, outPoints->Data);
}

// Determine (xmin,xmax, ymin,ymax, zmin,zmax) bounds of points.
void vtkPoints::ComputeBounds()
{
  vtkIdType i;
  double *x;

  if ( this->GetMTime() > this->ComputeTime )
    {
    this->Bounds[0] = this->Bounds[2] = this->Bounds[4] =  VTK_DOUBLE_MAX;
    this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_DOUBLE_MAX;
    for (i=0; i<this->GetNumberOfPoints(); i++)
      {
      x = this->GetPoint(i);
      this->Bounds[0] = x[0] < this->Bounds[0] ? x[0] : this->Bounds[0];
      this->Bounds[1] = x[0] > this->Bounds[1] ? x[0] : this->Bounds[1];
      this->Bounds[2] = x[1] < this->Bounds[2] ? x[1] : this->Bounds[2];
      this->Bounds[3] = x[1] > this->Bounds[3] ? x[1] : this->Bounds[3];
      this->Bounds[4] = x[2] < this->Bounds[4] ? x[2] : this->Bounds[4];
      this->Bounds[5] = x[2] > this->Bounds[5] ? x[2] : this->Bounds[5];
      }

    this->ComputeTime.Modified();
    }
}

// Return the bounds of the points.
double *vtkPoints::GetBounds()
{
  this->ComputeBounds();
  return this->Bounds;
}

// Return the bounds of the points.
void vtkPoints::GetBounds(double bounds[6])
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

  this->Data->Delete();
  this->Data = vtkDataArray::CreateDataArray(dataType);
  this->Data->SetNumberOfComponents(3);
  this->Data->SetName("Points");
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
    if (!this->Data->GetName())
      {
      this->Data->SetName("Points");
      }
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

  os << indent << "Data: " << this->Data << "\n";
  if ( this->Data->GetName() )
    {
    os << indent << "Data Array Name: " << this->Data->GetName() << "\n";
    }
  else
    {
    os << indent << "Data Array Name: (none)\n";
    }

  os << indent << "Number Of Points: " << this->GetNumberOfPoints() << "\n";
  double *bounds = this->GetBounds();
  os << indent << "Bounds: \n";
  os << indent << "  Xmin,Xmax: (" << bounds[0] << ", " << bounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << bounds[2] << ", " << bounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << bounds[4] << ", " << bounds[5] << ")\n";
}
