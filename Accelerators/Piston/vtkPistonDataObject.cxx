/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPistonDataObject.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPistonDataObject.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPistonReference.h"

vtkStandardNewMacro(vtkPistonDataObject);

//----------------------------------------------------------------------------
vtkPistonDataObject::vtkPistonDataObject()
{
  //cerr << "PDO(" << this << ") Create" << endl;
  this->Information->Set(vtkDataObject::DATA_EXTENT_TYPE(), VTK_PIECES_EXTENT);
  this->Information->Set(vtkDataObject::DATA_PIECE_NUMBER(), -1);
  this->Information->Set(vtkDataObject::DATA_NUMBER_OF_PIECES(), 1);
  this->Information->Set(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(), 0);
  this->Reference = new vtkPistonReference;
  this->OwnReference = true;
  vtkMath::UninitializeBounds(this->Bounds);
  this->ScalarsArrayName = 0;
  this->ScalarsRange[0] = this->ScalarsRange[1] = 0.0;
}

//----------------------------------------------------------------------------
vtkPistonDataObject::~vtkPistonDataObject()
{
  //cerr << "PDO(" << this << ") Destroy" << endl;
  if (this->OwnReference)
    {
    delete this->Reference;
    }

  if (this->ScalarsArrayName)
    {
    this->SetScalarsArrayName(0);
    }
}

//----------------------------------------------------------------------------
vtkPistonDataObject* vtkPistonDataObject::GetData(vtkInformation* info)
{
  return info? vtkPistonDataObject::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkPistonDataObject* vtkPistonDataObject::GetData(vtkInformationVector* v, int i)
{
  return vtkPistonDataObject::GetData(v->GetInformationObject(i));
}

//----------------------------------------------------------------------------
void vtkPistonDataObject::PrintSelf(ostream &os, vtkIndent indent)
{
  vtkDataObject::PrintSelf(os, indent);
  os << indent << "Reference MTime: " << this->Reference->mtime << endl;
  os << indent << "Reference Type: " << this->Reference->type << endl;
  os << indent << "Reference Data: " << (this->Reference->data!=NULL?
                                         this->Reference->data:"NULL")<< endl;
}

//----------------------------------------------------------------------------
void vtkPistonDataObject::ShallowCopy(vtkDataObject* src)
{
  //cerr << "PDO(" << this << ") ShallowCopy " << src << endl;
  if (vtkPistonDataObject* const pdo = vtkPistonDataObject::SafeDownCast(src))
    {
    if (this->OwnReference)
    {
      delete this->Reference;
    }
    this->Reference = pdo->Reference;
    this->OwnReference = false;
    this->SetBounds(pdo->GetBounds());
    this->SetOrigin(pdo->GetOrigin());
    this->SetSpacing(pdo->GetSpacing());
    this->SetScalarsArrayName(pdo->GetScalarsArrayName());
    this->SetScalarsRange(pdo->GetScalarsRange());
    this->Modified();
    }

  this->Superclass::ShallowCopy(src);
}

//----------------------------------------------------------------------------
void vtkPistonDataObject::DeepCopy(vtkDataObject* src)
{
  //cerr << "PDO(" << this << ") DeepCopy " << src << endl;
  if (vtkPistonDataObject* const pdo = vtkPistonDataObject::SafeDownCast(src))
    {
    if (this->OwnReference)
    {
      delete this->Reference;
    }
    this->Reference = new vtkPistonReference(pdo->Reference);
    this->OwnReference = true;
    this->SetBounds(pdo->GetBounds());
    this->SetOrigin(pdo->GetOrigin());
    this->SetSpacing(pdo->GetSpacing());
    this->SetScalarsArrayName(pdo->GetScalarsArrayName());
    this->SetScalarsRange(pdo->GetScalarsRange());
    this->Modified();
    }

  this->Superclass::DeepCopy(src);
}

//----------------------------------------------------------------------------
int vtkPistonDataObject::GetReferredType()
{
  return this->Reference->type;
}

//----------------------------------------------------------------------------
void * vtkPistonDataObject::GetReferredData()
{
  return this->Reference->data;
}

//----------------------------------------------------------------------------
void vtkPistonDataObject::ComputeBounds()
{
  //TODO: ask pistondatawrangler to actually compute it?
  if ( this->GetMTime() > this->ComputeTime )
    {
    this->ComputeTime.Modified();
    }
}

//----------------------------------------------------------------------------
double *vtkPistonDataObject::GetBounds()
{
  this->ComputeBounds();
  return this->Bounds;
}

//----------------------------------------------------------------------------
void vtkPistonDataObject::GetBounds(double bounds[6])
{
  this->ComputeBounds();
  for (int i=0; i<6; i++)
    {
    bounds[i] = this->Bounds[i];
    }
}

//----------------------------------------------------------------------------
void vtkPistonDataObject::SetBounds(const double bounds[6])
{
  bool modified = false;
  for (int i=0; i<6; i++)
    {
    if (this->Bounds[i] != bounds[i])
      {
      modified = true;
      }
    this->Bounds[i] = bounds[i];
    }
  if (modified)
    {
    this->Modified();
    }
}

//----------------------------------------------------------------------------
double *vtkPistonDataObject::GetOrigin()
{
  return this->Origin;
}

//----------------------------------------------------------------------------
void vtkPistonDataObject::GetOrigin(double origin[3])
{
  for (int i=0; i<3; i++)
    {
    origin[i] = this->Origin[i];
    }
}

//----------------------------------------------------------------------------
void vtkPistonDataObject::SetOrigin(const double origin[3])
{
  bool modified = false;
  for (int i=0; i<3; i++)
    {
    if (this->Origin[i] != origin[i])
      {
      modified = true;
      }
    this->Origin[i] = origin[i];
    }
  if (modified)
    {
    this->Modified();
    }
}

//----------------------------------------------------------------------------
double *vtkPistonDataObject::GetSpacing()
{
  return this->Spacing;
}

//----------------------------------------------------------------------------
void vtkPistonDataObject::GetSpacing(double spacing[3])
{
  for (int i=0; i<3; i++)
    {
    spacing[i] = this->Spacing[i];
    }
}

//----------------------------------------------------------------------------
void vtkPistonDataObject::SetSpacing(double spacing[3])
{
  bool modified = false;
  for (int i=0; i<3; i++)
    {
    if (this->Spacing[i] != spacing[i])
      {
      modified = true;
      }
    this->Spacing[i] = spacing[i];
    }
  if (modified)
    {
    this->Modified();
    }
}

//----------------------------------------------------------------------------
double* vtkPistonDataObject::GetScalarsRange()
{
  return this->ScalarsRange;
}

//----------------------------------------------------------------------------
void vtkPistonDataObject::GetScalarsRange(double range[2])
{
  range[0] =  this->ScalarsRange[0];
  range[1] =  this->ScalarsRange[1];
}

//----------------------------------------------------------------------------
void vtkPistonDataObject::SetScalarsRange(double range[2])
{
  if(!range || (range[0] == this->ScalarsRange[0] &&
                range[1] == this->ScalarsRange[1]))
    {
     return;
    }

  this->ScalarsRange[0] = range[0];
  this->ScalarsRange[1] = range[1];
  this->Modified();
}
