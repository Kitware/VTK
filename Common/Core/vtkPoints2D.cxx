/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPoints2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPoints2D.h"

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
vtkInstantiatorNewMacro(vtkPoints2D);

//----------------------------------------------------------------------------
vtkPoints2D* vtkPoints2D::New(int dataType)
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPoints2D");
  if(ret)
    {
    if(dataType != VTK_FLOAT)
      {
      static_cast<vtkPoints2D*>(ret)->SetDataType(dataType);
      }
    return static_cast<vtkPoints2D*>(ret);
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPoints2D(dataType);
}

vtkPoints2D* vtkPoints2D::New()
{
  return vtkPoints2D::New(VTK_FLOAT);
}

// Construct object with an initial data array of type float.
vtkPoints2D::vtkPoints2D(int dataType)
{
  this->Data = vtkFloatArray::New();
  this->Data->Register(this);
  this->Data->Delete();
  this->SetDataType(dataType);

  this->Data->SetNumberOfComponents(2);
  this->Data->SetName("Points2D");

  this->Bounds[0] = this->Bounds[2] = 0.0;
  this->Bounds[1] = this->Bounds[3] = 1.0;
}

vtkPoints2D::~vtkPoints2D()
{
  this->Data->UnRegister(this);
}

// Given a list of pt ids, return an array of points.
void vtkPoints2D::GetPoints(vtkIdList *ptIds, vtkPoints2D *fp)
{
  vtkIdType num = ptIds->GetNumberOfIds();

  for (vtkIdType i=0; i < num; i++)
    {
    fp->InsertPoint(i, this->GetPoint(ptIds->GetId(i)));
    }
}

// Determine (xmin,xmax, ymin,ymax, zmin,zmax) bounds of points.
void vtkPoints2D::ComputeBounds()
{
  vtkIdType i;
  int j;
  double *x;

  if ( this->GetMTime() > this->ComputeTime )
    {
    this->Bounds[0] = this->Bounds[2] =  VTK_DOUBLE_MAX;
    this->Bounds[1] = this->Bounds[3] = -VTK_DOUBLE_MAX;
    for (i=0; i < this->GetNumberOfPoints(); ++i)
      {
      x = this->GetPoint(i);
      for (j=0; j < 2; ++j)
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
double *vtkPoints2D::GetBounds()
{
  this->ComputeBounds();
  return this->Bounds;
}

// Return the bounds of the points.
void vtkPoints2D::GetBounds(double bounds[4])
{
  this->ComputeBounds();
  for (int i=0; i<4; i++)
    {
    bounds[i] = this->Bounds[i];
    }
}

int vtkPoints2D::Allocate(const vtkIdType sz, const vtkIdType ext)
{
  int numComp=this->Data->GetNumberOfComponents();
  return this->Data->Allocate(sz*numComp,ext*numComp);
}

void vtkPoints2D::Initialize()
{
  this->Data->Initialize();
}

int vtkPoints2D::GetDataType()
{
  return this->Data->GetDataType();
}

// Specify the underlying data type of the object.
void vtkPoints2D::SetDataType(int dataType)
{
  if ( dataType == this->Data->GetDataType() )
    {
    return;
    }

  this->Modified();

  this->Data->Delete();
  this->Data = vtkDataArray::CreateDataArray(dataType);
  this->Data->SetNumberOfComponents(2);
  this->Data->SetName("Points2D");
}

// Set the data for this object. The tuple dimension must be consistent with
// the object.
void vtkPoints2D::SetData(vtkDataArray *data)
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
      this->Data->SetName("Points2D");
      }
    this->Modified();
    }
}

// Deep copy of data. Checks consistency to make sure this operation
// makes sense.
void vtkPoints2D::DeepCopy(vtkPoints2D *da)
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
void vtkPoints2D::ShallowCopy(vtkPoints2D *da)
{
  this->SetData(da->GetData());
}

unsigned long vtkPoints2D::GetActualMemorySize()
{
  return this->Data->GetActualMemorySize();
}

void vtkPoints2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  double *bounds;

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
}
